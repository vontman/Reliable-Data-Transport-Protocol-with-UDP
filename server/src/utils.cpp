#include "../include/utils.hpp"

struct DataPacket* Utils::createPacket(uint16_t len, uint16_t seqno, std::string& data, int index){
    std::cout << "Entered" << std::endl;
    struct DataPacket* dataPacket = (struct DataPacket*) malloc(sizeof(struct DataPacket));
    if(dataPacket == NULL){
        std::cerr <<"Failed to allocate memory for new data packet." << std::endl;
        return NULL;
    }

    dataPacket->len = len;
    dataPacket->seqno = seqno;
    memcpy(dataPacket->data, data.substr(index, len).c_str(), len);
    return dataPacket;
}

std::vector<DataPacket*> Utils::divideFileIntoPackets(std::string fileName){
	std::ifstream fin(fileName);
	std::stringstream buffer;
	buffer <<fin.rdbuf();
	std::string data = buffer.str();
	std::vector<DataPacket*> ret;
	for(int i = 0; i < data.length(); i += PACKET_LEN)
		ret.push_back(createPacket(std::min((int)(data.length() - i), PACKET_LEN), i / PACKET_LEN, data, i));

	DataPacket* lastPacket = ret[ret.size() - 1];
	if(lastPacket->len == sizeof(lastPacket->data)) {
	    DataPacket* emptyPacket = (struct DataPacket*) malloc(sizeof(struct DataPacket));
	    emptyPacket->len = 0;
	    emptyPacket->seqno = lastPacket->seqno + 1;
	    ret.push_back(emptyPacket);
	}
	std::cout << "Quoi" << std::endl;
	return ret;
}

int Utils::sendDataPacket(int socket, struct DataPacket *dataPacket, struct sockaddr_in dest_addr){
	int counter = 0;
    char *sending_pointer = (char*)dataPacket;
    int len = sizeof(struct DataPacket);
    while (len > 0)
    {
        int amount = sendto(socket,(const char*)sending_pointer, len ,0, (struct sockaddr*) &dest_addr, sizeof(struct sockaddr_in));
        if (amount == SOCKET_ERROR)
        {
            std::cerr <<"Sending failed in Utils::sendDataPacket function.";
            return SOCKET_ERROR;
        }
        else
        {
            counter += amount;
            len -= amount;
            sending_pointer += amount;
        }
    }
    return counter;
}

int Utils::sendFileRequestPacket(int socket, struct FileRequest *requestPacket, struct sockaddr_in dest_addr){
	int counter = 0;
    char *sending_pointer = (char*)requestPacket;
    int len = sizeof(struct DataPacket);
    while (len > 0)
    {
        int amount = sendto(socket,(const char*)sending_pointer, len ,0, (struct sockaddr*) &dest_addr, sizeof(struct sockaddr_in));
        if (amount == SOCKET_ERROR)
        {
            std::cerr <<"Sending failed in Utils::sendFileRequestPacket function.";
            return SOCKET_ERROR;
        }
        else
        {
            counter += amount;
            len -= amount;
            sending_pointer += amount;
        }
    }
    return counter;
}