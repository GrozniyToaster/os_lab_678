#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "structs.h"

#include <error.h>

INFO MyPassport;


void getINFO( INFO* inf, const int argc,  char** argv ){
    if ( argc != 5  ){
        printf("Usage <id> <in socket head> <out socket head> <out socket tail> <in socket tail>\n");
        exit (1);
    }
    inf -> MY_ID = atoi( argv[0] );
    strcpy( inf -> HISN, argv[1] );
    printf("HISN %s\n", inf -> HISN);
    strcpy( inf -> HOSN, argv[2] );
    strcpy( inf -> TISN, argv[3] );
    strcpy( inf -> TOSN, argv[4] );
}

void initialize( INFO* info ){
    info -> CONTEXT = zmq_ctx_new ();
    info -> HIS = zmq_socket ( info -> CONTEXT, ZMQ_REP );
    info -> HOS = zmq_socket ( info -> CONTEXT, ZMQ_REQ );
    info -> TIS = zmq_socket ( info -> CONTEXT, ZMQ_REP );
    info -> TOS = zmq_socket ( info -> CONTEXT, ZMQ_REQ );
    int rc = zmq_bind ( info -> HOS, info -> HOSN );
    assert (rc == 0); // perror
    rc = zmq_bind ( info -> HIS, info -> HISN );
    assert (rc == 0); // perror
} 

void deinitialize( INFO* info ){ // on signals
    zmq_disconnect  (info -> TOS, info -> TOSN);
    zmq_unbind  (info -> TIS, info -> TISN);
    
    //zmq_close (info -> OSTREAM);
    zmq_ctx_destroy (info -> CONTEXT);
}



void reconnect( message* m ){
    int rc = zmq_disconnect( MyPassport.HIS, MyPassport.HISN );
    assert (rc == 0);
    printf("child %d Disconect %s\n", MyPassport.MY_ID, MyPassport.HISN );
    rc = zmq_bind( MyPassport.HIS, m -> data );
    assert (rc == 0);
    printf("child %d binded to %s\n", MyPassport.MY_ID, MyPassport.HISN );

    strcpy( MyPassport.HISN, m -> data );
}

void forward( message* m, void* deraction ){
    zmq_send( deraction, m, sizeof( message ), 0);
    zmq_recv( deraction, m, sizeof( message ), 0 );
    if ( m -> type == ERR ){
        printf("mess with err");
    }
}

int i = 1;

void calculate( message* m ){
    message answer;
    char bud[BUF_SIZE];
    sprintf( bud, "Answer %d from %d", i,MyPassport.MY_ID );
    messageInit( &answer, MyPassport.MY_ID, m -> sender, ANSWER, bud, 0, 0 );
    forward( &answer, MyPassport.HOS );
}


void parseMessage( message* m, int from  ){
    printf( "Child %d received\n", MyPassport.MY_ID );
    if ( MyPassport.MY_ID != m -> recipient && m -> recipient != -42 ){ // first watching message
        if ( from == 0 ){
            forward( m, MyPassport.HOS );
        }else{
            forward( m, MyPassport.TOS );
        }
    }
    if ( m -> type == CALCULATE ){
        calculate( m );
    }
}

void answer( void* deraction, message* m ){
    printf( "child answered\n" );
    message toAnswer;
    messageInit( &toAnswer, MyPassport.MY_ID, m -> sender, REPLY, "", 0,0  );
    zmq_send( deraction, &toAnswer, sizeof( message ), 0 );
}

void cycle (){
    message received;
    while ( 1 ){
        printf( "child %d\n", MyPassport.MY_ID );
        zmq_recv( MyPassport.HIS, &received, sizeof( received ), ZMQ_DONTWAIT );
        if ( errno != EAGAIN ){
            answer( MyPassport.HIS, &received );
            parseMessage( &received, 0 );
            break;
        }
        zmq_recv( MyPassport.TIS, &received, sizeof( received ), ZMQ_DONTWAIT );
        if ( errno != EAGAIN ){
            answer( MyPassport.TIS, &received );
            parseMessage( &received, 1 );
            break;
        }
        sleep(1);
    }
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
    /*zmq_recv( MyPassport.ISTREAM, buf, 10, 0 );
    printf("Client %s\n", buf);
    zmq_send(MyPassport.ISTREAM, "HiServ!!!", 10, 0);
    zmq_send(MyPassport.ISTREAM, "May i ask", 10, 0);
    zmq_recv( MyPassport.ISTREAM, buf, 10, 0 );
    printf("Client %s\n", buf);
    */
    
    cycle();
    //void* out = zmq_socket (context, ZMQ_PAIR );
    /*
    
    printf( "%s\n", buf );

    zmq_close (in);
    //zmq_ctx_destroy (context);
    */
    deinitialize( &MyPassport );
    return 0;
}