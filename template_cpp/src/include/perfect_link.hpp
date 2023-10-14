#pragma once

#include "ack_set.hpp"
#include "concurrent_map.hpp"
#include "concurrent_queue.hpp"
#include "concurrent_set.hpp"
#include "delivery_set.hpp"
#include "fair_loss_link.hpp"
#include "host_lookup.hpp"
#include "message.hpp"
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

public:
    PerfectLink(Address address, HostLookup host_lookup)
        : link(address), host_lookup(host_lookup), acked_messages(host_lookup),
          delivered_messages(host_lookup) {
        std::cout << "Initialized Perfect link on address: " << address.to_string() << std::endl;
    }

    void send(Address address, Message &message) {
        uint64_t length = 0;
        auto payload = message.serialize(length);
        q.push(TransportMessage(address, message.get_id(), payload, length));
    }

    void shut_down() { continue_sending = false; }

    Address get_address() { return this->link.address; }

    std::thread start_sending() {
        return std::thread([this]() {
            std::cout << "Andrija7" << std::endl;
            while (this->continue_sending) {
                if (!this->q.is_empty()) {
                    auto first = this->q.dequeue();
                    if (first.has_value()) {
                        TransportMessage m = first.value();
                        bool isacked = this->acked_messages.is_acked(m);
                        std::cout << "Andrija9: " << m.get_id() << isacked << std::endl;
                        if (!this->acked_messages.is_acked(m)) {
                            this->link.send(m);
                            this->q.push(m);
                        }
                    }
                }
            }

            std::cout << "Shut down sending on node: " << this->link.address.to_string()
                      << std::endl;
        });
    }

    std::thread start_receiving(std::function<void(TransportMessage)> handler) {
        return std::thread([this, handler]() {
            this->link.start_receiving([this, handler](TransportMessage message) {
                if (message.is_ack) {
                    std::cout << "Received ack for: " << message.get_id()
                              << "from: " << message.address.to_string() << std::endl;
                    this->acked_messages.ack(message);

                    return;
                }

                std::cout << "Received message: " << message.get_id() << std::endl;
                if (!this->delivered_messages.is_delivered(message)) {
                    this->delivered_messages.deliver(message);
                    this->link.send(TransportMessage(message.address, message.get_id(),
                                                     std::shared_ptr<char[]>(new char[0]), 0,
                                                     true));
                    std::cout << "Delivering the message: " << message.get_id() << std::endl;
                    handler(message);
                }
            });
        });
    }
};