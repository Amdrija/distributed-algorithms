#pragma once

#include "sorted_list.hpp"
#include "uniform_reliable_broadcast.hpp"

class FifoBroadcast {
    UniformReliableBroadcast urb;
    std::unique_ptr<uint32_t[]> last_delivered;
    std::unique_ptr<SortedList[]> pending_messages;

public:
    // the +1 is because the hosts start from 1
    FifoBroadcast(uint8_t host_id, HostLookup lookup,
                  std::function<void(BroadcastMessage)> handler)
        : urb(host_id, lookup,
              [this, handler](BroadcastMessage message) {
                  if (this->last_delivered[message.get_source()] + 1 ==
                      message.get_sequence_number()) {
                      this->last_delivered[message.get_source()]++;
                      handler(std::move(message));
                      while (true) {
                      } // Here process the messages in order
                  } else {
                      this->pending_messages.get()->insert(std::move(message));
                  }
              }),
          last_delivered(new uint32_t[lookup.get_host_count() + 1]),
          pending_messages(new SortedList[lookup.get_host_count() + 1]) {}

    void broadcast(Message &m, uint32_t sequence_number) {
        this->urb.broadcast(m, sequence_number);
    }

    void shut_down() { this->urb.shut_down(); }
};