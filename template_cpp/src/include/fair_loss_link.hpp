#pragma once
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <functional>
#include <string>
#include <sstream>

// technically, the actual size is lower, but a few bytes should not make an impact
#define MAX_BUFFER_SIZE 65535

class FairLossLink {
private:
    int socket_fd;
    sockaddr_in address;
    std::string ip_address;
    unsigned short port;
public:
    FairLossLink(const std::string& ip_address, unsigned short port) {
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            std::cout << "Error while opening socket on address: " << ip_address << ":" << port << std::endl;
            exit(1);
        }

        create_socket_address(ip_address, port, &address);

        if (bind(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
            std::cout << "Error while binding the socket on address: " << ip_address << ":" << port << std::endl;
            exit(1);
        }

        this->ip_address = ip_address;
    }

    ssize_t send(const std::string& destination_address, unsigned short desination_port, const std::string& message) {
        sockaddr_in destination;
        create_socket_address(destination_address, desination_port, &destination);

        return sendto(socket_fd, message.c_str(), message.length(), 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));
    }

    void start_receiving(std::function<void(const uint32_t, const uint16_t, const std::string)> handler) {
        char buffer[MAX_BUFFER_SIZE];
        sockaddr_in source;
        socklen_t source_length = sizeof(source);

        auto received_length = recvfrom(socket_fd, buffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&source), &source_length);
        while(received_length >= 0) {
            std::string message = std::string(buffer, received_length);

            handler(source.sin_addr.s_addr, source.sin_port, message);
    
            received_length = recvfrom(socket_fd, buffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&source), &source_length);
        }

        std::cout << "Stopped receiveing on address: " << this->ip_address << ":" << this->port << std::endl;
    }

private:
    void create_socket_address(const std::string& ip_address, unsigned short port, sockaddr_in* address) {
        memset(reinterpret_cast<void*>(address), 0, sizeof(address));
        address->sin_family = AF_INET;
        address->sin_addr.s_addr = inet_addr(ip_address.c_str());
        address->sin_port = htons(port);
    }
};