#pragma once
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

class FairLossLink {
private:
    int socket_fd;
    sockaddr_in address;
public:
    FairLossLink(char* ip_address, unsigned short port) {
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
    }

    ssize_t send(char* destination_address, unsigned short desination_port, void* message, unsigned short message_length) {
        sockaddr_in destination;
        create_socket_address(destination_address, desination_port, &destination);

        return sendto(socket_fd, message, message_length, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));
    }

    ssize_t receive(void* message, unsigned short length, sockaddr* source, socklen_t* source_adress_length) {
        return recvfrom(socket_fd, message, length, 0, source, source_adress_length);
    }

private:
    void create_socket_address(char* ip_address, unsigned short port, sockaddr_in* address) {
        memset(reinterpret_cast<void*>(address), 0, sizeof(address));
        address->sin_family = AF_INET;
        address->sin_addr.s_addr = inet_addr(ip_address);
        address->sin_port = htons(port);
    }
};