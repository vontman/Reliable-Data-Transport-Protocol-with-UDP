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
 	SR_Worker(FileRequest* request, int maxWindowSize);
 	// function to read acks
  private:
  	int maxWindowSize;
  	std::vector<Packet*> packets;
  	std::vector<bool> ack;
  	std::vector<clock_t> timer;
  	const int TIMEOUT = 2; // in sec

  	void sendFileLen(int len);
  	void sendFile();
  	void checkTimeouts(int l, int r);
};

#endif /* ifndef SR_WORKER_HEADER */
