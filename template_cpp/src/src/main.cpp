#include <chrono>
#include <iostream>
#include <thread>

#include "fifo_broadcast.hpp"
#include "fifo_config.hpp"
#include "hello.h"
#include "host_lookup.hpp"
#include "interval_set.hpp"
#include "lattice_agreement.hpp"
#include "lattice_config.hpp"
#include "network_config.hpp"
#include "output_file.hpp"
#include "parser.hpp"
#include "perfect_link.hpp"
#include <fstream>
#include <signal.h>

std::unique_ptr<LatticeAgreement> agreement;
std::shared_ptr<OutputFile> output_file;

static void stop(int) {
    // reset signal handlers to default
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    // immediately stop network packet processing
    agreement->shut_down();
    // WAITING FOR THREADS TO SHUT DOWN
    std::cout << "Waiting for threads to shut down" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
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
    // for (auto &host : hosts) {
    //     std::cout << host.id << "\n";
    //     std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    //     std::cout << "Machine-readable IP: " << host.ip << "\n";
    //     std::cout << "Human-readbale Port: " << host.portReadable() << "\n";
    //     std::cout << "Machine-readbale Port: " << host.port << "\n";
    //     std::cout << "\n";
    // }
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

    LatticeConfig config(parser.configPath());
    std::cout << "Rounds: " << config.round_count << std::endl;
    std::cout << "Set max size: " << config.set_max_size << std::endl;
    std::cout << "Distinct count: " << config.distinct_count << std::endl;
    int i = 0;
    for (auto set : config.proposals) {
        std::cout << "Proposed set " << ++i;
        for (auto value : set) {
            std::cout << " " << value;
        }
        std::cout << std::endl;
    }

    auto id = parser.id();
    agreement = std::unique_ptr<LatticeAgreement>(new LatticeAgreement(
        host_lookup, static_cast<uint8_t>(parser.id()),
        [id](const std::string &decided_set, uint64_t round) {
            // std::cout << "Decided: " << round <<
            // std::endl; std::string result = "Round " +
            // std::to_string(round) + " set: ";

            output_file->write(decided_set + "\n");
        }));

    std::thread([config]() {
        for (uint64_t i = 0; i < config.round_count; i++) {
            if (!agreement->propose(i + 1, config.proposals[i])) {
                break;
            }
        }
        std::cout << "FINISHED SENDING THREAD" << std::endl;
    }).detach();

    // After a process finishes broadcasting,
    // it waits forever for the delivery of messages.
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}
