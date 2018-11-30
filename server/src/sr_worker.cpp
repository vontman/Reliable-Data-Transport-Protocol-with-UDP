#include "../include/sr_worker.hpp"
#include "../include/utils.hpp"

SR_Worker::SR_Worker(FileRequest* request, int maxWindowSize){
	this->maxWindowSize = maxWindowSize;
	this->packets = Utils::divideFileIntoPackets(std::string(request->file_name));
	this->ack = std::vector<bool>(packets.size());
	this->timer = std::vector<clock_t>(packets.size());

	int len = 0;
	for(Packet* packet : packets)
		len += packet->len;
	sendFileLen(len);

	sendFile();
}

void SR_Worker::sendFileLen(int len){
	FileRequest *reply = (FileRequest *) malloc(sizeof(struct FileRequest));
	if(reply == NULL){
		std::cerr <<"Failed to allocate memory for reply packet.";
		return;
	}
	reply->len = len;

	// send reply
}

void SR_Worker::sendFile(){
	int n = packets.size();
	int l = 0, r = 0;
	while(l < n){
		if(r - l < maxWindowSize and r < n){
			// send rth
			ack[r] = false;
			timer[r] = clock();
			++r;
		}
		else if(ack[l]) ++l;
		else checkTimeouts(l, r);
	}
}

void SR_Worker::checkTimeouts(int l, int r){
	for(int i = l; i < r; ++i)
		if(double(clock() - timer[i]) / CLOCKS_PER_SEC > TIMEOUT){
			// resend
			timer[i] = clock();
		}
}
