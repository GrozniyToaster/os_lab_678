CC = gcc
CC_FLAGS = -Wall -std=c17
LIBS = -l zmq

all: client server

client:  client.c client_utils.c  structs.o structs.h client.h 
	$(CC) $(CC_FLAGS) structs.o  client.c client_utils.c $(LIBS) -o $@

server: server.c server_utils.c structs.o structs.h server.h 
	$(CC) $(CC_FLAGS) structs.o  server.c server_utils.c $(LIBS) -o $@

structs.o:  structs.c structs.h
	$(CC) $(CC_FLAGS) -c $< $(LIBS) -o $@ 

clean: 
	rm client server *.o
