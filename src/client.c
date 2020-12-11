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
    //printf("HISN %s\n", inf -> HISN);
    strcpy( inf -> HOSN, argv[2] );
    strcpy( inf -> TISN, argv[3] );
    //printf("TISN %s\n", inf -> TISN);
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
    rc = zmq_connect ( info -> TIS, info -> TISN );
    assert (rc == 0); // perror
    rc = zmq_connect ( info -> TOS, info -> TOSN );
    assert (rc == 0); // perror
    //printf( "child %d initialized\n", MyPassport.MY_ID );
} 

void deinitialize( INFO* info ){ // on signals
    zmq_disconnect  (info -> TOS, info -> TOSN);
    zmq_unbind  (info -> TIS, info -> TISN);
    
    //zmq_close (info -> OSTREAM);
    zmq_ctx_destroy (info -> CONTEXT);
}

void recreateOutput(){
    zmq_close (MyPassport.TOS);
        zmq_close (MyPassport.TIS);
        MyPassport.HOS = zmq_socket ( MyPassport.CONTEXT, ZMQ_REQ );
        MyPassport.HIS = zmq_socket ( MyPassport.CONTEXT, ZMQ_REP );
        int rc = zmq_connect ( MyPassport.TOS, MyPassport.TOSN );
        assert (rc == 0); // perror
        rc = zmq_connect ( MyPassport.TIS, MyPassport.TISN );
        assert (rc == 0); // perror
}

void reconnect( message* m ){
    //printf("starting reconnect\n");
    int rc = zmq_unbind( MyPassport.HIS, MyPassport.HISN );
    assert (rc == 0);
    rc = zmq_unbind( MyPassport.HOS, MyPassport.HOSN );
    assert (rc == 0);
    //printf("child %d Disconect %s\n", MyPassport.MY_ID, MyPassport.HISN );
    
    MyPassport.HOS = zmq_socket ( MyPassport.CONTEXT, ZMQ_REQ );
    MyPassport.HIS = zmq_socket ( MyPassport.CONTEXT, ZMQ_REP );
    sleep(2);
    char* tok = strtok( m -> data , " ");
    //printf( "%d reconnect HIS to %s\n", MyPassport.MY_ID,tok );
    rc = zmq_bind( MyPassport.HIS, m -> data );
    assert (rc == 0);
    
    strcpy( MyPassport.HISN, tok );
    
    tok = strtok( NULL , " ");
    //printf( "%d reconnect HOS to %s}\n", MyPassport.MY_ID,tok );
    rc = zmq_bind( MyPassport.HOS, tok );
    assert (rc == 0);
    
    strcpy( MyPassport.HOSN, tok );
    //printf("child %d binded to %s\n", MyPassport.MY_ID, MyPassport.HISN );

}

void forward( message* m, void* deraction ){
    m -> lastowner = MyPassport.MY_ID;
    printf("Forward from %d to %d mess: %s \n", m -> sender , m ->recipient, m -> data );
    zmq_send( deraction, m, sizeof( message ), 0 );
    printf("%d wait complete forword to %d\n", MyPassport.MY_ID , m ->recipient );
    zmq_recv( deraction, m, sizeof( message ), 0 );
    printf(" %d Forward ended from %d to %d %s\n", MyPassport.MY_ID , m -> sender , m ->recipient, m -> data );
    
    if ( m -> type == ERR ){
        printf("mess with err");
    }
}

int i = 1;

void calculate( message* m ){
    message answer;
    char bud[BUF_SIZE];
    sprintf( bud, "Answer %d from %d", i, MyPassport.MY_ID );
    messageInit( &answer, MyPassport.MY_ID, m -> sender, MyPassport.MY_ID, ANSWER, bud, 0, 0 );
    forward( &answer, MyPassport.HOS );
}


void parseMessage( message* m, int from  ){
    printf( "Child %d received mess: s %d r %d data %s\n", MyPassport.MY_ID, m -> sender, m -> recipient, m -> data );
    if ( MyPassport.MY_ID != m -> recipient && m -> recipient != TO_ALL && m -> recipient != FIRST_WATCHED ){ // first watching message
        //printf( "dirrect %d\n", from);
        if ( from == 0 ){
            forward( m, MyPassport.TOS );
        }else{
            forward( m, MyPassport.HOS );
        }
        return;
    }
    if ( m -> type == RECONNECT ){
        reconnect( m );
        return;
    }
    if ( m -> type == CALCULATE ){
        calculate( m );
    }
}

void answer( void* deraction, message* m ){
    message toAnswer;
    char kek[] = "Forworded";
    messageInit( &toAnswer, MyPassport.MY_ID, m->lastowner, MyPassport.MY_ID, REPLY, kek, 0,0  );
    zmq_send( deraction, &toAnswer, sizeof( message ), 0 );
    printf( "child %d answered %d\n", MyPassport.MY_ID ,m -> lastowner );
}

void cycle (){
    message received;
    while ( 1 ){
        printf( "child %d\n", MyPassport.MY_ID );
        printf( "id %d hisn %s tisn %s\n\t hosn %s tosn %s\n", MyPassport.MY_ID, MyPassport.HISN, MyPassport.TISN, MyPassport.HOSN, MyPassport.TOSN );
        zmq_recv( MyPassport.TIS, &received, sizeof( received ), ZMQ_DONTWAIT );
        if ( errno != EAGAIN ){
            answer( MyPassport.TIS, &received );
            parseMessage( &received, 1 );
            
        }
        zmq_recv( MyPassport.HIS, &received, sizeof( received ), ZMQ_DONTWAIT );
        if ( errno != EAGAIN ){
            printf("%d gain from head\n", MyPassport.MY_ID);    
            answer( MyPassport.HIS, &received );
            parseMessage( &received, 0 );
            
        }
        sleep(1);
    }
}


int main (int argc, char* argv[] ){
    //printf ("Connecting to hello world server…\n");

    //printf( "argc %d\n", argc );
    for( int i = 0; i < argc ; i++ ){
        //printf( "%s\n", argv[i] );
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