#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <messages.hpp>
#include <string>
#define BUFLEN 500

int main(int argc, char **argv) {
    char const * const address = argv[1];
    int const port = atoi(argv[2]);
    char const * const filename(argv[3]);
    int const max_window_size = atoi(argv[4]);

    struct sockaddr_in si_other;
    int s, i, slen = sizeof(si_other);
    char buf[BUFLEN];

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        perror("socket"), exit(1);

    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(port);
    if (inet_aton(address, &si_other.sin_addr) == -1) {
        perror("inet_aton() failed\n");
        exit(1);
    }


    DataPacket p;
    p.chksum = 1;
    p.len = 2;
    p.seqno = 3;
    strcpy(p.data, filename);
    printf("Sending packet %d\n", i);
    sprintf(buf, "This is packet %d filename: %s\n", i, filename);
    if (sendto(s, &p, sizeof p, 0, (sockaddr *)&si_other, slen) == -1)
        perror("sendto()"), exit(1);
    AckPacket a1, a2;
    a1.ackno = 11111;
    a2.ackno = 22222;
    if (sendto(s, &a1, sizeof a1, 0, (sockaddr *)&si_other, slen) == -1)
        perror("sendto()"), exit(1);
    if (sendto(s, &a2, sizeof a2, 0, (sockaddr *)&si_other, slen) == -1)
        perror("sendto()"), exit(1);

    close(s);
    return 0;
}
