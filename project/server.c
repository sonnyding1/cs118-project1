#include "libsocket.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: server <port>\n");
        exit(1);
    }

    int port = atoi(argv[1]);

    // TODO: Create socket

    // TODO: Set stdin and socket nonblocking

    // TODO: Construct server address
    
    // TODO: Bind address to socket
    
    // TODO: Create sockaddr_in and socklen_t buffers to store client address

    char buffer[1024];
    int client_connected = 0;

    // Listen loop
    while (1) {
        // TODO: Receive from socket
        // TODO: If no data and client not connected, continue
        // TODO: If data, client is connected and write to stdout
        // TODO: Read from stdin
        // TODO: If data, send to socket
    }

    return 0;
}
