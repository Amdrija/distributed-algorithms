#pragma once

#include "ack_set.hpp"
#include "concurrent_map.hpp"
#include "concurrent_queue.hpp"
#include "concurrent_set.hpp"
#include "delivery_set.hpp"
#include "fair_loss_link.hpp"
#include "host_lookup.hpp"
#include "message.hpp"
#include "output_file.hpp"
#include <fstream>
#include <set>
#include <string>
#include <thread>

class PerfectLink {
private:
    ConcurrentQueue<TransportMessage> q;
    FairLossLink link;
    bool continue_sending = true;
    HostLookup host_lookup;
    AckSet acked_messages;
    DeliverySet delivered_messages;
    uint8_t host_id;

public:
    PerfectLink(Address address, HostLookup host_lookup)
        : link(address, host_lookup), host_lookup(host_lookup),
          acked_messages(host_lookup), delivered_messages(host_lookup),
          host_id(host_lookup.get_host_id_by_ip(address)) {
        std::cout << "Initialized Perfect link on address: "
                  << address.to_string() << std::endl;
    }

    void send(Address address, Message &message) {
        // std::cout << "Triggered Perfect link send on address: " <<
        // address.to_string() << std::endl;
        uint64_t length = 0;
        auto payload = message.serialize(length);

        q.push(TransportMessage(address, std::move(payload), length));
    }

    void broadcast(Message &message) {
        uint64_t length = 0;
        std::shared_ptr payload = message.serialize(length);

        TransportMessage tm =
            TransportMessage(this->link.address, payload, length);

        for (auto host : this->host_lookup.get_hosts()) {
            // if (host != this->host_id) {
            auto address = this->host_lookup.get_address_by_host_id(host);

            q.push(TransportMessage(tm, address));
            // }
        }
    }

    void shut_down() {
        this->link.stop_receiving();
        this->stop_sending();
    }

    Address get_address() { return this->link.address; }

    std::thread start_sending() {
        std::cout << "Started sending on: " << this->link.address.to_string()
                  << std::endl;

        return std::thread([this]() {
            while (true) {
                // TODO: Use a condition variable isntead
                auto first = this->q.dequeue();
                if (first.has_value()) {
                    TransportMessage m = first.value();

                    if (!this->acked_messages.is_acked(m)) {
                        if (!this->continue_sending) {
                            break;
                        }
                        this->link.send(m);
                        q.push(m);
                    }
                }
            }

            std::cout << "Shut down sending on node: "
                      << this->link.address.to_string() << std::endl;
        });
    }

    std::thread start_receiving(std::function<void(TransportMessage)> handler) {
        std::cout << "Started receiving on: " << this->link.address.to_string()
                  << std::endl;

        return std::thread([this, handler]() {
            this->link.start_receiving(
                [this, handler](TransportMessage message) {
                    if (message.is_ack) {
                        // std::cout << "Received ack for: " << message.get_id()
                        //           << "from: " << message.address.to_string()
                        //           << std::endl;
                        this->acked_messages.ack(message);

                        return;
                    }

                    // std::cout << "Received message: " << message.get_id()
                    //           << " from: " << message.address.to_string() <<
                    //           std::endl;

                    this->link.send(TransportMessage::create_ack(message));
                    if (!this->delivered_messages.is_delivered(message)) {
                        this->delivered_messages.deliver(message);
                        // std::cout << "Delivering the message: " <<
                        // message.get_id() << std::endl;

                        handler(message);
                    }
                });
        });
    }

private:
    void stop_sending() {
        this->continue_sending = false;
        std::cout << "Stopping sending on node: "
                  << this->link.address.to_string() << std::endl;
    }
};