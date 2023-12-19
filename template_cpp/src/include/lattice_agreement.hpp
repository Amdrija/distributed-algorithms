#pragma once

#include "perfect_link.hpp"
#include <condition_variable>
#include <map>

class LatticeAgreement {
    PerfectLink link;
    HostLookup lookup;
    std::unordered_map<uint64_t, bool> active;
    std::unordered_map<uint64_t, uint8_t> ack_count;
    std::unordered_map<uint64_t, uint8_t> nack_count;
    std::unordered_map<uint64_t, uint64_t> active_proposal;
    std::unordered_map<uint64_t, std::unordered_set<propose_value>>
        proposed_value;
    std::unordered_map<uint64_t, std::unordered_set<propose_value>>
        accepted_value;
    const uint8_t threshold = 0;
    std::function<void(const std::string &, uint64_t)> handler;
    std::map<uint64_t, std::string> rounds_to_deliver;
    uint64_t last_delivered = 0;
    std::mutex lock;
    std::mutex cv_mutex;
    std::condition_variable cv;
    const uint32_t SEND_Q_SIZE = 200;
    bool stop_sending = false;

public:
    LatticeAgreement(HostLookup lookup, uint8_t host_id,
                     std::function<void(const std::string &, uint64_t)> handler)
        : link(lookup.get_address_by_host_id(host_id), lookup), lookup(lookup),
          threshold(static_cast<uint8_t>(lookup.get_host_count() / 2 + 1)),
          handler(handler) {
        auto sending_thread = this->link.start_sending();
        sending_thread.detach();

        auto receiving_thread =
            this->link.start_receiving([this](TransportMessage msg) {
                ProposeMessage message(msg.get_payload(), msg.length);
                this->handle_message(std::move(message), msg.address);
            });

        receiving_thread.detach();
    }

    bool propose(uint64_t round,
                 std::unordered_set<propose_value> propose_set) {
        {

            std::unique_lock<std::mutex> cv_lock(this->cv_mutex);
            while (static_cast<int64_t>(this->last_delivered) <
                   static_cast<int64_t>(round) - this->SEND_Q_SIZE) {
                this->cv.wait(cv_lock);
                if (this->stop_sending) {
                    return false;
                }
            }
        }

        if (this->stop_sending) {
            return false;
        }

        this->lock.lock();
        this->proposed_value[round] = propose_set;
        this->active[round] = true;
        this->active_proposal[round] = 1;
        this->ack_count[round] = 0;
        this->nack_count[round] = 0;
        ProposeMessage msg(round, this->active_proposal[round],
                           this->proposed_value[round], ProposeType::Proposal);
        this->lock.unlock();

        // TODO: Broadcast to self
        this->link.broadcast(msg);

        return true;
    }

    void shut_down() {
        {
            std::unique_lock<std::mutex> cv_lock(this->cv_mutex);
            this->stop_sending = true;
        }
        this->cv.notify_all();

        this->link.shut_down();
    }

private:
    void handle_message(ProposeMessage message, Address source) {
        this->lock.lock();
        switch (message.propose_type) {
        case ProposeType::Ack:
            this->process_ack(std::move(message));
            break;
        case ProposeType::Nack:
            this->process_nack(std::move(message));
            break;
        case ProposeType::Proposal:
            this->process_proposal(std::move(message), source);
            break;
        default:
            break;
        }
        this->lock.unlock();
    }

    void process_ack(ProposeMessage message) {
        if (this->active_proposal[message.round] != message.proposal_number) {
            return;
        }

        this->ack_count[message.round]++;
        if (this->active[message.round] &&
            this->ack_count[message.round] == this->threshold) {
            // means we can decide
            // add it to the fifo queue for deciding
            // so that we can decide in the order of the rounds
            this->active[message.round] = false;
            this->rounds_to_deliver.emplace(
                message.round, LatticeAgreement::set_to_string(
                                   this->proposed_value[message.round]));
            auto current = this->rounds_to_deliver.cbegin();
            auto delivered = 0;
            while (!this->rounds_to_deliver.empty() &&
                   this->last_delivered + delivered + 1 == current->first) {

                this->handler(current->second, current->first);
                this->rounds_to_deliver.erase(current);
                delivered++;
                current = this->rounds_to_deliver.cbegin();
            }

            {
                std::unique_lock<std::mutex> cv_lock(this->cv_mutex);
                this->last_delivered += delivered;
            }
            this->cv.notify_all();
        }
    }

    void process_nack(ProposeMessage message) {
        if (this->active_proposal[message.round] != message.proposal_number) {
            return;
        }

        LatticeAgreement::set_union(this->proposed_value[message.round],
                                    message.proposed_values);
        this->nack_count[message.round]++;

        if (this->active[message.round]) {
            this->active_proposal[message.round]++;
            this->ack_count[message.round] = 0;
            this->nack_count[message.round] = 0;

            ProposeMessage new_proposal(
                message.round, this->active_proposal[message.round],
                this->proposed_value[message.round], ProposeType::Proposal);

            // TODO: Broadcast to self
            this->link.broadcast(new_proposal);
        }
    }

    void process_proposal(ProposeMessage message, Address source) {
        if (LatticeAgreement::is_subset(this->accepted_value[message.round],
                                        message.proposed_values)) {
            this->accepted_value[message.round] = message.proposed_values;
            ProposeMessage ack(message.round, message.proposal_number,
                               std::unordered_set<propose_value>(),
                               ProposeType::Ack);

            this->link.send(source, ack);
        } else {
            LatticeAgreement::set_union(this->accepted_value[message.round],
                                        message.proposed_values);
            ProposeMessage nack(message.round, message.proposal_number,
                                this->accepted_value[message.round],
                                ProposeType::Nack);

            this->link.send(source, nack);
        }
    }

    static void set_union(std::unordered_set<propose_value> &dest,
                          std::unordered_set<propose_value> &source) {
        for (auto value : source) {
            dest.insert(value);
        }
    }

    static bool is_subset(std::unordered_set<propose_value> &subset,
                          std::unordered_set<propose_value> &superset) {
        if (superset.size() < subset.size()) {
            return false;
        }

        for (auto element : subset) {
            if (superset.count(element) == 0) {
                return false;
            }
        }

        return true;
    }

    static std::string set_to_string(std::unordered_set<propose_value> &set) {
        std::string result;

        for (auto value : set) {
            result += std::to_string(value) + " ";
        }

        result.pop_back();

        return result;
    }
};