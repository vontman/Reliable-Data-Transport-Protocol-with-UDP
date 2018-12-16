#ifndef MESSAGES_HEADER
#define MESSAGES_HEADER
#include <stdint.h>

const int PACKET_LEN = 500;
const int FILE_NAME_LEN = 100;

struct FileRequest {
    uint16_t chksum;
    uint16_t len;
    char file_name[FILE_NAME_LEN];
    uint16_t calc_chksum() const {
        uint16_t curr_sum = len;
        uint16_t* file_name_ptr = (uint16_t*)file_name;
        for (int i = 0; i < FILE_NAME_LEN / 2; ++ i) {
            curr_sum += *file_name_ptr++;
        }
        return curr_sum;
    }
};

struct DataPacket {
    uint16_t chksum;
    uint16_t len;
    int32_t seqno;
    char data[PACKET_LEN];
    uint16_t calc_chksum() const {
        uint16_t curr_sum = len;
        curr_sum += seqno;
        uint16_t* data_ptr = (uint16_t*)data;
        for (int i = 0; i < PACKET_LEN / 2; ++ i) {
            curr_sum += *data_ptr++;
        }
        return curr_sum;
    }
};

struct AckPacket {
    uint16_t chksum;
    uint16_t len;
    int32_t ackno;
    uint16_t calc_chksum() const {
        return len + ackno;
    }
};

#endif /* ifndef MESSAGES_HEADER */
