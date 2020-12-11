#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <ftw.h>
#include <signal.h>

#include "structs.h"

#define MAX_CHILD 54


int COUNT_CHILD = 0;
int CHILDS[MAX_CHILD];
char TEMP_DIR[] = "/tmp/lab678.XXXXXX";

char CHILD_NAME[] = "./client";

INFO MyPassport;

void initialize( INFO* info ){
    char *dir_name = mkdtemp(TEMP_DIR);

    if(dir_name == NULL){
        perror("mkdtemp failed: ");
        exit(0);
    }
    char buf[BUF_SIZE];
    sprintf( info -> TOSN, "ipc://%s/father.out", TEMP_DIR );
    //strcpy( info -> TOS, buf );
    sprintf( info -> TISN, "ipc://%s/father.in", TEMP_DIR );
    //strcpy( info -> TOS, buf );
    info -> CONTEXT = zmq_ctx_new ();
    info -> TOS = zmq_socket ( info -> CONTEXT, ZMQ_REQ );
    info -> TIS = zmq_socket ( info -> CONTEXT, ZMQ_REP );
    info -> MY_ID = -1;
    int rc = zmq_connect ( info -> TOS, info -> TOSN );
    assert (rc == 0); // perror
    rc = zmq_connect ( info -> TIS, info -> TISN );
    assert (rc == 0); // perror
    
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
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS); // из за этого g++
}

void deinitialize( INFO* info ){
    zmq_close (info -> TOS);
    zmq_close (info -> TIS);
    /*
    for ( int i = 0; i < MAX_CHILD; ++i ){
        if ( CHILDS[i] != -1 ){
            kill( CHILDS[i] , SIGKILL );
        }
    }
    */
    zmq_ctx_destroy (info -> CONTEXT);
    rmrf( TEMP_DIR );
    kill( 0, SIGKILL );
}

int initChild( int id ){
    if ( CHILDS[id] != -1 ){
        return 2;
    }
    if ( COUNT_CHILD != 0 ){
        message toRebuild;
        char data[BUF_SIZE];
        sprintf( data, "ipc://%s/%d.out", TEMP_DIR, id ); 
        messageInit( &toRebuild, MyPassport.MY_ID, TO_ALL, RECONNECT, data, 0, 0 );
    }

    char argID[3];
    char arg_head_socket_in[BUF_SIZE];
    char arg_head_socket_out[BUF_SIZE];
    char arg_tail_socket_in[BUF_SIZE];
    char arg_tail_socket_out[BUF_SIZE];
    sprintf(argID, "%d", id );
    sprintf(arg_head_socket_in, "ipc://%s/father.out", TEMP_DIR  );
    sprintf(arg_head_socket_out, "ipc://%s/father.in", TEMP_DIR  );
    sprintf(arg_tail_socket_out, "ipc://%s/%d.out", TEMP_DIR, id );
    sprintf(arg_tail_socket_in, "ipc://%s/%d.out", TEMP_DIR, id );


    int childPID;
    if ( (childPID = fork()) == -1 ){
        printf("Fork err\n");
        return 1;
    }else if ( childPID == 0 ){
        execl( CHILD_NAME, argID, arg_head_socket_in, arg_head_socket_out, arg_tail_socket_out, arg_tail_socket_in, NULL );
    }else{
        CHILDS[ id ] = childPID;
        COUNT_CHILD++;
    }
    return 0;
}


int main (void){
    //  Socket to talk to clients
    //void *context = zmq_ctx_new ();
    initialize( &MyPassport );
    //printf("pass %s", myPassport.OUT_SOCKET );
    //void *responder = zmq_socket (context, ZMQ_PAIR );
    
    //int rc = zmq_connect (responder, myPassport.OUT_SOCKET );
    //assert (rc == 0);
    char buffer[10];
    initChild(1);
    //initChild(2);
    /*
    int i =0;
    i++;
    printf ("Received ");
    printf( "%d %s\n", i,buffer );
    sleep (1);          //  Do some 'work'
    */
    
    char kek[] ="";

    message toSend;
    messageInit( &toSend, -1, 1, CALCULATE, kek , 0, 0  );
    zmq_send (MyPassport.TOS, &toSend, sizeof( message ), 0);
    zmq_recv (MyPassport.TOS, &toSend, sizeof(message) , 0);
    
    //messageInit( &toSend, -1, 2, CALCULATE, kek, 0, 0  );
    //zmq_send (MyPassport.TOS, &toSend, sizeof( message ), 0);
    
    sleep(3);
    zmq_recv (MyPassport.TIS, &toSend, sizeof(message) , 0);
    
    printf("Serv %s\n", toSend.data );
    //zmq_recv (MyPassport.TOS, &toSend, sizeof( message ), 0);
    //printf("Serv %s\n", toSend.data );
    //printf("Serv %s\n", toSend.data );
    //zmq_send (MyPassport.OSTREAM, "Ofcorse!!", 10, 0);
    zmq_send (MyPassport.TIS, &toSend, sizeof( message ), 0);

    deinitialize( &MyPassport );
    //zmq_close (MyPassport.OSTREAM);
    //zmq_ctx_destroy (MyPassport.CONTEXT);
    return 0;
}