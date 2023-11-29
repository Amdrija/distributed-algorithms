#pragma once

#include "sorted_list.hpp"
#include "uniform_reliable_broadcast.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>

class FifoBroadcast {
    uint8_t host_id;
    UniformReliableBroadcast urb;
    std::unique_ptr<std::atomic_uint32_t[]> last_delivered;
    std::unique_ptr<SortedList[]> pending_messages;
    std::function<void(BroadcastMessage)> handler;
    std::mutex mutex;
    std::condition_variable cv;
    bool stop_sending = false;
    const uint32_t SEND_Q_SIZE = 20;

public:
    // the +1 is because the hosts start from 1
    FifoBroadcast(uint8_t host_id, HostLookup lookup,
                  std::function<void(BroadcastMessage)> handler)
        : host_id(host_id), urb(host_id, lookup,
                                [this](BroadcastMessage message) {
                                    this->handle_message(std::move(message));
                                }),
          last_delivered(new std::atomic_uint32_t[lookup.get_host_count() + 1]),
          pending_messages(new SortedList[lookup.get_host_count() + 1]),
          handler(handler),
          SEND_Q_SIZE(
              100 - (lookup.get_host_count() * lookup.get_host_count()) / 200) {
        for (auto host : lookup.get_hosts()) {
            this->last_delivered[host] = 0;
        }
    }

    bool broadcast(Message &m, uint32_t sequence_number) {
        {

            std::unique_lock<std::mutex> lock(this->mutex);
            while (this->last_delivered[this->host_id] <
                   static_cast<int64_t>(sequence_number) - this->SEND_Q_SIZE) {
                // std::cout << "SLEEPING" << std::endl;
                this->cv.wait(lock);
                // std::cout << "WOKE UP" << std::endl;
                if (this->stop_sending) {
                    return false;
                }
            }
        }

        if (this->stop_sending) {
            return false;
        }

        this->urb.broadcast(m, sequence_number);

        return true;
    }

    void shut_down() {
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->stop_sending = true;
        }
        this->cv.notify_all();

        this->urb.shut_down();
    }

private:
    void handle_message(BroadcastMessage message) {
        // std::cout << "DELIVERING" << std::endl;
        uint8_t host = message.get_source();
        // std::cout << "HOST: " << static_cast<int>(host) << std::endl;
        auto to_deliver = this->pending_messages.get()[host].to_be_delivered(
            std::move(message), this->last_delivered.get()[host]);
        // std::cout << "AFTER TO DELIVER" << std::endl;
        if (!to_deliver.empty()) {
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                this->last_delivered[host] +=
                    static_cast<uint32_t>(to_deliver.size());
            }
            // std::cout << "NOTIFYING" << std::endl;
            this->cv.notify_all();

            while (!to_deliver.empty()) {
                // std::cout << "HANDLING MESSAGE: " << std::endl;
                this->handler(std::move(to_deliver.front()));
                to_deliver.pop_front();
            }
        }
    }
};