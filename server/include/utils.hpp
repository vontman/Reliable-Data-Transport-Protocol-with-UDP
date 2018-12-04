#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <vector>
#include "../../messages/include/messages.hpp"
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>

class Utils {
  public:
    static const int SOCKET_ERROR = -1;

    static Packet* createPacket(uint16_t len, uint16_t seqno, char *data);
    static std::vector<Packet*> divideFileIntoPackets(std::string fileName);
    static int sendDataPacket(int socket, struct Packet *dataPacket, struct sockaddr_in dest_addr);
    static int sendFileRequestPacket(int socket, struct FileRequest *requestPacket, struct sockaddr_in dest_addr);
};

#endif /* ifndef UTILS_HEADER */
