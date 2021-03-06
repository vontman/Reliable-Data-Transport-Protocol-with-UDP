#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <map>
#include <messages.hpp>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h> // for gettimeofday()
#include <sys/types.h>
#include <unistd.h>

const char *CONFIGURATION_FILE_NAME = "client.in";

const std::string GO_BACK_N_METHOD = "go-back-n";
const std::string SELECTIVE_REPEAT_METHOD = "selective-repeat";
const std::string STOP_AND_WAIT_METHOD = "stop-and-wait";

int last_window_base = 0;

void create_socket(int &socket_descriptor, struct sockaddr_in &si_other,
                   std::string &server_address, int server_port,
                   int client_port);
void request_file(std::string file_name, int socket_descriptor,
                  struct sockaddr_in si_other);
void receive_file_using_selective_repeat(std::ofstream &output_file,
                                         int socket_descriptor,
                                         struct sockaddr_in si_other,
                                         int window_size);
void receive_file_using_go_back(std::ofstream &output_file,
                                int socket_descriptor,
                                struct sockaddr_in si_other);

void read_client_configuration(std::string &server_address, int &server_port,
                               int &client_port, std::string &file_name,
                               int &window_size, std::string &method) {
    std::ifstream config_file(CONFIGURATION_FILE_NAME);
    config_file >> server_address >> server_port >> client_port >> file_name >>
        window_size >> method;
    config_file.close();
    if (method == STOP_AND_WAIT_METHOD) {
        window_size = 1;
        std::cout
            << "Window size is defaulted to 1 because of using Stop And Wait"
            << std::endl;
    }
    std::cout << "Configuration read: " << std::endl;
    std::cout << "Server Address: " << server_address << "\n"
              << "Server Port: " << server_port << "\n"
              << "Client Port: " << client_port << "\n"
              << "File Name: " << file_name << "\n"
              << "Window Size: " << window_size << "\n"
              << "Method: " << method << std::endl;
}

int main(int argc, char **argv) {
    int server_port;
    int client_port;
    std::string file_name;
    std::string server_address;
    int window_size;
    std::string method;
    read_client_configuration(server_address, server_port, client_port,
                              file_name, window_size, method);

    struct timeval t1, t2;
    int socket;
    struct sockaddr_in si_other;
    create_socket(socket, si_other, server_address, server_port, client_port);
    request_file(file_name, socket, si_other);
    std::ofstream output_file(file_name);
    gettimeofday(&t1, NULL);
    if (method == GO_BACK_N_METHOD) {
        receive_file_using_go_back(output_file, socket, si_other);
    } else {
        receive_file_using_selective_repeat(output_file, socket, si_other,
                                            window_size);
    }
    std::cout << "Finished receiving successfully" << std::endl;
    gettimeofday(&t2, NULL);
    double elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0; // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;     // us to ms
    std::cout << "Total time required to send is " << elapsedTime / 1000.0
              << " seconds" << std::endl;

    return 0;
}

