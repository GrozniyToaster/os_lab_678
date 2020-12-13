#pragma once

#include <string.h> 
#include <zmq.h>

#define BUF_SIZE 256
#define MAIN_MODULE -1
#define TO_ALL -42
#define FIRST_WATCHED -87

#define MORE_DATA 1



typedef enum {
    REPLY,
    RECONNECT,
    CALCULATE,
    ANSWER,
    PING,
    DATA,
    DELETE,
    ERR
} Command;

typedef enum {
    MAIN,
    CALCULATOR
} type_of_node;

typedef struct {
    type_of_node type;
    int MY_ID;
    void* CONTEXT;
    char ISN [BUF_SIZE]; //INPUT_SOCKET_NAME
    char OSN [BUF_SIZE]; //OUTPUT_SOCKET_NAME
    char PSN [BUF_SIZE]; //PING_SOCKET_NAME
    void* IS; // INPUT_STREAM
    void* OS; // OUTPUT_STREAM
    void* PS; // OUTPUT_STREAM
} IDCard ;



typedef struct{
    Command type; 
    int sender;
    int recipient;
    int lastowner;
    char data[BUF_SIZE];
    int moreData;
    int messageID;
} message;

void zmq_messageInit( zmq_msg_t* mes, int sender, int recipient, int lastowner, Command command, char* data, int moreData, int messageID  );

void messageInit( message* mes, int sender, int recipient, int lastowner, Command command, char* data, int moreData, int messageID  );

void message_standart( zmq_msg_t* mes, int sender ,int recipient, Command command, char* data );