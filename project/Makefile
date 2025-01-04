CC=gcc
CPPFLAGS=-Wall -Wextra 
LDFLAGS= -L.
LDLIBS=-lsocket

DEPS=

all: server client 

server: server.o $(DEPS)
client: client.o $(DEPS)

clean:
	@find . -type f \
		! -name "*.c" \
		! -name "*.h" \
		! -name "*.cpp" \
		! -name "*.hpp" \
		! -name "Makefile" -delete
	@find . -type d \( ! -name "." \) -exec rm -rf {} +
