#include "../include/utils.hpp"

struct Packet* Utils::createPacket(uint16_t len, uint16_t seqno, char *data){
    struct Packet* dataPacket = (struct Packet*) malloc(sizeof(struct Packet));
    if(dataPacket == NULL){
        std::cerr <<"Failed to allocate memory for new data packet.";
        return NULL;
    }

    dataPacket->len = len;
    dataPacket->seqno = seqno;
    memset(&dataPacket->data, 0, PACKET_LEN);
    if(data != NULL)
        memcpy(&dataPacket->data, data, len);
    return dataPacket;
}

std::vector<Packet*> Utils::divideFileIntoPackets(std::string fileName){
	std::ifstream fin(fileName);
	std::stringstream buffer;
	buffer <<fin.rdbuf();
	char* data = (char *) buffer.str().c_str();

	std::vector<Packet*> ret;
	int n = strlen(data);
	for(int i = 0; i < n; i += PACKET_LEN)
		ret.push_back(createPacket(std::min(n - i, PACKET_LEN), i / PACKET_LEN, data + i));
	return ret;
}

int Utils::sendDataPacket(int socket, struct Packet *dataPacket, struct sockaddr_in &server){
	int counter = 0;
    char *sending_pointer = (char*)dataPacket;
    int len = sizeof(struct Packet);
    while (len > 0)
    {
        int amount = sendto(socket,(const char*)sending_pointer, len ,0, (struct sockaddr*)&server, sizeof(struct sockaddr_in));
        if (amount == SOCKET_ERROR)
        {
            // handle error ...
            puts("Send failed");
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