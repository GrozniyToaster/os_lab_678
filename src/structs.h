#pragma once

#define BUF_SIZE 256

#define TO_ALL -42
#define FIRST_WATCHED 87

typedef enum {
    REPLY,
    RECONNECT,
    CALCULATE,
    ANSWER,
    PING,
    DATA,
    ERR
} Command;


typedef struct {
    char HISN [BUF_SIZE]; //HEAD_INPUT_SOCKET_NAME
    char HOSN [BUF_SIZE]; //HEAD_OUTPUT_SOCKET_NAME
    char TISN [BUF_SIZE]; //TAIL_INPUT_SOCKET_NAME
    char TOSN [BUF_SIZE]; //TAIL_OUTPUT_SOCKET_NAME
    int MY_ID;
    void* HIS; // HEAD_INPUT_STREAM
    void* HOS; // HEAD_OUTPUT_STREAM
    void* TIS; // TAIL_INPUT_STREAM
    void* TOS; // TAIL_OUTPUT_STREAM
    void* CONTEXT;
} INFO ;


typedef struct{
    int sender;
    int recipient;
    int lastowner;
    Command type; 
    char data[BUF_SIZE];
    int moreData;
    int messageID;
} message;

void messageInit( message* mes, int s,int r, int lo,Command t, char* d, int flag, int id  ){
    mes -> lastowner = lo;
    mes -> sender = s;
    mes -> recipient = r;
    mes -> type = t;
    strcpy( mes -> data, d );
    mes -> moreData = flag;
    mes -> messageID = id;
}