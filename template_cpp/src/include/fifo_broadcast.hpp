#pragma once

#include "sorted_list.hpp"
#include "uniform_reliable_broadcast.hpp"
#include <atomic>

// TODO: Improve the atrocious performance
class FifoBroadcast {
    UniformReliableBroadcast urb;
    std::unique_ptr<std::atomic_uint32_t[]> last_delivered;
    std::unique_ptr<SortedList[]> pending_messages;
    std::function<void(BroadcastMessage)> handler;

public:
    // the +1 is because the hosts start from 1
    FifoBroadcast(uint8_t host_id, HostLookup lookup,
                  std::function<void(BroadcastMessage)> handler)
        : urb(host_id, lookup,
              [this](BroadcastMessage message) {
                  //   this->handler(std::move(message));
                  this->handle_message(std::move(message));
              }),
          last_delivered(new std::atomic_uint32_t[lookup.get_host_count() + 1]),
          pending_messages(new SortedList[lookup.get_host_count() + 1]),
          handler(handler) {
        for (auto host : lookup.get_hosts()) {
            this->last_delivered[host] = 0;
        }
    }

    void broadcast(Message &m, uint32_t sequence_number) {
        this->urb.broadcast(m, sequence_number);
    }

    void shut_down() { this->urb.shut_down(); }

private:
    void handle_message(BroadcastMessage message) {
        uint8_t host = message.get_source();
        auto to_deliver = this->pending_messages.get()[host].to_be_delivered(
            std::move(message), this->last_delivered.get()[host]);
        std::cout << "MSG: " << message.get_sequence_number()
                  << " TO DELIVER: " << to_deliver.size() << std::endl;
        if (!to_deliver.empty()) {
            for (auto &msg : to_deliver) {
                this->handler(std::move(msg));
            }

            this->last_delivered[host] +=
                static_cast<uint32_t>(to_deliver.size());
        }
    }
};