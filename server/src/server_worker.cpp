#include "server_worker.hpp"
#include "../include/utils.hpp"

#include <unistd.h>
#include <algorithm>
#include <fstream>

double ServerWorker::TIMEOUT = 0.3;

const double ServerWorker::CONNECTION_TIMEOUT = 2;
const std::string ServerWorker::CONGESTION_FILE = "congestion.in";

ServerWorker::ServerWorker(FileRequest request, int maxWindowSize, int pipe, struct sockaddr_in dest_addr,
                           double loss_probability)
        : loss_probability_(loss_probability) {
    this->maxWindowSize = maxWindowSize;
    this->packets = Utils::divideFileIntoPackets(std::string(request.file_name));
    this->ack = std::vector<bool>(maxWindowSize);
    this->timer = std::vector<clock_t>(maxWindowSize);
    this->socket_descriptor = create_socket();
    this->read_pipe_ = pipe;
    this->dest_addr = dest_addr;
    this->cwnd = 1;
    this->file_name_ = request.file_name;
}


int ServerWorker::create_socket() {
    std::string const client_address_str =
            std::string(inet_ntoa(dest_addr.sin_addr)) + ":" +
            std::to_string(ntohs(dest_addr.sin_port));
    int socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_descriptor == -1) {
        perror("Couldn't create socket");
        exit(1);
    }
    if (inet_aton(client_address_str.c_str(), &dest_addr.sin_addr) == -1) {
        perror("inet_aton() failed\n");
        exit(1);
    }

    if (bind(socket_descriptor, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }
    std::cout << "Created a UDP socket for the client successfully" << std::endl;
    return socket_descriptor;
}

void ServerWorker::sendSRFile() {
    size_t packets_count = packets.size();
    int packet_window_start = 0, packet_window_end = 0;
    std::cout << "Sending (" << file_name_ << ") Using SR with window " << maxWindowSize << " divided into "
              << packets_count << " packets" << std::endl;
    double start_time = clock();
    std::vector<int> congestion_packets = read_congestion_packets();
    int congestion_packet_ptr = 0;
    clock_t last_response_time = clock();
    std::ofstream window_size_recorder;
    window_size_recorder.open ("selective_repeat_windows_" + file_name_ + ".txt");
    while (packet_window_start < packets_count) {
        if (packet_window_end - packet_window_start < cwnd and packet_window_end < packets_count) {
            window_size_recorder << packet_window_end - packet_window_start+1<<"\n";
            Utils::sendDataPacket(socket_descriptor, packets[packet_window_end], dest_addr, loss_probability_);
            ack[packet_window_end % maxWindowSize] = false;
            timer[packet_window_end % maxWindowSize] = clock();
            ++packet_window_end;
        } else if (ack[packet_window_start % maxWindowSize]) {
            last_response_time = clock();
            ++packet_window_start;
            if(congestion_packet_ptr < congestion_packets.size() && congestion_packets[congestion_packet_ptr] == packet_window_start) {
                cwnd = (cwnd + 1) / 2;
                packet_window_end = packet_window_start + cwnd;
                std::cout << "cwnd cut to half because of Congestion at packet " << congestion_packets[congestion_packet_ptr]
                          << " and new value = " << cwnd << std::endl;
                congestion_packet_ptr++;
            }
        }
        else {
            AckPacket ack_packet;
            while ((read(read_pipe_, &ack_packet, sizeof(struct AckPacket))) != -1) {
                int acked_packet_number = ack_packet.ackno;
                if (acked_packet_number < packet_window_end && acked_packet_number >= packet_window_start) {
                    ack[acked_packet_number % maxWindowSize] = true;
                    cwnd = std::min(cwnd + 1, maxWindowSize);
                }
            }
            if (checkTimeouts(packet_window_start, packet_window_end)) {
                std::cout << "Timeout had occurred at window (" <<  packet_window_start << ", " << packet_window_end
                        << ") and new cwnd = 1" << std::endl;
                if(double(clock() - last_response_time) / CLOCKS_PER_SEC >= CONNECTION_TIMEOUT) {
                    std::cout << "Server is cutting the connection due to timing out since last response" << std::endl;
                    break;
                }
                cwnd = 1;
                packet_window_end = packet_window_start + 1;
            }
        }
    }
    close(socket_descriptor);
    double totalTime = (clock() - start_time) / CLOCKS_PER_SEC;
    std::cout << "Total time required to send (" << file_name_ << ") is " << totalTime << " seconds" << std::endl;
}

std::vector<int> ServerWorker::read_congestion_packets() {
    std::vector<int> result_set;
    std::ifstream congestion_file(CONGESTION_FILE);
    int packet_number;
    while(congestion_file >> packet_number) {
        result_set.push_back(packet_number);
    }
    congestion_file.close();
    std::sort(result_set.begin(), result_set.end());
    return result_set;
}

void ServerWorker::sendGoBackNFile() {
    int n = packets.size();
    int l = 0, r = 0;
    std::cout << "Attempting to send (" << file_name_ << ") using Go Back N divided into " << n << " packets"
              << std::endl;
    double start_time = clock();
    std::cout << packets[10458]->len << std::endl;
    while (l < n) {
        if (r - l < maxWindowSize and r < n) {
            Utils::sendDataPacket(socket_descriptor, packets[r], dest_addr, loss_probability_);
            ack[r % maxWindowSize] = false;
            timer[r % maxWindowSize] = clock();
            ++r;
        } else if (ack[l % maxWindowSize] && l <= r) ++l;
        else {
            AckPacket ack_packet;
            while ((read(read_pipe_, &ack_packet, sizeof(struct AckPacket))) != -1) {
                int acked_packet_number = ack_packet.ackno;
                if (acked_packet_number < r && acked_packet_number >= l) {
                    ack[acked_packet_number % maxWindowSize] = true;
                }
            }
        }
        if (clock() - timer[l % maxWindowSize] > 1.0 * CLOCKS_PER_SEC * TIMEOUT) {
            for (int retry_index = l; retry_index < r; retry_index++) {
                Utils::sendDataPacket(socket_descriptor, packets[retry_index], dest_addr, loss_probability_);
                timer[retry_index % maxWindowSize] = clock();
            }
        }
    }
    close(socket_descriptor);
    double totalTime = (clock() - start_time) / CLOCKS_PER_SEC;
    std::cerr << "Total time required to send (" << file_name_ << ") is " << totalTime << " seconds" << std::endl;
}

bool ServerWorker::checkTimeouts(int l, int r) {
    for (int i = l; i < r; ++i) {
        if (clock() - timer[i % maxWindowSize] > 1L * CLOCKS_PER_SEC * TIMEOUT){
            Utils::sendDataPacket(socket_descriptor, packets[i], dest_addr, loss_probability_);
            timer[i % maxWindowSize] = clock();
            return true;
        }
    }
    return false;
}
