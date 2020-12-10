#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include <sys/stat.h>

#include <ftw.h>

#include "structs.h"

#define MAX_CHILD 54



int COUNT_CHILD = 0;
int CHILDS[MAX_CHILD];
char TEMP_DIR[] = "/tmp/lab678.XXXXXX";

char CHILD_NAME[] = "./client";

INFO MyPassport;

void initialize( INFO* inf ){
    char *dir_name = mkdtemp(TEMP_DIR);

    if(dir_name == NULL){
        perror("mkdtemp failed: ");
        exit(0);
    }
    char buf[BUF_SIZE];
    sprintf( buf, "ipc://%s/father", TEMP_DIR );
    strcpy( inf -> OUT_SOCKET_NAME, buf );
    inf -> CONTEXT = zmq_ctx_new ();
    inf -> OSTREAM = zmq_socket ( inf -> CONTEXT, ZMQ_PAIR );
    inf -> MY_ID = -1;
    int rc = zmq_connect ( inf -> OSTREAM, inf -> OUT_SOCKET_NAME );
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
    zmq_close (info -> OSTREAM);
    zmq_ctx_destroy (info -> CONTEXT);
    rmrf( TEMP_DIR );
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
    char arg_in_socket[BUF_SIZE];
    char arg_out_socket[BUF_SIZE];
    sprintf(argID, "%d", id );
    sprintf(arg_in_socket, "ipc://%s/father", TEMP_DIR  );
    sprintf(arg_out_socket, "ipc://%s/%d.out", TEMP_DIR, id );


    int childPID;
    if ( (childPID = fork()) == -1 ){
        printf("Fork err\n");
        return 1;
    }else if ( childPID == 0 ){
        execl( CHILD_NAME, argID, arg_in_socket, arg_out_socket, NULL );
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
    /*
    int i =0;
    i++;
    printf ("Received ");
    printf( "%d %s\n", i,buffer );
    sleep (1);          //  Do some 'work'
    */
    zmq_send (MyPassport.OSTREAM, "Hi son!!!", 10, 0);
    zmq_recv (MyPassport.OSTREAM, buffer,10 , 0);
    printf("Serv %s\n",buffer);
    sleep(3);
    zmq_recv (MyPassport.OSTREAM, buffer,10 , 0);
    printf("Serv %s\n",buffer);
    zmq_send (MyPassport.OSTREAM, "Ofcorse!!", 10, 0);

    deinitialize( &MyPassport );
    //zmq_close (MyPassport.OSTREAM);
    //zmq_ctx_destroy (MyPassport.CONTEXT);
    return 0;
}