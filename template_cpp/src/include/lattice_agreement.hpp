#pragma once

#include "perfect_link.hpp"

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
    std::function<void(ProposeMessage)> handler;
    std::mutex lock;

public:
    LatticeAgreement(HostLookup lookup, uint8_t host_id,
                     std::function<void(ProposeMessage)> handler)
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

    void propose(uint64_t round,
                 std::unordered_set<propose_value> propose_set) {
        ProposeMessage msg(round, 1, propose_set, ProposeType::Proposal);
        this->lock.lock();
        this->proposed_value[round] = propose_set;
        this->active_proposal[round] = 1;
        this->ack_count[round] = 0;
        this->nack_count[round] = 0;
        // LatticeAgreement::set_union(this->accepted_value[round],
        // propose_set);
        this->lock.unlock();

        // TODO: Broadcast to self
        this->link.broadcast(msg);
    }

    void shut_down() { this->link.shut_down(); }

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
            this->handler(std::move(message));
        } else {
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
        std::cout << this->link.get_address().to_string()
                  << " got proposal from " << source.to_string() << std::endl;
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
                                ProposeType::Ack);

            this->link.send(source, nack);
        }
    }

    bool at_threshold(uint64_t round) {
        return this->ack_count[round] + this->nack_count[round] ==
               this->threshold;
    }

    static void set_union(std::unordered_set<propose_value> &dest,
                          std::unordered_set<propose_value> source) {
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