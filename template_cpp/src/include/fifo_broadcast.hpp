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
        if (host == 1) {
            std::cout << "Andrija " << this->last_delivered.get()[host]
                      << std::endl;
        }
        if (this->last_delivered[host] + 1 == message.get_sequence_number()) {
            this->last_delivered[host]++;
            this->handler(std::move(message));

            // TODO: Possible optimization to process in batches
            while (!this->pending_messages.get()[host].empty() &&
                   this->last_delivered[host] + 1 ==
                       this->pending_messages.get()[host]
                           .lowest_sequence_number()) {
                this->last_delivered[host]++;
                handler(this->pending_messages.get()[host].pop_front());
            } // Here process the messages in order
        } else {
            this->pending_messages.get()->insert(std::move(message));
        }
    }
};