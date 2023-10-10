#pragma once

#include "address.hpp"
#include "transport_message.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>

// technically, the actual size is lower, but a few bytes should not make an impact
#define MAX_BUFFER_SIZE 65535

class FairLossLink {
private:
    int socket_fd;
    const Address address;

public:
    FairLossLink(Address address) : address(address) {
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            std::cout << "Error while opening socket on address: " << address.to_string() << std::endl;
            exit(1);
        }

        auto sock_addr = this->address.to_sockaddr();
        if (bind(socket_fd, reinterpret_cast<sockaddr *>(&sock_addr), sizeof(sock_addr)) == -1) {
            std::cout << "Error while binding the socket on address: " << address.to_string() << std::endl;
            exit(1);
        }
    }

    void send(TransportMessage &&message) {
        sockaddr_in address = message.address.to_sockaddr();

        uint64_t serialized_length = 0;
        std::unique_ptr<char[]> payload = message.serialize(serialized_length);
        sendto(socket_fd, payload.get(), serialized_length, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    }

    void start_receiving(std::function<void(TransportMessage)> handler) {
        char buffer[MAX_BUFFER_SIZE];
        sockaddr_in source;
        socklen_t source_length = sizeof(source);

        auto received_length = recvfrom(socket_fd, buffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr *>(&source), &source_length);
        while (received_length >= 0) {
            for (int i = 0; i < 8; i++) {
                std::cout << buffer[i] + 1 << "|";
            }
            std::cout << "\nReceived: " << buffer[0] << " " << received_length << std::endl;

            handler(TransportMessage(Address(source), buffer, received_length));

            received_length = recvfrom(socket_fd, buffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr *>(&source), &source_length);
        }

        std::cout << "Stopped receiveing on address: " << this->address.to_string() << std::endl;
    }
};