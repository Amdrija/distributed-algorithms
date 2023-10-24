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

class FairLossLink
{
private:
    int socket_fd;
    bool continue_receiving = true;

public:
    const Address address;
    FairLossLink(Address address) : address(address)
    {
        this->socket_fd = create_socket();
        std::cout << "Initialized FairLoss link on address: " << address.to_string() << std::endl;
    }

    void send(TransportMessage message)
    {
        sockaddr_in address = message.address.to_sockaddr();

        uint64_t serialized_length = 0;
        std::unique_ptr<char[]> payload = message.serialize(serialized_length);
        sendto(this->socket_fd, payload.get(), serialized_length, 0,
               reinterpret_cast<sockaddr *>(&address), sizeof(address));
        // std::cout << "Sent message: " << message.get_id() << " to " <<
        // message.address.to_string()
        //           << std::endl;
    }

    void start_receiving(std::function<void(TransportMessage)> handler)
    {
        char buffer[MAX_BUFFER_SIZE];
        sockaddr_in source;
        socklen_t source_length = sizeof(source);

        auto received_length = recvfrom(this->socket_fd, buffer, MAX_BUFFER_SIZE, 0,
                                        reinterpret_cast<sockaddr *>(&source), &source_length);
        while (received_length >= 0)
        {
            handler(TransportMessage(Address(source), buffer, received_length));

            if (!this->continue_receiving)
            {
                break;
            }
            received_length = recvfrom(this->socket_fd, buffer, MAX_BUFFER_SIZE, 0,
                                       reinterpret_cast<sockaddr *>(&source), &source_length);
        }

        std::cout << "Stopped receiveing on address: " << this->address.to_string() << std::endl;
    }

    void stop_receiving()
    {
        this->continue_receiving = false;
        std::cout << "Stopping receiving on address: " << this->address.to_string() << std::endl;
    }

private:
    int create_socket()
    {
        const int opt = 1;
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0)
        {
            std::cout << "Error while opening socket on address: " << address.to_string()
                      << std::endl;
            exit(1);
        }

        auto sock_addr = this->address.to_sockaddr();
        if (bind(fd, reinterpret_cast<sockaddr *>(&sock_addr), sizeof(sock_addr)) == -1)
        {
            std::cout << "Error while binding the socket on address: " << address.to_string()
                      << std::endl;
            exit(1);
        }

        return fd;
    }
};