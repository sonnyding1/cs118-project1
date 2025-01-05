CC=gcc
CPPFLAGS=-Wall -Wextra 
LDFLAGS= 
LDLIBS=-lsocket

DEPS=

all: server client 

server: server.o $(DEPS)
client: client.o $(DEPS)

clean:
	@rm -rf server client *.bin *.o	