void create_socket(int &socket_descriptor, struct sockaddr_in &si_other,
                   std::string &server_address, int server_port,
                   int client_port) {
    if ((socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("Couldn't create socket");
        exit(1);
    }
    struct sockaddr_in myaddr;
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(client_port);

    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(server_port);
    if (inet_aton(server_address.c_str(), &si_other.sin_addr) == -1) {
        perror("inet_aton() failed\n");
        exit(1);
    }

    if (bind(socket_descriptor, (struct sockaddr *)&myaddr, sizeof(myaddr)) <
        0) {
        perror("bind failed");
        exit(1);
    }
}

void request_file(std::string file_name, int socket_descriptor,
                  struct sockaddr_in si_other) {
    size_t s_len = sizeof(si_other);

    FileRequest p;
    p.len = file_name.length();
    strcpy(p.file_name, file_name.c_str());
    p.chksum = p.calc_chksum();
    if (sendto(socket_descriptor, &p, sizeof p, 0, (sockaddr *)&si_other,
               s_len) == -1) {
        perror("Couldn't request file from server, exiting");
        exit(1);
    }
}

void report(int window_base, int enforce) {
    if (enforce || (window_base - last_window_base >= 300 &&
                    window_base != last_window_base)) {
        last_window_base = window_base;
        std::cout << "Reached the packet with number: " << window_base
                  << std::endl;
    }
}

void receive_file_using_selective_repeat(std::ofstream &output_file,
                                         int socket_descriptor,
                                         struct sockaddr_in si_other,
                                         int window_size) {
    size_t addr_len = sizeof(si_other);
    std::map<int, DataPacket> id_to_packet_map;

    bool done_receiving = false;
    size_t buffer_size = sizeof(DataPacket);
    char buffer[buffer_size];

    int window_base = 0;
    while (!done_receiving) {
        report(window_base, false);
        ssize_t bytes_received =
            recv(socket_descriptor, buffer, buffer_size, 0);
        if (bytes_received == -1) {
            perror("Failed to receive data");
            continue;
        }
        DataPacket packet;
        memcpy(&packet, buffer, sizeof(packet));
        //std::cout << "Received DataPacket(Received Chksum:" << packet.chksum
                  //<< ", Calculated Chksum" << packet.calc_chksum() << ")\n"
                  //<< std::flush;

        if (packet.chksum != packet.calc_chksum()) {
            std::cout << "Received an invalid packet, ignoring.\n"
                      << std::flush;
            continue;
        }

        int packetNumber = packet.seqno;
        if (packetNumber >= window_base - window_size &&
            packetNumber < window_base + window_size) {
            AckPacket response;
            response.len = packet.len;
            response.ackno = packet.seqno;
            response.chksum = response.calc_chksum();
            sendto(socket_descriptor, &response, sizeof response, 0,
                   (sockaddr *)&si_other, addr_len);

            // If it's a new packet, append it to the map
            if (packetNumber >= window_base) {
                id_to_packet_map[packetNumber] = packet;
            }

            // Empty the map
            while (id_to_packet_map.size() != 0 &&
                   id_to_packet_map.begin()->first == window_base) {
                DataPacket to_be_written_packet = id_to_packet_map[window_base];
                output_file.write(to_be_written_packet.data,
                                  to_be_written_packet.len);
                if (to_be_written_packet.len !=
                    sizeof(to_be_written_packet.data)) {
                    done_receiving = true;
                }
                window_base++;
                id_to_packet_map.erase(window_base - 1);
            }
        }
    }
    report(window_base, true);
    close(socket_descriptor);
    output_file.close();
}

void receive_file_using_go_back(std::ofstream &output_file,
                                int socket_descriptor,
                                struct sockaddr_in si_other) {
    socklen_t addr_len = sizeof(si_other);

    bool done_receiving = false;
    size_t buffer_size = sizeof(DataPacket);
    char buffer[buffer_size];

    int window_base = 0;
    while (!done_receiving) {
        report(window_base, false);
        ssize_t bytes_received =
            recv(socket_descriptor, buffer, buffer_size, 0);
        if (bytes_received == -1) {
            perror("Failed to receive data");
            continue;
        }
        DataPacket packet;
        memcpy(&packet, buffer, sizeof(packet));
        std::cout << "Received DataPacket(Chksum:" << packet.chksum << ", "
                  << packet.calc_chksum() << ")\n"
                  << std::flush;

        int packetNumber = packet.seqno;
        AckPacket response;
        response.len = packet.len;
        if (window_base == packetNumber) {
            response.ackno = packet.seqno;
            output_file.write(packet.data, packet.len);
            window_base++;
        } else {
            response.ackno = window_base - 1;
        }
        response.chksum = response.calc_chksum();
        sendto(socket_descriptor, &response, sizeof response, 0,
               (sockaddr *)&si_other, addr_len);
        if (packet.len != sizeof(packet.data)) {
            done_receiving = true;
        }
    }
    report(window_base, true);
    close(socket_descriptor);
    output_file.close();
}
