#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <ftw.h>
#include <string.h>
#include <time.h>
#include <zmq.h>

#include "structs.h"

#define MAX_CHILD 54


int COUNT_CHILD = 0;
int CHILDS[MAX_CHILD];
char TEMP_DIR[] = "/tmp/lab678.XXXXXX";
const char CHILD_NAME[] = "./client";
//char OUTPUT_NAME[] = "./output_module";
IDCard ID;




void initialize( IDCard* ID ){
    char *dir_name = mkdtemp(TEMP_DIR);

    if(dir_name == NULL){
        perror("mkdtemp failed: ");
        exit(0);
    }

    sprintf( ID -> OSN, "ipc://%s/master.out", TEMP_DIR  );
    sprintf( ID -> PSN, "ipc://%s/ping", TEMP_DIR  );

    ID -> MY_ID = MAIN_MODULE;
    ID -> CONTEXT = zmq_ctx_new ();

    ID -> OS = zmq_socket( ID -> CONTEXT, ZMQ_PUB );
    int rc = zmq_bind( ID -> OS, ID -> OSN );
    assert (rc == 0);

    ID -> PS = zmq_socket( ID -> CONTEXT, ZMQ_REP );
    rc = zmq_bind( ID -> PS, ID -> PSN );
    assert (rc == 0);

    for ( int i = 0 ; i < MAX_CHILD; i++ ){
        CHILDS[i] = -1;
    }
} 

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    int rv = remove(fpath);
    if (rv){
        perror(fpath);
    }
    return rv;
}

int rmrf(char *path){
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS); 
}

void deinitialize( IDCard* ID ){
    zmq_close(ID -> OS );
    zmq_close(ID -> PS );
    
    for ( int i = 0; i < MAX_CHILD; ++i ){
        if ( CHILDS[i] != -1 ){
            kill( CHILDS[i] , SIGKILL );
        }
    }
    
    zmq_ctx_destroy (ID -> CONTEXT);
    rmrf( TEMP_DIR );
    //kill( 0, SIGKILL );
}

void recreateOutput(){
    zmq_close (ID.OS);
    sleep(1);
    ID.OS = zmq_socket ( ID.CONTEXT, ZMQ_PUB );
    int rc = zmq_bind( ID.OS, ID.OSN );
    assert (rc == 0);
}


int initChild( int id ){
    if ( CHILDS[id] != -1 ){
        return 2;
    }
    
    if ( COUNT_CHILD != 0 ){
        printf("Sendind rebuild\n");
        zmq_msg_t toRebuild;
        char sockIn[BUF_SIZE];
        sprintf( sockIn, "ipc://%s/%d.out", TEMP_DIR, id ); 
        zmq_messageInit( &toRebuild, ID.MY_ID, FIRST_WATCHED, -1, RECONNECT, sockIn, 0, 0 );
        zmq_msg_send( &toRebuild, ID.OS,  0 );
        zmq_msg_close( &toRebuild );
        sleep(1);
        recreateOutput();
    }
    
    char argID[3];
    char arg_socket_in[BUF_SIZE];
    char arg_socket_out[BUF_SIZE];
    char arg_socket_ping[BUF_SIZE];
    sprintf(argID, "%d", id );
    sprintf(arg_socket_in, "%s", ID.OSN  );
    sprintf(arg_socket_out, "ipc://%s/%d.out", TEMP_DIR, id );
    sprintf(arg_socket_ping, "%s", ID.PSN  );

    int childPID = fork();
    if ( childPID == -1 ){
        printf("Fork err\n");
        return 1;
    }else if ( childPID == 0 ){
        execl( CHILD_NAME, argID, arg_socket_in, arg_socket_out, arg_socket_ping, NULL );
    }else{
        CHILDS[ id ] = childPID;
        COUNT_CHILD++;
    }
    return 0;
}


void ping(){
    zmq_msg_t ping_message;
    char mes[] = "PING_TO_ALL";
    zmq_messageInit( &ping_message, MAIN_MODULE, TO_ALL, MAIN_MODULE, PING, mes, 0, 0  );
    zmq_msg_send( &ping_message, ID.OS, 0 );
    //sleep(COUNT_CHILD * 2 );
    int pings[MAX_CHILD];
    for ( int i = 0; i < MAX_CHILD; i++ ){
        pings[i] = -1;
    }
    //printf("Serv start collect pings \n" );
    message data;
    errno = 0;
    int i = 0;
    while( i < 10 ){
        sleep(1);
        zmq_msg_t p;
        zmq_msg_init( &p );
        int size_of_message = zmq_msg_recv( &p, ID.PS, ZMQ_DONTWAIT);
        if ( size_of_message <= 0 ){ 
            i++;
            continue;
        }
        memcpy( &data, zmq_msg_data(&p),  sizeof(message));
        printf("Serv get ping from %d %d\n", data.messageID, errno );
        pings[ data.messageID ] = 1;
        zmq_msg_close( &p );
        sleep(1);
        zmq_msg_t r;
        zmq_messageInit( &r, MAIN_MODULE, data.sender, MAIN_MODULE, ANSWER, mes, 0, 0 );
        zmq_msg_send( &r, ID.PS, 0 );
        zmq_msg_close( &r );
        i++;
    }
    int tokill[MAX_CHILD];
    int count_to_kill = 0;
    for ( int i = 0 ; i < MAX_CHILD; i++ ){
        if (  CHILDS[i] != -1 && pings[i] != 1 ){
            //printf( "ping %d child %d\n", pings[i], CHILDS[i] );
            tokill[ count_to_kill ] = i;
            count_to_kill++;
        }
    }
    char res[BUF_SIZE];
    if ( count_to_kill == 0 ){
        printf( "Ok: -1\n" );
        return;
    }
    res[0] = '\0';
    for ( int i = 0; i < count_to_kill; i++ ){
        char tmp[5];
        sprintf( tmp, "%d ", tokill[i] );
        strcat( res, tmp );
        kill( CHILDS[ tokill[i] ] , SIGTERM );
        CHILDS[ tokill[i] ] = -1; 
    }
    printf("Ok: %s\n", res );
}



int main (void){

    initialize( &ID );
    initChild(1);
    //sleep(1);
    initChild(2);
    //sleep(1);
    initChild(3);

    char kek[] ="1 2 3 4 5";
    sleep(1);
    //ping();
    //zmq_msg_t toSend;
    //message_standart( &toSend, MAIN_MODULE, 3, DELETE, "" );
    //zmq_msg_send (&toSend, ID.OS ,  0);
    //ping();
    for ( int i = 1 ; i < 4 ; i++ ){
        sleep(1);
        zmq_msg_t toSend;
        zmq_messageInit( &toSend, -1, 1, -1, CALCULATE, kek , 1, 0  );
        zmq_msg_send (&toSend, ID.OS ,  0);
        zmq_msg_close( &toSend );
    }
    zmq_msg_t toSend;
        zmq_messageInit( &toSend, -1, 1, -1, CALCULATE, kek , 0, 0  );
        zmq_msg_send (&toSend, ID.OS ,  0);
        zmq_msg_close( &toSend );
    sleep(3);
    deinitialize( &ID );
    return 0;
}