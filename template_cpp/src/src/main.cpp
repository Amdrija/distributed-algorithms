#include <chrono>
#include <iostream>
#include <thread>

#include "fifo_broadcast.hpp"
#include "fifo_config.hpp"
#include "hello.h"
#include "host_lookup.hpp"
#include "interval_set.hpp"
#include "network_config.hpp"
#include "output_file.hpp"
#include "parser.hpp"
#include "perfect_link.hpp"
#include <fstream>
#include <signal.h>

std::unique_ptr<FifoBroadcast> broadcast;
std::shared_ptr<OutputFile> output_file;

static void stop(int) {
    // reset signal handlers to default
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    // immediately stop network packet processing
    broadcast.get()->shut_down();
    std::cout << "Immediately stopping network packet processing.\n";

    // write/flush output file if necessary
    std::cout << "Writing output." << std::endl;
    // output_file.get()->flush();
    // output_file.get()->close();

    // exit directly from signal handler
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    // `true` means that a config file is required.
    // Call with `false` if no config file is necessary.
    bool requireConfig = true;
    // IntervalSet iset;
    // std::cout << iset.insert(1) << std::endl;
    // std::cout << iset.insert(2) << std::endl;
    // std::cout << iset.insert(5) << std::endl;
    // std::cout << iset.insert(6) << std::endl;
    // std::cout << iset.insert(7) << std::endl;
    // std::cout << iset.insert(11) << std::endl;
    // std::cout << iset.insert(12) << std::endl;
    // std::cout << iset.to_string() << std::endl;
    // std::cout << iset.insert(3) << std::endl;
    // std::cout << iset.insert(5) << std::endl;
    // std::cout << iset.insert(6) << std::endl;
    // std::cout << iset.insert(7) << std::endl;
    // std::cout << iset.insert(4) << std::endl;
    // std::cout << iset.to_string() << std::endl;
    // std::cout << iset.insert(9) << std::endl;
    // std::cout << iset.to_string() << std::endl;
    // std::cout << iset.insert(8) << std::endl;
    // std::cout << iset.insert(10) << std::endl;
    // std::cout << iset.to_string() << std::endl;
    // std::cout << iset.contains(10) << std::endl;
    // std::cout << iset.contains(0) << std::endl;
    // std::cout << iset.contains(12) << std::endl;
    // std::cout << iset.contains(120) << std::endl;

    // return 0;

    Parser parser(argc, argv);
    parser.parse();

    hello();
    std::cout << std::endl;

    std::cout << "My PID: " << getpid() << "\n";
    std::cout << "From a new terminal type `kill -SIGINT " << getpid()
              << "` or `kill -SIGTERM " << getpid()
              << "` to stop processing packets\n\n";

    std::cout << "My ID: " << parser.id() << "\n\n";

    std::cout << "List of resolved hosts is:\n";
    std::cout << "==========================\n";
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
    HostLookup host_lookup(parser.hostsPath());

    std::cout << "Path to output:\n";
    std::cout << "===============\n";
    std::cout << parser.outputPath() << "\n\n";

    output_file =
        std::shared_ptr<OutputFile>(new OutputFile(parser.outputPath()));

    if (!output_file.get()) {
        std::cout << "Failed to open the file: " << parser.outputPath()
                  << std::endl;
    }

    std::cout << "Path to config:\n";
    std::cout << "===============\n";
    std::cout << parser.configPath() << "\n\n";
    FifoConfig config(parser.configPath());
    std::cout << "Messages to send: " << config.get_message_count()
              << std::endl;

    std::cout << "Broadcasting and delivering messages...\n\n";
    broadcast = std::unique_ptr<FifoBroadcast>(new FifoBroadcast(
        static_cast<uint8_t>(parser.id()), host_lookup,
        [](BroadcastMessage bm) {
            output_file.get()->write(
                "d " + std::to_string(static_cast<int>(bm.get_source())) + " " +
                std::to_string(bm.get_sequence_number()) + "\n");
        }));

    for (uint32_t i = 1; i <= config.get_message_count(); i++) {
        EmptyMessage em;
        // output_file.get()->write("b " + std::to_string(i) + "\n");
        broadcast.get()->broadcast(em, i);
    }

    // After a process finishes broadcasting,
    // it waits forever for the delivery of messages.
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}
