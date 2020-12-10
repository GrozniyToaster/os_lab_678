#pragma once

#define BUF_SIZE 256

#define TO_ALL -42

typedef enum {
    RECONNECT,
    CALCULATE,
    PING,
    DATA
} Command;


typedef struct {
    char IN_SOCKET_NAME [BUF_SIZE];
    char OUT_SOCKET_NAME [BUF_SIZE];
    int MY_ID;
    void* ISTREAM;
    void* OSTREAM;
    void* CONTEXT;
} INFO ;


typedef struct{
    
    int sender;
    int recipient;
    Command type; 
    char data[BUF_SIZE];
    int moreData;
    int messageID;


} message;

void messageInit( message* mes, int s,int r, Command t, char* d, int flag, int id  ){
    mes -> sender = s;
    mes -> recipient = r;
    mes -> type = t;
    strcpy( mes -> data, d );
    mes -> moreData = flag;
    mes -> messageID = id;
}