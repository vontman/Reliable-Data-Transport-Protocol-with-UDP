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

    static DataPacket* createPacket(uint16_t len, uint16_t seqno, std::string& data, int index);
    static std::vector<DataPacket*> divideFileIntoPackets(std::string fileName);
    static int sendDataPacket(int socket, struct DataPacket *dataPacket, struct sockaddr_in dest_addr, double loss_probability);
    static ssize_t send_with_packet_loss(double prob_of_drop, int socket, sockaddr_in &client_address, char const *data, size_t len);
};

#endif /* ifndef UTILS_HEADER */
