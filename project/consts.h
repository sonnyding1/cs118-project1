#pragma once

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Maximum payload size
#define MAX_PAYLOAD 1016

// Retransmission time
#define TV_DIFF(end, start)                                                    \
    (end.tv_sec * 1000000) - (start.tv_sec * 1000000) + end.tv_usec -          \
        start.tv_usec
#define RTO 1000000

// Window size
#define MAX_WINDOW 20 
#define DUP_NACKS 3

// Diagnostic messages
#define RECV 0
#define SEND 1
#define RTOD 2
#define DUPA 3

// Type of program
#define CLIENT 0
#define SERVER 1

// Structs
typedef struct {
    uint16_t seq;
    uint16_t length;
    uint16_t unused;
    int8_t nack;
    uint8_t flags; // LSb 0 Parity, LSb 1 NACK
    uint8_t payload[0];
} packet;

// Helpers
static inline void print(char* txt) {
    fprintf(stderr, "%s\n", txt);
}

static inline void print_diag(packet* pkt, int diag) {
    switch (diag) {
    case RECV:
        fprintf(stderr, "RECV");
        break;
    case SEND:
        fprintf(stderr, "SEND");
        break;
    case RTOD:
        fprintf(stderr, "RTOS");
        break;
    case DUPA:
        fprintf(stderr, "DUPS");
        break;
    }

    bool syn = pkt->flags & 0b01;
    bool nack = pkt->flags & 0b10;
    fprintf(stderr, " %u NACK %hhd SIZE %hu FLAGS ", ntohl(pkt->seq),
            ntohl(pkt->nack), ntohs(pkt->length));
    if (!syn && !nack) {
        fprintf(stderr, "NONE");
    } else {
        if (syn) {
            fprintf(stderr, "PARITY ");
        }
        if (nack) {
            fprintf(stderr, "NACK ");
        }
    }
    fprintf(stderr, "\n");
}


