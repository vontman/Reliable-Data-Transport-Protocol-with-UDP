#include "../include/sr_worker.hpp"
#include "../include/utils.hpp"

SR_Worker::SR_Worker(FileRequest* request, int maxWindowSize, int socket, struct sockaddr_in dest_addr){
	this->maxWindowSize = maxWindowSize;
	this->packets = Utils::divideFileIntoPackets(std::string(request->file_name));
	this->ack = std::vector<bool>(maxWindowSize);
	this->timer = std::vector<clock_t>(maxWindowSize);
	this->socket = socket;
	this->dest_addr = dest_addr;

	// Send file length to client
	int len = 0;
	for(Packet* packet : packets)
		len += packet->len;
	sendFileLen(len);

	// Send file to client
	sendFile();
}

void SR_Worker::sendFileLen(int len){
	FileRequest *reply = (FileRequest *) malloc(sizeof(struct FileRequest));
	if(reply == NULL){
		std::cerr <<"Failed to allocate memory for reply packet.";
		return;
	}
	reply->len = len;

	while(Utils::sendFileRequestPacket(socket, reply, dest_addr) == Utils::SOCKET_ERROR)
		std::cerr <<"Failed to send file length to client, retrying.";
}

void SR_Worker::sendFile(){
	int n = packets.size();
	int l = 0, r = 0;
	while(l < n){
		if(r - l < maxWindowSize and r < n){
			Utils::sendDataPacket(socket, packets[r], dest_addr);
			ack[r % maxWindowSize] = false;
			timer[r % maxWindowSize] = clock();
			++r;
		}
		else if(ack[l % maxWindowSize]) ++l;
		else{
			// check new acks
			checkTimeouts(l, r);
		}
	}
}

void SR_Worker::checkTimeouts(int l, int r){
	for(int i = l; i < r; ++i)
		if(double(clock() - timer[i % maxWindowSize]) / CLOCKS_PER_SEC > TIMEOUT){
			Utils::sendDataPacket(socket, packets[i], dest_addr);
			timer[i % maxWindowSize] = clock();
		}
}
