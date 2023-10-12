#pragma once

#include "concurrent_queue.hpp"
#include "fair_loss_link.hpp"
#include "message.hpp"
#include <fstream>
#include <string>
#include <thread>

class PerfectLink {
private:
    ConcurrentQueue<TransportMessage> q;
    FairLossLink link;
    bool continue_sending = true;

public:
    PerfectLink(Address address) : link(address) {
    }

    void send(Address address, Message &message) {
        uint64_t length = 0;
        auto payload = message.serialize(length);
        q.push(TransportMessage(address, message.id, payload, length));
    }

    void shut_down() {
        continue_sending = false;
    }

    Address get_address() {
        return this->link.address;
    }

    std::thread start_sending() {
        return std::thread([this]() {
            this->send_scheduler();
        });
    }

    std::thread start_receiving(std::function<void(TransportMessage)> handler) {
        return std::thread([this, handler]() {
            this->link.start_receiving(handler);
        });
    }

private:
    void send_scheduler() {
        while (continue_sending) {
            if (!q.is_empty()) {
                link.send(q.dequeue());
            }
        }

        std::cout << "Shut down sending on node: " << this->link.address.to_string() << std::endl;
    }
};