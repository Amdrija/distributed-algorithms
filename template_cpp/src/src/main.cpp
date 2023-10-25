#include <chrono>
#include <iostream>
#include <thread>

#include "hello.h"
#include "host_lookup.hpp"
#include "network_config.hpp"
#include "output_file.hpp"
#include "parser.hpp"
#include "perfect_link.hpp"
#include <fstream>
#include <signal.h>

std::unique_ptr<PerfectLink> perfect_link;
std::shared_ptr<OutputFile> output_file;
std::thread sending_thread, receiving_thread;

static void stop(int)
{
    // reset signal handlers to default
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    // immediately stop network packet processing
    perfect_link.get()->shut_down();
    std::cout << "Immediately stopping network packet processing.\n";

    // write/flush output file if necessary
    std::cout << "Writing output.\n";
    output_file.get()->flush();
    output_file.get()->close();
    std::cout << "Writing output.\n";

    // exit directly from signal handler
    exit(0);
}

int main(int argc, char **argv)
{
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
    auto hosts = parser.hosts();
    for (auto &host : hosts)
    {
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

    output_file = std::shared_ptr<OutputFile>(new OutputFile(parser.outputPath()));

    if (!output_file.get())
    {
        std::cout << "Failed to open the file: " << parser.outputPath() << std::endl;
    }

    std::cout << "Path to config:\n";
    std::cout << "===============\n";
    std::cout << parser.configPath() << "\n\n";
    NetworkConfig config(parser.configPath());
    std::cout << "Receiver id: " << config.get_receiver_id()
              << " messages: " << config.get_message_count() << std::endl;
    auto receiver_id = config.get_receiver_id();

    perfect_link = std::unique_ptr<PerfectLink>(
        new PerfectLink(host_lookup.get_address_by_host_id(parser.id()), host_lookup, output_file));

    std::cout << "Broadcasting and delivering messages...\n\n";
    if (config.get_receiver_id() == parser.id())
    {
        receiving_thread = perfect_link.get()->start_receiving(
            [host_lookup, receiver_id](TransportMessage message)
            {
                // std::cout << "d " << host_lookup.get_host_id_by_ip(message.address) << " "
                //           << message.get_id() << std::endl;
            });
    }
    else
    {
        Address receiver_address = host_lookup.get_address_by_host_id(config.get_receiver_id());
        for (uint64_t i = 0; i < config.get_message_count(); i++)
        {
            auto m = EmptyMessage();
            perfect_link.get()->send(receiver_address, m);
        }
        sending_thread = perfect_link.get()->start_sending();
        receiving_thread = perfect_link.get()->start_receiving([](TransportMessage m)
                                                               {
                                                                   // std::cout << "Received ack: " << m.get_id() << std::endl;
                                                               });
    }

    // After a process finishes broadcasting,
    // it waits forever for the delivery of messages.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    sending_thread.join();
    receiving_thread.join();

    return 0;
}
