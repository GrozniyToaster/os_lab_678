#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "structs.h"

INFO MyPassport;


void getINFO( INFO* inf, const int argc,  char** argv ){
    if ( argc != 3  ){
        printf("Usage <id> <in socket> <out soket>\n");
        exit (1);
    }
    inf -> MY_ID = atoi( argv[0] );
    strcpy( inf -> IN_SOCKET_NAME, argv[1] );

    strcpy( inf -> OUT_SOCKET_NAME, argv[2] );
    
}

void initialize( INFO* inf ){

    inf -> CONTEXT = zmq_ctx_new ();
    inf -> ISTREAM = zmq_socket ( inf -> CONTEXT, ZMQ_PAIR );
    inf -> OSTREAM = zmq_socket ( inf -> CONTEXT, ZMQ_PAIR );
    printf( "OUT NAME %s\n", inf -> OUT_SOCKET_NAME );
    int rc = zmq_connect ( inf -> OSTREAM, inf -> OUT_SOCKET_NAME );
    assert (rc == 0); // perror
    rc = zmq_bind ( inf -> ISTREAM, inf -> IN_SOCKET_NAME );
    assert (rc == 0); // perror
} 

void deinitialize( INFO* info ){
    zmq_disconnect(info -> ISTREAM, info -> IN_SOCKET_NAME);
    zmq_close (info -> OSTREAM);
    zmq_ctx_destroy (info -> CONTEXT);
}


int main (int argc, char* argv[] ){
    printf ("Connecting to hello world server…\n");

    printf( "argc %d\n", argc );
    for( int i = 0; i < argc ; i++ ){
        printf( "%s\n", argv[i] );
    }
    getINFO( &MyPassport, argc, argv );
    initialize( &MyPassport );
    //void* context = zmq_ctx_new ();
    //void* in = zmq_socket (context, ZMQ_PAIR );
    //printf( "Client %s\n", myPassport.IN_SOCKET );
    //printf( "ID %d, IN %s, OUT %s\n", myPassport.MY_ID, myPassport.IN_SOCKET, myPassport.OUT_SOCKET );
    //zmq_bind ( in, MyPassport.IN_SOCKET_NAME );
    char buf[80];
    zmq_recv( MyPassport.ISTREAM, buf, 10, 0 );
    printf("Client %s\n", buf);
    zmq_send(MyPassport.ISTREAM, "HiServ!!!", 10, 0);
    zmq_send(MyPassport.ISTREAM, "May i ask", 10, 0);
    zmq_recv( MyPassport.ISTREAM, buf, 10, 0 );
    printf("Client %s\n", buf);
    //void* out = zmq_socket (context, ZMQ_PAIR );
    /*

    printf( "%s\n", buf );

    zmq_close (in);
    //zmq_ctx_destroy (context);
    */
    deinitialize( &MyPassport );
    return 0;
}