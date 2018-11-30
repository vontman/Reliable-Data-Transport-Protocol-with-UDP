#ifndef MESSAGES_HEADER
#define MESSAGES_HEADER

const int PACKET_LEN = 500;

#include <stdint.h>

struct FileRequest {
    uint16_t len;
    char file_name[100];
};

struct Packet {
    uint16_t chksum;
    uint16_t len;
    uint32_t seqno;
    char data[PACKET_LEN];
};

struct AckPacket {
    uint16_t chksum;
    uint16_t len;
    uint32_t ackno;
};

#endif /* ifndef MESSAGES_HEADER */
