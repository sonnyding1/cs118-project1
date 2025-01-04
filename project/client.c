#include "libsocket.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: client <hostname> <port> \n");
        exit(1);
    }

    // Only supports localhost as a hostname, but that's all we'll test on
    const char* addr =
        strcmp(argv[1], "localhost") == 0 ? "127.0.0.1" : argv[1];
    int port = atoi(argv[2]);

    // TODO: Create socket

    // TODO: Set stdin and socket nonblocking

    // TODO: Construct server address
    
    char buffer[1024];

    // Listen loop
    while (1) {
        // TODO: Receive from socket
        // TODO: If data, write to stdout
        // TODO: Read from stdin
        // TODO: If data, send to socket
    }

    return 0;
}
