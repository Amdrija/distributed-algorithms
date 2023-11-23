#include <chrono>
#include <iostream>
#include <thread>

#include "hello.h"
#include "host_lookup.hpp"
#include "interval_set.hpp"
#include "network_config.hpp"
#include "output_file.hpp"
#include "parser.hpp"
#include "perfect_link.hpp"
#include <fstream>
#include <signal.h>

std::unique_ptr<PerfectLink> perfect_link;
std::shared_ptr<OutputFile> output_file;
std::thread sending_thread, receiving_thread;

static void stop(int) {
    // reset signal handlers to default
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    // immediately stop network packet processing
    perfect_link.get()->shut_down();
    std::cout << "Immediately stopping network packet processing.\n";

    // write/flush output file if necessary
    std::cout << "Writing output." << std::endl;
    output_file.get()->flush();
    output_file.get()->close();

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
    NetworkConfig config(parser.configPath());
    std::cout << "Receiver id: " << config.get_receiver_id()
              << " messages: " << config.get_message_count() << std::endl;
    auto receiver_id = config.get_receiver_id();

    // StringMessage s(std::string("yo mam"));
    // BroadcastMessage a(s, 1, 1);
    // uint64_t length;
    // std::unique_ptr<char[]> p = a.serialize(length);
    // BroadcastMessage a1(std::move(p), length);
    // std::cout << "d " + std::to_string(static_cast<int>(a1.get_source())) +
    //                  " " + std::to_string(a1.get_sequence_number());
    // auto inner_msg = std::move(a1).get_message();
    // std::cout
    //     << " {" + static_cast<StringMessage
    //     *>(inner_msg.get())->get_message() +
    //            "}"
    //     << std::endl;
    // return 0;
    perfect_link = std::unique_ptr<PerfectLink>(new PerfectLink(
        host_lookup.get_address_by_host_id(static_cast<uint8_t>(parser.id())),
        host_lookup, output_file));

    std::cout << "Broadcasting and delivering messages...\n\n";
    sending_thread = perfect_link.get()->start_sending();
    receiving_thread =
        perfect_link.get()->start_receiving([](TransportMessage message) {
            // std::cout << "Received ack: " << m.get_id() << std::endl;
            auto bm = BroadcastMessage(message.get_payload(), message.length);
            output_file.get()->write(
                "d " + std::to_string(static_cast<int>(bm.get_source())) + " " +
                std::to_string(bm.get_sequence_number()));
            auto inner_msg = std::move(bm).get_message();
            output_file.get()->write(
                " {" +
                static_cast<StringMessage *>(inner_msg.get())->get_message() +
                "}\n");
        });
    Address receiver_address = host_lookup.get_address_by_host_id(
        static_cast<uint8_t>(config.get_receiver_id()));
    for (uint64_t i = 0; i < config.get_message_count(); i++) {
        auto m =
            StringMessage(std::string("Message: ") + std::to_string(i + 1));
        auto bm = BroadcastMessage(m, static_cast<uint32_t>(i + 1),
                                   static_cast<uint8_t>(parser.id()));
        // output_file.get()->write("b " + std::to_string(i + 1) + "\n");
        perfect_link.get()->broadcast(bm);
    }

    sending_thread.detach();
    receiving_thread.detach();

    // After a process finishes broadcasting,
    // it waits forever for the delivery of messages.
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}
