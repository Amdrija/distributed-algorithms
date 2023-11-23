#pragma once

#include "broadcast_ack_set.hpp"
#include "broadcast_set.hpp"
#include "perfect_link.hpp"

class UniformReliableBroadcast {
    PerfectLink link;
    HostLookup lookup;
    uint8_t host_id;
    uint8_t delivery_threshold;
    BroadcastSet pending_messages;
    BroadcastSet delivered_messages;
    BroadcastAckSet acked_messages;
    std::function<void(BroadcastMessage)> handler;

public:
    UniformReliableBroadcast(uint8_t host_id, HostLookup lookup,
                             std::function<void(BroadcastMessage)> handler)
        : link(lookup.get_address_by_host_id(host_id), lookup), lookup(lookup),
          host_id(host_id),
          delivery_threshold(static_cast<uint8_t>(lookup.get_host_count() / 2)),
          pending_messages(lookup.get_host_count()),
          delivered_messages(lookup.get_host_count()), acked_messages(lookup),
          handler(handler) {
        std::thread sending_thread = this->link.start_sending();
        sending_thread.detach();

        std::thread receiving_thread =
            this->link.start_receiving([this](TransportMessage t_msg) {
                // std::cout << "Received: " << t_msg.get_id()
                //           << " from: " << t_msg.address.to_string()
                //           << std::endl;
                this->deliver(
                    BroadcastMessage(t_msg.get_payload(), t_msg.length),
                    this->lookup.get_host_id_by_ip(t_msg.address));
            });
        receiving_thread.detach();
    }

    void broadcast(Message &m, uint32_t sequence_number) {
        deliver(BroadcastMessage(m, sequence_number, this->host_id),
                this->host_id);
    }

    void shut_down() { this->link.shut_down(); }

private:
    void deliver(BroadcastMessage m, uint8_t from) {
        // std::cout << "SN: " << m.get_sequence_number()
        //           << " source: " << static_cast<uint32_t>(m.get_source())
        //           << std::endl;
        this->acked_messages.ack(m, from);

        if (!this->pending_messages.contains(m)) {
            // std::cout << "PENDING" << std::endl;
            this->acked_messages.ack(m, this->host_id);
            this->pending_messages.add(m);
            this->link.broadcast(m);
        }
        // std::cout << "ACK_COUNT: "
        //           << static_cast<int>(this->acked_messages.ack_count(m))
        //           << std::endl;
        if (!this->delivered_messages.contains(m) &&
            this->acked_messages.ack_count(m) > this->delivery_threshold) {
            // std::cout << "DELIVERED" << std::endl;
            this->delivered_messages.add(m);
            // TODO: Remove message from pending?
            this->handler(std::move(m));
        }
    }
};