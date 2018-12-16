#include "../include/utils.hpp"

struct DataPacket *Utils::createPacket(int len, int seqno, std::string &data, int index) {
    struct DataPacket *dataPacket = (struct DataPacket *) malloc(sizeof(struct DataPacket));
    if (dataPacket == NULL) {
        std::cerr << "Failed to allocate memory for new data packet." << std::endl;
        return NULL;
    }

    dataPacket->len = len;
    dataPacket->seqno = seqno;
    memcpy(dataPacket->data, data.substr(index, len).c_str(), len);
    dataPacket->chksum = dataPacket->calc_chksum();
    return dataPacket;
}

std::vector<DataPacket *> Utils::divideFileIntoPackets(std::string fileName) {
    std::ifstream fin(fileName);
    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::string data = buffer.str();
    std::vector<DataPacket *> ret;
    for (int i = 0; i < data.length(); i += PACKET_LEN) {
        ret.push_back(createPacket(std::min((int) (data.length() - i), PACKET_LEN), i / PACKET_LEN, data, i));
    }

    DataPacket *lastPacket = ret[ret.size() - 1];
    if (lastPacket->len == sizeof(lastPacket->data)) {
        DataPacket *emptyPacket = (struct DataPacket *) malloc(sizeof(struct DataPacket));
        emptyPacket->len = 0;
        emptyPacket->seqno = lastPacket->seqno + 1;
        emptyPacket->chksum = emptyPacket->calc_chksum();
        ret.push_back(emptyPacket);
    }
    fin.close();
    return ret;
}

int Utils::sendDataPacket(int socket, struct DataPacket *dataPacket, struct sockaddr_in dest_addr,
                          double loss_probability) {
    int counter = 0;
    char *sending_pointer = (char *) dataPacket;
    int len = sizeof(struct DataPacket);
    int amount = send_with_packet_loss(loss_probability, socket, dest_addr, sending_pointer, len);
    if (amount == SOCKET_ERROR) {
        std::cerr << "Sending failed in Utils::sendDataPacket function.";
        return SOCKET_ERROR;
    }
    return counter;
}

ssize_t Utils::send_with_packet_loss(double prob_of_drop, int socket, sockaddr_in &client_address, char const *data,
                                     size_t len) {
    double r = ((double) rand() / (RAND_MAX));
    if (r < prob_of_drop) {
        return len;
    }

    return sendto(socket, data, len, 0, (sockaddr *) &client_address, sizeof client_address);
}
