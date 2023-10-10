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

    auto link = FairLossLink(Address("127.0.0.1", 11001));
    std::cout << "fail" << std::endl;
    TransportMessage m(Address("127.0.0.1", 11001), "Hello");
    std::cout << "Sending m" << std::endl;
    link.send(m);
    std::cout << "Sent: " << m.payload.get()[4] << std::endl;

    link.start_receiving([](TransportMessage message) {
        std::cout << "Recevied m" << std::endl;
        std::string body(message.payload.get(), message.length);
        std::cout << "Received: " << body << "| from: " << message.address.to_string() << std::endl;
    });

    std::cout << "Changed\n";
    // After a process finishes broadcasting,
    // it waits forever for the delivery of messages.
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}
