#include "../include/sr_worker.hpp"
#include "../include/utils.hpp"

#include <unistd.h>

ServerWorker::ServerWorker(FileRequest request, int maxWindowSize, int pipe, struct sockaddr_in dest_addr) {
	this->maxWindowSize = maxWindowSize;
	this->packets = Utils::divideFileIntoPackets(std::string(request.file_name));
	this->ack = std::vector<bool>(maxWindowSize);
	this->timer = std::vector<clock_t>(maxWindowSize);
	this->socket_descriptor = create_socket();
	this->read_pipe_ = pipe;
	this->dest_addr = dest_addr;
	this->cwnd = 1;

	// Send file length to client
	int len = 0;
	for(DataPacket* packet : packets)
		len += packet->len;
	//sendFileLen(len);
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

	if (bind(socket_descriptor, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		perror("bind failed");
		exit(1);
	}
	std::cout << "Created a UDP socket for the client successfully" << std::endl;
	return socket_descriptor;
}

void ServerWorker::sendFileLen(int len){
	FileRequest *reply = (FileRequest *) malloc(sizeof(struct FileRequest));
	if(reply == NULL){
		std::cerr <<"Failed to allocate memory for reply packet.";
		return;
	}
	reply->len = len;

	while(Utils::sendFileRequestPacket(socket_descriptor, reply, dest_addr) == Utils::SOCKET_ERROR)
		std::cerr <<"Failed to send file length to client, retrying.";
}

void ServerWorker::sendSRFile(){
	int n = packets.size();
	int l = 0, r = 0;
	std::cout << "Attempting to send the file divided into " << n << " packets" << std::endl;
	while(l < n){
		if(r - l < cwnd and r < n){
		    Utils::sendDataPacket(socket_descriptor, packets[r], dest_addr);
			ack[r % maxWindowSize] = false;
			timer[r % maxWindowSize] = clock();
			++r;
		}
		else if(ack[l % maxWindowSize]) ++l;
		else{
            AckPacket ack_packet;
            while((read(read_pipe_, &ack_packet, sizeof(struct AckPacket))) != -1) {
                int acked_packet_number = ack_packet.ackno;
                if(acked_packet_number < r && acked_packet_number >= l) {
                    ack[acked_packet_number % maxWindowSize] = true;
                    cwnd = std::min(cwnd + 1, maxWindowSize);
                }
            }
			if(checkTimeouts(l, r)){
				cwnd = (cwnd + 1) / 2;
				r = l + cwnd;
			}
		}
	}
}

void ServerWorker::sendGoBackNFile() {
    int n = packets.size();
    int l = 0, r = 0;
    std::cout << "Attempting to send the file divided into " << n << " packets" << std::endl;
        while(l < n){
        if(r - l < maxWindowSize and r < n){
            Utils::sendDataPacket(socket_descriptor, packets[r], dest_addr);
            ack[r % maxWindowSize] = false;
            timer[r % maxWindowSize] = clock();
            ++r;
        }
        else if(ack[l % maxWindowSize]) ++l;
        else{
            AckPacket ack_packet;
            while((read(read_pipe_, &ack_packet, sizeof(struct AckPacket))) != -1) {
                int acked_packet_number = ack_packet.ackno;
                if(acked_packet_number < r && acked_packet_number >= l) {
                    ack[acked_packet_number % maxWindowSize] = true;
                    cwnd = std::min(cwnd + 1, maxWindowSize);
                }
            }
        }
        if(double(clock() - timer[l % maxWindowSize]) / CLOCKS_PER_SEC > TIMEOUT) {
            for(int retry_index = l; retry_index < r; retry_index++) {
                Utils::sendDataPacket(socket_descriptor, packets[retry_index], dest_addr);
                timer[retry_index % maxWindowSize] = clock();
            }
        }
    }
}

bool ServerWorker::checkTimeouts(int l, int r){
	for(int i = l; i < r; ++i)
		if(double(clock() - timer[i % maxWindowSize]) / CLOCKS_PER_SEC > TIMEOUT){
			Utils::sendDataPacket(socket_descriptor, packets[i], dest_addr);
			timer[i % maxWindowSize] = clock();
			return true;
		}
	return false;
}
