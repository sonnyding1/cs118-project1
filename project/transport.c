#include "consts.h"
#include "io.h"
#include "transport.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>>

#define MAX_PACKET (sizeof(packet) + MAX_PAYLOAD)
// 40 defined in MAX_WINDOW as max # of packets
#define MAX_BUFFER 40


uint16_t initial_seq_num()
{
    return (uint16_t)(rand() % 1000);
}

void set_parity(packet *pkt)
{
    int count = bit_count(pkt);
    uint16_t flags = pkt->flags;
    if (count % 2 != 0)
    {
        flags |= PARITY;
    }
    else
    {
        flags &= ~PARITY;
    }
    pkt->flags = flags;
}

typedef struct
{
    uint16_t seq_num;
    uint16_t received_ack;
} initial_seqs;

typedef struct {
    char data[MAX_PACKET];
    size_t len;
    struct timeval sent_time;
    int retransmit_count;
} SentPacket;

typedef struct {
    char data[MAX_PACKET];
    size_t len;
    uint16_t seq;
} ReceivedPacket;

initial_seqs handshake(int sockfd, struct sockaddr_in *addr, int type,
                       ssize_t (*input_p)(uint8_t *, size_t),
                       void (*output_p)(uint8_t *, size_t))
{
    initial_seqs seq_nums;
    bool is_handshake_complete = false;
    while (!is_handshake_complete)
    {
        if (type == CLIENT)
        {
            // client

            // handshake, SYN -> SYN-ACK -> ACK
            uint16_t client_initial_seq_num = initial_seq_num();
            seq_nums.seq_num = client_initial_seq_num;
            struct sockaddr_in server_addr = *addr;
            while (!is_handshake_complete)
            { // while loop for retransmission
                // send SYN
                uint8_t payload[MAX_PAYLOAD] = {0};
                ssize_t payload_size = 0;

                char syn_packet_buffer[sizeof(packet) + MAX_PAYLOAD] = {0};
                packet *syn_packet = (packet *)syn_packet_buffer;
                syn_packet->seq = htons(client_initial_seq_num);
                syn_packet->ack = htons(0);
                syn_packet->length = htons(payload_size);
                syn_packet->win = htons(MIN_WINDOW);
                syn_packet->flags = SYN;
                memcpy(syn_packet->payload, payload, payload_size);
                set_parity(syn_packet);
                print_diag(syn_packet, SEND);

                sendto(sockfd, syn_packet, sizeof(packet) + payload_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

                // receive SYN-ACK
                // select is basically a timeout for recvfrom
                // https://cboard.cprogramming.com/networking-device-communication/56595-recvfrom-timeout.html
                fd_set read_fds;
                FD_ZERO(&read_fds);              // initialize
                FD_SET(sockfd, &read_fds);       // add sockfd to the set
                struct timeval timeout = {1, 0}; // 1s timeout
                int is_syn_ack_received = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

                if (is_syn_ack_received == 0)
                {
                    continue; // timeout, retransmit
                }
                else if (is_syn_ack_received < 0)
                {
                    fprintf(stderr, "select() failed\n");
                    exit(1);
                }
                char receive_buffer[sizeof(packet) + MAX_PAYLOAD] = {0};
                socklen_t server_addr_size = sizeof(server_addr);
                ssize_t bytes = recvfrom(sockfd, receive_buffer, sizeof(receive_buffer), 0, (struct sockaddr *)&server_addr, &server_addr_size);
                if (bytes < (ssize_t)sizeof(packet))
                    continue;

                packet *receive_packet = (packet *)receive_buffer;
                print_diag(receive_packet, RECV);
                uint16_t flags = receive_packet->flags;
                bool syn = flags & SYN;
                bool ack = flags & ACK;
                uint16_t ack_num = ntohs(receive_packet->ack);

                // send ACK
                if (syn && ack && ack_num == client_initial_seq_num + 1 && bit_count(receive_packet) % 2 == 0)
                {
                    uint16_t server_initial_seq_num = ntohs(receive_packet->seq);
                    seq_nums.received_ack = server_initial_seq_num + 1;

                    uint8_t ack_payload[MAX_PAYLOAD] = {0};
                    ssize_t ack_payload_size = 0;

                    char ack_buffer[sizeof(packet) + MAX_PAYLOAD] = {0};
                    packet *ack_packet = (packet *)ack_buffer;
                    ack_packet->seq = htons(ack_num);
                    ack_packet->ack = htons(server_initial_seq_num + 1);
                    ack_packet->length = htons(ack_payload_size);
                    ack_packet->win = htons(MIN_WINDOW);
                    ack_packet->flags = ACK;
                    memcpy(ack_packet->payload, ack_payload, ack_payload_size);
                    set_parity(ack_packet);
                    print_diag(ack_packet, SEND);

                    sendto(sockfd, ack_packet, sizeof(packet) + ack_payload_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                    is_handshake_complete = true;
                }
            }

            return seq_nums;
        }
        else
        {
            // server
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            uint16_t server_initial_seq_num = initial_seq_num();
            seq_nums.seq_num = server_initial_seq_num;
            while (!is_handshake_complete)
            { // while loop for retransmission
                // receive SYN
                char receive_buffer[sizeof(packet) + MAX_PAYLOAD] = {0};
                ssize_t bytes = recvfrom(sockfd, receive_buffer, sizeof(receive_buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
                if (bytes < (ssize_t)sizeof(packet))
                    continue;

                packet *receive_packet = (packet *)receive_buffer;
                if (bit_count(receive_packet) % 2 != 0)
                    continue;
                print_diag(receive_packet, RECV);

                uint16_t flags = receive_packet->flags;
                bool syn = flags & SYN;
                bool ack = flags & ACK;
                uint16_t client_initial_seq_num = ntohs(receive_packet->seq);

                if (syn && !ack)
                {
                    // send SYN-ACK
                    uint8_t syn_ack_payload[MAX_PAYLOAD] = {0};
                    ssize_t syn_ack_size = 0;

                    char syn_ack_buffer[sizeof(packet) + MAX_PAYLOAD] = {0};
                    packet *syn_ack_packet = (packet *)syn_ack_buffer;
                    syn_ack_packet->seq = htons(server_initial_seq_num);
                    syn_ack_packet->ack = htons(client_initial_seq_num + 1);
                    syn_ack_packet->length = htons(syn_ack_size);
                    syn_ack_packet->win = htons(MIN_WINDOW);
                    syn_ack_packet->flags = SYN | ACK;
                    memcpy(syn_ack_packet->payload, syn_ack_payload, syn_ack_size);
                    set_parity(syn_ack_packet);
                    print_diag(syn_ack_packet, SEND);

                    sendto(sockfd, syn_ack_packet, sizeof(packet) + syn_ack_size, 0, (struct sockaddr *)&client_addr, addr_len);

                    // recieve ACK
                    fd_set read_fds;
                    FD_ZERO(&read_fds);
                    FD_SET(sockfd, &read_fds);
                    struct timeval timeout = {1, 0}; // 1s timeout
                    int is_ack_received = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

                    if (is_ack_received == 0)
                    {
                        continue; // timeout, retransmit
                    }
                    else if (is_ack_received < 0)
                    {
                        fprintf(stderr, "select() failed\n");
                        exit(1);
                    }

                    char ack_buffer[sizeof(packet) + MAX_PAYLOAD] = {0};
                    struct sockaddr_in ack_addr;
                    socklen_t ack_addr_len = sizeof(ack_addr);
                    ssize_t ack_bytes = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&ack_addr, &ack_addr_len);
                    if (ack_bytes < (ssize_t)sizeof(packet))

                        if (ack_addr.sin_addr.s_addr != client_addr.sin_addr.s_addr || ack_addr.sin_port != client_addr.sin_port)
                            continue;

                    packet *ack_packet = (packet *)ack_buffer;
                    if (bit_count(ack_packet) % 2 != 0)
                        continue;
                    print_diag(ack_packet, RECV);

                    uint16_t ack_flags = ack_packet->flags;
                    uint16_t ack_num = ntohs(ack_packet->ack);
                    uint16_t seq_num = ntohs(ack_packet->seq);

                    seq_nums.received_ack = seq_num;

                    if ((ack_flags & ACK) && ack_num == server_initial_seq_num + 1)
                    {
                        // seq_nums.client_num = seq_num;
                        // seq_nums.server_num = ack_num;
                        is_handshake_complete = true;
                    }
                }
            }

            return seq_nums;
        }
    }
    return seq_nums;
}

// convert packet from network to host order
void ntohs_packet(packet *pkt)
{
    pkt->seq = ntohs(pkt->seq);
    pkt->ack = ntohs(pkt->ack);
    pkt->length = ntohs(pkt->length);
    pkt->win = ntohs(pkt->win);
    pkt->flags = pkt->flags;
    pkt->unused = ntohs(pkt->unused);
}

// convert packet from host to network order
void htons_packet(packet *pkt)
{
    pkt->seq = htons(pkt->seq);
    pkt->ack = htons(pkt->ack);
    pkt->length = htons(pkt->length);
    pkt->win = htons(pkt->win);
    pkt->flags = pkt->flags;
    pkt->unused = htons(pkt->unused);
}

// Main function of transport layer; never quits
void listen_loop(int sockfd, struct sockaddr_in *addr, int type,
                 ssize_t (*input_p)(uint8_t *, size_t),
                 void (*output_p)(uint8_t *, size_t))
{

    srand(time(NULL) ^ getpid()); // make initial_seq_num() different for each process

    initial_seqs initial_seq_nums = handshake(sockfd, addr, type, input_p, output_p);
    // fprintf(stderr, "seq: %d, ack: %d\n", initial_seq_nums.seq_num, initial_seq_nums.received_ack);

    SentPacket sender_buffer[MAX_BUFFER];
    int sender_buffer_count = 0;
    int bytes_in_flight = 0;
    int current_window = MIN_WINDOW;

    ReceivedPacket receiver_buffer[MAX_BUFFER];
    int receiver_buffer_count = 0;
    int receiver_buffer_bytes = 0;

    uint16_t my_ack = initial_seq_nums.received_ack;
    uint16_t my_seq = initial_seq_nums.seq_num+1;

    uint16_t last_ack_num = 0;
    int dup_ack_count = 0;

    // indexes receiver buffer
    int last_packet_rcvd = 0;

    // manages sender buffer
    int last_packet_sent = 0, last_packet_acked = 0;

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        struct timeval timeout = {0.1, 0};

        int ready = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (ready < 0) {
            perror("select");
            exit(1);
        } else if (ready > 0) {
            struct sockaddr_in from_addr;
            socklen_t addr_len = sizeof(from_addr);
            uint8_t buf[MAX_PACKET];
            ssize_t bytes_recvd = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from_addr, &addr_len);

            if (bytes_recvd > 0) {
                packet *pkt = (packet *)buf;
                if (bit_count(pkt) % 2 != 0) {
                    continue;
                }
                print_diag(pkt, RECV); // print_diag assumes network order
                ntohs_packet(pkt);
                

                uint16_t flags = pkt->flags;
                bool syn = flags & SYN;
                bool ack = flags & ACK;
                uint16_t ack_num = pkt->ack;
                uint16_t seq_num = pkt->seq;
                my_ack = seq_num + 1;

                if (pkt->length > 0) {
                    output_p(pkt->payload, pkt->length);
                    fprintf(stderr, "received: %s\n", pkt->payload);
                }
                // ssize_t output_len = output_p(pkt->payload, MAX_PAYLOAD);
            }
        }

        // check input
        uint8_t input_data[MAX_PAYLOAD];
        ssize_t input_len = input_p(input_data, MAX_PAYLOAD);
        if (input_len > 0) {
            uint8_t send_buf[MAX_PACKET] = {0};
            packet *send_pkt = (packet *)send_buf;
            send_pkt->seq = my_seq;
            my_seq++;
            send_pkt->ack = my_ack;
            send_pkt->length = input_len;
            send_pkt->win = MIN_WINDOW;
            send_pkt->flags |= ACK;
            memcpy(send_pkt->payload, input_data, input_len);
            set_parity(send_pkt);
            htons_packet(send_pkt);
            
            // htons_packet(send_pkt);
            sendto(sockfd, send_pkt, sizeof(packet) + input_len, 0, (struct sockaddr *)addr, sizeof(*addr));
            print_diag(send_pkt, SEND);
            // add_to_send_buffer(send_pkt, sizeof(packet) + input_len);
        }
    }
}