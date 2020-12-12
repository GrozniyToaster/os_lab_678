#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "structs.h"

#include <error.h>
#include <time.h>

IDCard ID;


void getIDCard( IDCard* inf, const int argc,  char** argv ){
    if ( argc != 3  ){
        printf("Usage <id> <in socket> <out socket>\n");
        exit (1);
    }
    for ( int i = 0; i< argc; i++ ){
        //printf("Client args: %s\n", argv[i]);
    }
    inf -> MY_ID = atoi( argv[0] );
    strcpy( inf -> ISN, argv[1] );
    strcpy( inf -> OSN, argv[2] );
}

void initialize( IDCard* ID ){
    ID -> CONTEXT = zmq_ctx_new ();
    ID -> IS = zmq_socket ( ID -> CONTEXT, ZMQ_REP );
    ID -> OS = zmq_socket ( ID -> CONTEXT, ZMQ_REQ );
    int rc = zmq_connect ( ID -> OS, ID -> OSN );
    assert (rc == 0); // perror
    rc = zmq_bind ( ID -> IS, ID -> ISN );
    assert (rc == 0); // perror

    //printf( "child %d initialized\n", ID.MY_ID );
} 

void deinitialize( IDCard* ID ){ // on signals
    zmq_close(ID -> OS);
    zmq_close(ID -> IS);
    zmq_ctx_destroy (ID -> CONTEXT);
}
/*
void recreateOutput(){
    zmq_close (ID.TOS);
        zmq_close (ID.TIS);
        ID.HOS = zmq_socket ( ID.CONTEXT, ZMQ_REQ );
        ID.HIS = zmq_socket ( ID.CONTEXT, ZMQ_REP );
        int rc = zmq_connect ( ID.TOS, ID.TOSN );
        assert (rc == 0); // perror
        rc = zmq_connect ( ID.TIS, ID.TISN );
        assert (rc == 0); // perror
}
void reconnect( message* m ){
    //printf("starting reconnect\n");
    int rc = zmq_unbind( ID.HIS, ID.HISN );
    assert (rc == 0);
    rc = zmq_unbind( ID.HOS, ID.HOSN );
    assert (rc == 0);
    //printf("child %d Disconect %s\n", ID.MY_ID, ID.HISN );
    
    ID.HOS = zmq_socket ( ID.CONTEXT, ZMQ_REQ );
    ID.HIS = zmq_socket ( ID.CONTEXT, ZMQ_REP );
    sleep(2);
    char* tok = strtok( m -> data , " ");
    //printf( "%d reconnect HIS to %s\n", ID.MY_ID,tok );
    rc = zmq_bind( ID.HIS, m -> data );
    assert (rc == 0);
    
    strcpy( ID.HISN, tok );
    
    tok = strtok( NULL , " ");
    //printf( "%d reconnect HOS to %s}\n", ID.MY_ID,tok );
    rc = zmq_bind( ID.HOS, tok );
    assert (rc == 0);
    
    strcpy( ID.HOSN, tok );
    //printf("child %d binded to %s\n", ID.MY_ID, ID.HISN );

}
*/

void forward( message* m ){
    zmq_msg_t answer;
    zmq_messageInit( &answer, m -> sender, m ->recipient, ID.MY_ID, m -> type, m -> data, m -> moreData, m -> messageID );
    printf("%d Forward from %d to %d mess: %s \n", ID.MY_ID,m -> sender , m ->recipient, m -> data );
    zmq_msg_send( &answer, ID.OS, 0 );
    zmq_msg_close( &answer );
    sleep(1);
    //printf("%d wait complete forword to %d\n", ID.MY_ID , m ->recipient );
    zmq_msg_t rep;
    zmq_msg_recv(  &rep, ID.OS, 0 );
    zmq_msg_close( &rep );
    //printf(" %d Forward ended from %d to %d %s\n", ID.MY_ID , m -> sender , m ->recipient, m -> data );
}

int i = 1;

void calculate( message* m ){
    message to_send;
    char ans[BUF_SIZE];
    sprintf( ans, "Answer %d from %d", i, ID.MY_ID );
    messageInit( &to_send, ID.MY_ID, m -> sender, ID.MY_ID, ANSWER, ans, 0, 0 );
    forward( &to_send );
}


void parseMessage( message* m, int from  ){
    //printf( "Child %d received mess: s %d r %d data %s\n", ID.MY_ID, m -> sender, m -> recipient, m -> data );
    if ( ID.MY_ID != m -> recipient && m -> recipient != TO_ALL && m -> recipient != FIRST_WATCHED ){ // first watching message
        //printf( "dirrect %d\n", from);
        forward( m );
        return;
    }
    if ( m -> type == CALCULATE ){
        calculate( m );
    }
}

void answer(){
    sleep(1);
    zmq_msg_t toAnswer;
    char answer[] = "Forworded";
    zmq_messageInit( &toAnswer, ID.MY_ID, ID.MY_ID, ID.MY_ID, REPLY, answer, 0, 0 );
    zmq_msg_send( &toAnswer, ID.IS, 0 );
    zmq_msg_close(&toAnswer);
    //printf( "child %d answered \n", ID.MY_ID  );
}

void cycle (){
    message data;
    while ( 1 ){
        zmq_msg_t received;
        zmq_msg_init_size( &received, sizeof(message) );
        //printf( "id %d hisn %s tisn %s\n\t hosn %s tosn %s\n", ID.MY_ID, ID.HISN, ID.TISN, ID.HOSN, ID.TOSN );
        printf( "child %d waiting task\n", ID.MY_ID );
        zmq_msg_recv( &received, ID.IS, 0 );
        //if ( errno != EAGAIN ){
            memcpy( &data, zmq_msg_data(&received),  sizeof(message));
            zmq_close( &received );
            answer();
            parseMessage( &data, 1 );

        //}
        
            
        
        /*
        zmq_recv( ID.HIS, &received, sizeof( received ), ZMQ_DONTWAIT );
        if ( errno != EAGAIN ){
            memcpy( &data, zmq_msg_data(&received),  sizeof(message));
            printf("%d gain from head\n", ID.MY_ID);    
            answer( ID.HIS, &data );
            parseMessage( &data, 0 );
            
        }
        */
        //sleep(1);
    }
}


int main (int argc, char* argv[] ){
    getIDCard( &ID, argc, argv );
    initialize( &ID );
    time_t t = time(NULL);
    printf( "%d ready %ld\n", ID.MY_ID,t );
    cycle();
    deinitialize( &ID );
    return 0;
}