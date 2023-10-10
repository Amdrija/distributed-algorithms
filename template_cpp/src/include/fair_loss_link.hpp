#pragma once

#include "message.hpp"
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
    sockaddr_in address;
    uint32_t ip_address;
    uint16_t port;

public:
    FairLossLink(const std::string &ip_address, uint16_t port) : FairLossLink(ntohl(inet_addr(ip_address.c_str())), port) {
    }

    FairLossLink(const uint32_t ip_address, uint16_t port) {
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            std::cout << "Error while opening socket on address: " << ip_address << ":" << port << std::endl;
            exit(1);
        }

        create_socket_address(ip_address, port, &address);

        if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
            std::cout << "Error while binding the socket on address: " << ip_address << ":" << port << std::endl;
            exit(1);
        }

        this->ip_address = ip_address;
    }

    void send(Message &message) {
        sockaddr_in address;
        create_socket_address(message.ip, message.port, &address);

        uint64_t serialized_length = 0;
        std::shared_ptr<char[]> payload = message.transport_message.serialize(serialized_length);
        sendto(socket_fd, payload.get(), serialized_length, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    }

    void start_receiving(std::function<void(Message)> handler) {
        char buffer[MAX_BUFFER_SIZE];
        sockaddr_in source;
        socklen_t source_length = sizeof(source);

        auto received_length = recvfrom(socket_fd, buffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr *>(&source), &source_length);
        while (received_length >= 0) {
            for (int i = 0; i < 8; i++) {
                std::cout << buffer[i] + 1 << "|";
            }
            std::cout << "\nReceived: " << buffer[0] << " " << received_length << std::endl;
            TransportMessage message = TransportMessage::deserialize(buffer, received_length);

            handler(Message(ntohl(source.sin_addr.s_addr), ntohs(source.sin_port), message));

            received_length = recvfrom(socket_fd, buffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr *>(&source), &source_length);
        }

        std::cout << "Stopped receiveing on address: " << this->ip_address << ":" << this->port << std::endl;
    }

private:
    void create_socket_address(const uint32_t ip_address, uint16_t port, sockaddr_in *address) {
        memset(reinterpret_cast<void *>(address), 0, sizeof(address));
        address->sin_family = AF_INET;
        address->sin_addr.s_addr = htonl(ip_address);
        address->sin_port = htons(port);
    }

    void create_socket_address(const std::string &ip_address, uint16_t port, sockaddr_in *address) {
        create_socket_address(inet_addr(ip_address.c_str()), port, address);
    }
};