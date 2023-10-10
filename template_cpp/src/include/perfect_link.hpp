#pragma once

#include "concurrent_queue.hpp"
#include "fair_loss_link.hpp"
#include <string>

class PerfectLink {
private:
    ConcurrentQueue<std::string> q;
    FairLossLink link;

public:
    PerfectLink(const std::string &ip_address, unsigned short port) : link(ip_address, port) {
    }

    void send(const std::string &destination_address, unsigned short desination_port, const std::string &message) {
        // TODO: Implement
    }
};