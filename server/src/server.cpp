#include <messages.hpp>
#include <server.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
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

    const size_t MAX_PACKET_SIZE =
        std::max(sizeof(AckPacket), sizeof(FileRequest));

    char buffer[MAX_PACKET_SIZE];
    size_t bytes_received;
    sockaddr_in client_address;
    socklen_t client_address_len = sizeof client_address;

    while (d_is_running) {
        bytes_received =
            recvfrom(d_socket, buffer, MAX_PACKET_SIZE, 0,
                     (sockaddr *)&client_address, &client_address_len);

        std::string const client_address_str =
            std::string(inet_ntoa(client_address.sin_addr)) + ":" +
            std::to_string(ntohs(client_address.sin_port));

        if (bytes_received == -1)
            error("Error trying to receive data from " + client_address_str +
                  "\nData: " + std::string(buffer));

        std::cout << "Received packet from " << client_address_str << "\n";
        std::cout << "DataLen: " << bytes_received << "\n";

        if (bytes_received == sizeof(FileRequest)) {
            FileRequest file_request;
            memcpy(&file_request, buffer, sizeof(file_request));
            handle_file_request(client_address, file_request);
        } else if (bytes_received == sizeof(AckPacket)) {
            AckPacket ack_packet;
            memcpy(&ack_packet, buffer, sizeof(ack_packet));
            handle_ack_packet(client_address, ack_packet);
        } else {
            std::cout << "Unknown package received!\n" << std::flush;
        }
    }
}

void Server::handle_file_request(sockaddr_in const &client_address,
                                 FileRequest const &file_request) {
    std::cout << "FileRequest p(" << file_request.chksum << ", "
              << file_request.len << ", " << file_request.file_name << ")\n"
              << std::flush;
    
    std::string const client_address_str =
        std::string(inet_ntoa(client_address.sin_addr)) + ":" +
        std::to_string(ntohs(client_address.sin_port));

    if (pipes_mapper.find(client_address_str) != pipes_mapper.end()) {
        std::cout << "Client has already made a request.\n" << std::flush;
        return;
    }

    std::cout << "Serving a new client with address: " << client_address_str
              << "\n"
              << std::flush;

    std::pair<int, int> pipes;

    if (pipe(&pipes.first) == -1)
        error("Error trying to pipe");

    std::cout << "Started new pipes" << std::endl;
    std::cout << pipes.first << ", " << pipes.second << std::endl;
    pipes_mapper[client_address_str] = pipes;

    pid_t pid = fork();
    if (pid < 0)
        error("Couldn't fork.");
    if (fcntl(pipes.first, F_SETFL, O_NONBLOCK))
        error("Error trying to set flags for pipes");

    if (pid == 0) {
        // CHILD
        child_handle_client(client_address, pipes, file_request);
        exit(0);
    }
}

void Server::child_handle_client(sockaddr_in const &client_address,
                                 std::pair<int, int> pipes,
                                 FileRequest const &file_request) {
    AckPacket ack_packet;
    for (size_t i = 0; i < 100; ++i) {
        close(pipes.second);
        int n = read(pipes.first, &ack_packet, sizeof ack_packet);
        if (n > 0)
            std::cout << "CHILD INCOMING AckPacket p(" << ack_packet.chksum
                      << ", " << ack_packet.len << ", " << ack_packet.ackno
                      << ")\n"
                      << std::flush;
        else if (n == -1 && errno == EAGAIN) {
            std::cout << "CHILD SLEEPING waiting on pipes:" << std::endl;
            std::cout << pipes.first << ", " << pipes.second << std::endl;
            sleep(1);
        } else if (n == -1)
            error("Error reading the pipe");
    }
}

void Server::handle_ack_packet(sockaddr_in const &client_address,
                               AckPacket const &ack_packet) {
    std::cout << "AckPacket p(" << ack_packet.chksum << ", " << ack_packet.len
              << ", " << ack_packet.ackno << ")\n"
              << std::flush;

    std::string const client_address_str =
        std::string(inet_ntoa(client_address.sin_addr)) + ":" +
        std::to_string(ntohs(client_address.sin_port));

    if (pipes_mapper.find(client_address_str) == pipes_mapper.end()) {
        std::cout << "Received an unexpected ack_packet.\n" << std::flush;
        return;
    }

    std::pair<int, int> pipes = pipes_mapper[client_address_str];
    std::cout << "Sent the ackpacke to the child waiting at pipes: " << std::endl;
    std::cout << pipes.first << ", " << pipes.second << std::endl;
    close(pipes.first);
    write(pipes.second, &ack_packet, sizeof ack_packet);
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
