#pragma once

#include "perfect_link.hpp"
#include <condition_variable>
#include <set>

class LatticeAgreement {
    PerfectLink link;
    HostLookup lookup;
    std::unordered_map<uint64_t, uint8_t> ack_count;
    std::unordered_map<uint64_t, uint8_t> nack_count;
    std::unordered_map<uint64_t, uint64_t> active_proposal;
    std::unordered_map<uint64_t, std::unordered_set<propose_value>>
        proposed_value;
    std::unordered_map<uint64_t, std::unordered_set<propose_value>>
        accepted_value;
    const uint8_t threshold = 0;
    std::function<void(std::unordered_set<propose_value>, uint64_t)> handler;
    std::set<uint64_t> rounds_to_deliver;
    uint64_t last_delivered = 0;
    std::mutex lock;
    std::mutex cv_mutex;
    std::condition_variable cv;
    const uint32_t SEND_Q_SIZE = 100;
    bool stop_sending = false;

public:
    LatticeAgreement(
        HostLookup lookup, uint8_t host_id,
        std::function<void(std::unordered_set<propose_value>, uint64_t)>
            handler)
        : link(lookup.get_address_by_host_id(host_id), lookup), lookup(lookup),
          threshold(static_cast<uint8_t>(lookup.get_host_count() / 2 + 1)),
          handler(handler) {
        // std::cout << "Threshold: " << static_cast<int64_t>(this->threshold)
        //           << std::endl;
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
                // std::cout << "SLEEPING" << std::endl;
                this->cv.wait(cv_lock);
                // std::cout << "WOKE UP" << std::endl;
                if (this->stop_sending) {
                    return false;
                }
            }
        }

        if (this->stop_sending) {
            return false;
        }

        ProposeMessage msg(round, 1, propose_set, ProposeType::Proposal);
        this->lock.lock();
        this->proposed_value[round] = propose_set;
        this->active_proposal[round] = 1;
        this->ack_count[round] = 1;
        this->nack_count[round] = 0;
        this->accepted_value[round] = propose_set;
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
            if (this->active_proposal[message.round] ==
                message.proposal_number) {
                this->process_nack(std::move(message));
            }
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
        // TODO: Optimize so we don't have to check when we have already
        // sent out a NACK or decided, i.e ack_count > threshold
        if (this->active_proposal[message.round] != message.proposal_number) {
            return;
        }

        this->ack_count[message.round]++;
        if (this->at_threshold(message.round)) {
            this->process_threshold(std::move(message));
        }
    }

    void process_nack(ProposeMessage message) {
        if (this->active_proposal[message.round] != message.proposal_number) {
            return;
        }

        LatticeAgreement::set_union(this->proposed_value[message.round],
                                    message.proposed_values);
        this->nack_count[message.round]++;

        if (this->at_threshold(message.round)) {
            this->process_threshold(std::move(message));
        }
    }

    void process_threshold(ProposeMessage message) {
        if (this->nack_count[message.round] == 0) {
            // means we can decide
            // add it to the fifo queue for deciding
            // so that we can decide in the order of the rounds
            this->rounds_to_deliver.insert(message.round);
            auto first = this->rounds_to_deliver.cbegin();
            auto delivered = 0;
            while (!this->rounds_to_deliver.empty() &&
                   this->last_delivered + delivered + 1 == *first) {

                this->handler(this->accepted_value[*first], *first);
                this->rounds_to_deliver.erase(first);
                delivered++;
                first = this->rounds_to_deliver.cbegin();
            }
            // std::cout << "Delivered from: " << this->last_delivered + 1
            //           << " to: " << this->last_delivered + delivered + 1
            //           << std::endl;
            {
                std::unique_lock<std::mutex> cv_lock(this->cv_mutex);
                this->last_delivered += delivered;
            }
            // std::cout << "NOTIFYING" << std::endl;
            this->cv.notify_all();
        } else {
            this->active_proposal[message.round]++;
            this->ack_count[message.round] = 1;
            this->nack_count[message.round] = 0;
            LatticeAgreement::set_union(this->accepted_value[message.round],
                                        this->proposed_value[message.round]);

            ProposeMessage new_proposal(
                message.round, this->active_proposal[message.round],
                this->proposed_value[message.round], ProposeType::Proposal);

            // TODO: Broadcast to self
            this->link.broadcast(new_proposal);
        }
    }

    void process_proposal(ProposeMessage message, Address source) {
        // std::cout << this->link.get_address().to_string()
        //           << " got proposal from " << source.to_string() <<
        //           std::endl;
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

    bool at_threshold(uint64_t round) {
        // std::cout << "ACK: " << this->ack_count[round] << " "
        //           << this->nack_count[round] << std::endl;
        return this->ack_count[round] + this->nack_count[round] ==
               this->threshold;
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
            if (superset.find(element) == superset.cend()) {
                return false;
            }
        }

        return true;
    }
};