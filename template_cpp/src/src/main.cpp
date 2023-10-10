#include <chrono>
#include <iostream>
#include <thread>

#include "fair_loss_link.hpp"
#include "hello.h"
#include "network_config.hpp"
#include "parser.hpp"
#include <signal.h>

static void stop(int) {
    // reset signal handlers to default
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    // immediately stop network packet processing
    std::cout << "Immediately stopping network packet processing.\n";

    // write/flush output file if necessary
    std::cout << "Writing output.\n";

    // exit directly from signal handler
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    // `true` means that a config file is required.
    // Call with `false` if no config file is necessary.
    bool requireConfig = true;

    Parser parser(argc, argv);
    parser.parse();

    hello();
    std::cout << std::endl;

    std::cout << "My PID: " << getpid() << "\n";
    std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
              << getpid() << "` to stop processing packets\n\n";

    std::cout << "My ID: " << parser.id() << "\n\n";

    std::cout << "List of resolved hosts is:\n";
    std::cout << "==========================\n";
    // TODO: Convert to a map for faster reads
    auto hosts = parser.hosts();
    for (auto &host : hosts) {
        std::cout << host.id << "\n";
        std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
        std::cout << "Machine-readable IP: " << host.ip << "\n";
        std::cout << "Human-readbale Port: " << host.portReadable() << "\n";
        std::cout << "Machine-readbale Port: " << host.port << "\n";
        std::cout << "\n";
    }
    std::cout << "\n";

    std::cout << "Path to output:\n";
    std::cout << "===============\n";
    std::cout << parser.outputPath() << "\n\n";
    // TODO: Write to OUTPUT

    std::cout << "Path to config:\n";
    std::cout << "===============\n";
    std::cout << parser.configPath() << "\n\n";
    NetworkConfig config(parser.configPath());
    std::cout << "Sender id: " << config.get_sender_id() << " messages: " << config.get_message_count() << std::endl;

    std::cout << "Doing some initialization...\n\n";

    std::cout << "Broadcasting and delivering messages...\n\n";

    auto link = new FairLossLink("127.0.0.1", 11001);

    link->start_receiving([](const uint32_t source_ip, const uint16_t source_port, std::string message) {
        char source_buffer[16];
        inet_ntop(AF_INET, &(source_ip), source_buffer, 16);
        std::string source = std::string(source_buffer, 16);
        source.append(":");
        source.append(std::to_string((int)ntohs(source_port)));
        std::cout << "Received: " << message << "| from: " << source << std::endl;
    });

    std::cout << "Changed\n";
    // After a process finishes broadcasting,
    // it waits forever for the delivery of messages.
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}
