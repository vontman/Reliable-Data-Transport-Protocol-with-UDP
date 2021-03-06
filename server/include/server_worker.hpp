#ifndef SR_WORKER_HEADER
#define SR_WORKER_HEADER

#include "../../messages/include/messages.hpp"
#include <vector>
#include <time.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>

class ServerWorker {
public:
    ServerWorker(FileRequest request, int maxWindowSize, int socket, struct sockaddr_in dest_addr,
                 double loss_probability);

    void sendSRFile();
    void sendGoBackNFile();

private:
    static double TIMEOUT; // in sec
    const static double CONNECTION_TIMEOUT;
    const static std::string CONGESTION_FILE;

    int cwnd, maxWindowSize;
    int read_pipe_;
    std::vector<DataPacket *> packets;
    std::vector<bool> ack;
    std::vector<clock_t> timer;
    int socket_descriptor;
    std::string file_name_;
    struct sockaddr_in dest_addr;
    double loss_probability_;

    bool checkTimeouts(int l, int r);
    int create_socket();
    std::vector<int> read_congestion_packets();
};

#endif /* ifndef SR_WORKER_HEADER */
