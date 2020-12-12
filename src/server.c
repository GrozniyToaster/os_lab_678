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
#include <time.h>
#include "structs.h"

#define MAX_CHILD 54


int COUNT_CHILD = 0;
int CHILDS[MAX_CHILD];
char TEMP_DIR[] = "/tmp/lab678.XXXXXX";
char CHILD_NAME[] = "./client";
char OUTPUT_NAME[] = "./output_module";
IDCard ID;


void initialize( IDCard* ID ){
    char *dir_name = mkdtemp(TEMP_DIR);

    if(dir_name == NULL){
        perror("mkdtemp failed: ");
        exit(0);
    }

    char outputSocket[BUF_SIZE];
    char returnSocket[BUF_SIZE];
    sprintf( outputSocket, "ipc://%s/output.in", TEMP_DIR  );
    sprintf( returnSocket, "ipc://%s/output.out", TEMP_DIR  );

    int outputPID = fork();
    if ( outputPID  == -1 ){
        printf("Fork err\n");
        exit(1);
    }else if ( outputPID == 0 ){
        execl( OUTPUT_NAME, "output", outputSocket, returnSocket, NULL );
    }
    char buf[BUF_SIZE];
    ID -> MY_ID = MAIN_MODULE;
    ID -> CONTEXT = zmq_ctx_new ();
    //sprintf( ID -> OSN, "ipc://%s/father.out", TEMP_DIR );
    sprintf( ID -> OSN, "%s", outputSocket );
    ID -> OS = zmq_socket( ID -> CONTEXT, ZMQ_REQ );
    int rc = zmq_connect( ID -> OS, ID -> OSN );
    assert (rc == 0);
    
    sprintf( ID -> ISN, "%s", returnSocket );
    ID -> IS = zmq_socket( ID -> CONTEXT, ZMQ_REQ );
    rc = zmq_bind( ID -> IS, ID -> ISN );
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
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS); // из за этого g++
}

void deinitialize( IDCard* ID ){
    zmq_close(ID -> OS );
    zmq_close( ID -> IS );
    /*
    for ( int i = 0; i < MAX_CHILD; ++i ){
        if ( CHILDS[i] != -1 ){
            kill( CHILDS[i] , SIGKILL );
        }
    }
    */
    zmq_ctx_destroy (ID -> CONTEXT);
    rmrf( TEMP_DIR );
    kill( 0, SIGKILL );
}

void recreateOutput(){
    sleep(2);
    zmq_close (ID.OS);
    //zmq_ctx_destroy( ID.CONTEXT  );
    //ID.CONTEXT = zmq_ctx_new ();
    ID.OS = zmq_socket ( ID.CONTEXT, ZMQ_REQ );
    int rc = zmq_connect ( ID.OS, ID.OSN );
    assert (rc == 0);
}


int initChild( int id ){
    if ( CHILDS[id] != -1 ){
        return 2;
    }
    /*
    if ( COUNT_CHILD != 0 ){
        zmq_msg_t toRebuild;
        char sockIn[BUF_SIZE];
        char sockOut[BUF_SIZE];
        char data[BUF_SIZE];
        sprintf( sockOut, "ipc://%s/%d.in", TEMP_DIR, id ); 
        sprintf( sockIn, "ipc://%s/%d.out", TEMP_DIR, id );
        sprintf( data, "%s %s", sockIn, sockOut );
        zmq_messageInit( &toRebuild, ID.MY_ID, FIRST_WATCHED,-1, RECONNECT, data, 0, 0 );
        zmq_msg_send( &toRebuild, ID.TOS,  0 );
        zmq_msg_recv( &toRebuild, ID.TOS,  0 );
        sleep(1);
        recreateOutput();
    }
    */

    char argID[3];
    char arg_socket_in[BUF_SIZE];
    char arg_socket_out[BUF_SIZE];
    sprintf(argID, "%d", id );
    sprintf(arg_socket_in, "ipc://%s/%d.in", TEMP_DIR, id );
    sprintf(arg_socket_out, "%s", ID.OSN  );

    int childPID = fork();
    if ( childPID == -1 ){
        printf("Fork err\n");
        return 1;
    }else if ( childPID == 0 ){
        execl( CHILD_NAME, argID, arg_socket_in, arg_socket_out, NULL );
    }else{
        sprintf( ID.OSN, "%s", arg_socket_in );
        recreateOutput();
        CHILDS[ id ] = childPID;
        COUNT_CHILD++;
    }
    return 0;
}


int main (void){

    initialize( &ID );
    //char buffer[10];
    initChild(1);
    sleep(1);
    initChild(2);
    sleep(1);
    initChild(3);

    sleep(2);
    //sleep(3);
    char kek[] ="Calculate";

    
    for ( int i = 1 ; i < 4 ; i++ ){
        zmq_msg_t toSend;
        zmq_messageInit( &toSend, -1, 1, -1, CALCULATE, kek , 0, 0  );
        zmq_msg_send (&toSend, ID.OS ,  0);
        zmq_msg_close( &toSend );
        zmq_msg_t tmp;
        zmq_msg_init( &tmp );
        zmq_msg_recv(&tmp, ID.OS, 0);
        zmq_msg_close( &tmp );
        printf("Server send %d mess by %ld\n", i, time(NULL));
        sleep(1);
    }
    sleep(20);
    deinitialize( &ID );
    return 0;
}