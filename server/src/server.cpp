#include "../include/server.hpp"
#include "../../messages/include/messages.hpp"

#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

void error(std::string const &error_msg) {
    perror(error_msg.c_str());
    exit(1);
}

int send_with_packet_loss(double prob_of_drop, int socket,
                          sockaddr_in &client_address, char const *data,
                          size_t len) {
    double r = ((double)rand() / (RAND_MAX));
    if (r < prob_of_drop)
        return len;

    return sendto(socket, data, len, 0, (sockaddr *)&client_address,
                  sizeof client_address);
}

Server::Server(int port, int max_window_size)
    : d_port(port), d_max_window_size(max_window_size) {

    std::cout << "Server(" << port << ", " << max_window_size << ")\n";

    // socket address used for the server
    sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(d_port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    d_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (d_socket == -1) {
        error("Couldn't create UDF socket for port:" + std::to_string(d_port));
    }

    if (bind(d_socket, (sockaddr *)&server_address, sizeof(server_address)) !=
        0) {
        error("could not bind UDP socket with: port:" + std::to_string(d_port));
    }
}

Server::~Server() {
    std::cout << "~Server()\n";
    stop();
}

void Server::stop() {
    if (!d_is_running)
        return;
    d_is_running = false;
    close(d_socket);
}

void Server::start() {
    if (d_is_running)
        return;
    d_is_running = true;

    char buffer[1000];
    size_t bytes_received;
    sockaddr_in client_address;
    socklen_t client_address_len;

    while (d_is_running) {
        bytes_received = recvfrom(d_socket, buffer, 1000, 0, (sockaddr *)&client_address,
                     &client_address_len);
        if (bytes_received == -1)
            error("Error trying to receive data from " +
                  std::string(inet_ntoa(client_address.sin_addr)) + ": " +
                  std::to_string(ntohs(client_address.sin_port)) +
                  "\nData: " + std::string(buffer));

        std::cout << "Received packet from "
                  << inet_ntoa(client_address.sin_addr) << ":"
                  << ntohs(client_address.sin_port) << "\n";
        std::cout << "DataLen: " << bytes_received << "\n";
        std::cout << "Data: " << buffer << "\n" << std::flush;
        Packet p;
        memcpy(&p, buffer, sizeof(p));
        std::cout << "Packet p(" << p.chksum << ", " << p.len << ", " << p.seqno << ", " << p.data << ")\n" << std::flush;
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        perror("The server should be executed ./server PORT MAX_WINDOW_SIZE");
        exit(1);
    }
    int port = atoi(argv[1]);
    int max_window_size = atoi(argv[2]);
    Server s(port, max_window_size);
    s.start();

    return 0;
}
