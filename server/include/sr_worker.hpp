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

class ServerWorker{
 public:
 	ServerWorker(FileRequest request, int maxWindowSize, int socket, struct sockaddr_in dest_addr, double loss_probability);
 	// function to read acks, increment cwnd, check in range l to r and minimize cwnd with max size
    void sendSRFile();
    void sendGoBackNFile();
  private:
  	int cwnd, maxWindowSize;
  	int read_pipe_;
  	std::vector<DataPacket*> packets;
  	std::vector<bool> ack;
  	std::vector<clock_t> timer;
  	const int TIMEOUT = 2; // in sec
  	int socket_descriptor;
  	struct sockaddr_in dest_addr;
	double loss_probability_;

  	bool checkTimeouts(int l, int r);
  	int create_socket();
};

#endif /* ifndef SR_WORKER_HEADER */
