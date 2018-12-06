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

class SR_Worker{
 public:
 	SR_Worker(FileRequest request, int maxWindowSize, int socket, struct sockaddr_in dest_addr);
 	// function to read acks, increment cwnd, check in range l to r and minimize cwnd with max size
  private:
  	int cwnd, maxWindowSize;
  	int read_pipe_;
  	std::vector<DataPacket*> packets;
  	std::vector<bool> ack;
  	std::vector<clock_t> timer;
  	const int TIMEOUT = 2; // in sec
  	int socket_descriptor;
  	struct sockaddr_in dest_addr;

  	void sendFileLen(int len);
  	void sendFile();
  	bool checkTimeouts(int l, int r);
  	int create_socket();
};

#endif /* ifndef SR_WORKER_HEADER */
