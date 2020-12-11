CC = g++
CC_FLAGS = -Wall -std=c17
LIBS = -l zmq

all: client server

client:  client.c structs.h
	$(CC) $(CC_FLAGS) $< $(LIBS) -o $@

server: server.c structs.h
	$(CC) $(CC_FLAGS) $< $(LIBS) -o $@

clean: 
	rm client server
