#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <error.h>
#include <string.h>
#include <zmq.h>
#include "structs.h"

#include <time.h>

IDCard ID;

long long not_ended_result = 0;

void deinitialize( IDCard* ID ){ // on signals
    zmq_close(ID -> OS);
    zmq_close(ID -> IS);
    zmq_close(ID -> PS);
    zmq_ctx_destroy (ID -> CONTEXT);
}

void process_sigterm(int32_t sig) {
    printf("Computing node %d finishing it's work...\n", ID.MY_ID);
    deinitialize( &ID );
    exit(EXIT_SUCCESS);
}


void getIDCard( IDCard* inf, const int argc,  char** argv ){
    if ( argc != 4  ){
        printf("Usage <id> <in socket> <out socket> <ping socket>\n");
        exit (1);
    }
    inf -> MY_ID = atoi( argv[0] );
    strcpy( inf -> ISN, argv[1] );
    strcpy( inf -> OSN, argv[2] );
    strcpy( inf -> PSN, argv[3] );
}



void initialize( IDCard* ID ){
    ID -> CONTEXT = zmq_ctx_new ();
    ID -> IS = zmq_socket ( ID -> CONTEXT, ZMQ_SUB );
    ID -> OS = zmq_socket ( ID -> CONTEXT, ZMQ_PUB );
    ID -> PS = zmq_socket ( ID -> CONTEXT, ZMQ_REQ );
    int rc = zmq_connect ( ID -> IS, ID -> ISN );
    zmq_setsockopt(ID -> IS, ZMQ_SUBSCRIBE, 0, 0);
    assert (rc == 0); 
    
    rc = zmq_bind ( ID -> OS, ID -> OSN );
    assert (rc == 0); 

    rc = zmq_connect ( ID -> PS, ID -> PSN );
    assert (rc == 0); 

    if (signal(SIGTERM, process_sigterm) == SIG_ERR) {
        printf("err\n");
        exit( EXIT_FAILURE );
    }


} 

void recreateInput(){
    zmq_close (ID.IS);
    ID.IS = zmq_socket ( ID.CONTEXT, ZMQ_SUB );
    int rc = zmq_connect ( ID.IS, ID.ISN );
    assert (rc == 0); 
}

void reconnect( message* m ){
    sprintf( ID.ISN, "%s", m -> data  );
    recreateInput();
    sleep(1);
}


void forward( message* m ){
    zmq_msg_t answer;
    zmq_messageInit( &answer, m -> sender, m ->recipient, ID.MY_ID, m -> type, m -> data, m -> moreData, m -> messageID );
    //printf("%d Forward from %d to %d mess: %s \n", ID.MY_ID, m -> sender , m ->recipient, m -> data );
    zmq_msg_send( &answer, ID.OS, 0 );
    zmq_msg_close( &answer );

}

void ping(){
    zmq_msg_t ping_message;
    char mes [BUF_SIZE];
    printf( "%d is Alive\n", ID.MY_ID );
    sprintf( mes,"%d is Alive", ID.MY_ID ); 
    zmq_messageInit( &ping_message, ID.MY_ID, MAIN_MODULE, ID.MY_ID, ANSWER, mes, 0, ID.MY_ID );
    zmq_msg_send( &ping_message, ID.PS, 0 );
    zmq_msg_close( &ping_message );
    sleep(1);
    zmq_msg_t p;
    zmq_msg_init(&p);
    zmq_msg_recv( &p, ID.PS, 0 );
    zmq_msg_close( &p );
}

void calculate( message* m ){
    int cur_token = 0;
    int res = 0;
    printf("CAl %d\n", ID.MY_ID);
    char* tok = strtok( m -> data, " " );
    while ( tok != NULL ){
        cur_token = atoi( tok );
        res += cur_token;
        tok = strtok( NULL, " " );
    }
    if ( m -> moreData ){
        not_ended_result += res;
    }else{
        res += not_ended_result;
        not_ended_result = 0;
        printf("Ok:%d:%d\n", ID.MY_ID, res);
    }

}


void delete(){
    zmq_msg_t toRebuild;
    zmq_messageInit( &toRebuild, ID.MY_ID, FIRST_WATCHED, ID.MY_ID, RECONNECT, ID.ISN, 0, 0 );
    zmq_msg_send( &toRebuild, ID.OS,  0 );
    zmq_msg_close( &toRebuild );
    sleep(1);
    deinitialize( &ID );
    exit(  EXIT_SUCCESS );
}

void parseMessage( message* m ){
    printf( "Child %d received mess: s %d r %d data %s whith type %d \n", ID.MY_ID, m -> sender, m -> recipient, m -> data , m -> type );
    if ( ID.MY_ID != m -> recipient && m -> recipient != TO_ALL && m -> recipient != FIRST_WATCHED ){ // first watching message
        forward( m );
        return;
    }
    if ( m -> recipient == TO_ALL ){
        forward( m );
    }
    if ( m -> type == RECONNECT ){
        reconnect( m );
    }else if ( m -> type == CALCULATE ){
        calculate( m );
    }else if ( m -> type == PING ){
        ping();
    }else if ( m -> type == DELETE ){
        delete();
    }
    
}

void cycle (){
    message data;
    while ( 1 ){
        //printf( "%d is %s os %s\n", ID.MY_ID, ID.ISN, ID.OSN );
        zmq_msg_t received;
        zmq_msg_init_size( &received, sizeof(message) );
        zmq_setsockopt(ID.IS, ZMQ_SUBSCRIBE, 0, 0);
        //printf( "id %d hisn %s tisn %s\n\t hosn %s tosn %s\n", ID.MY_ID, ID.HISN, ID.TISN, ID.HOSN, ID.TOSN );
        //printf( "child %d waiting task\n", ID.MY_ID );
        zmq_msg_recv( &received, ID.IS, 0 );
        memcpy( &data, zmq_msg_data(&received),  sizeof(message));
        zmq_close( &received );
        parseMessage( &data );
    }
}


int main (int argc, char* argv[] ){
    getIDCard( &ID, argc, argv );
    initialize( &ID );
    cycle();
    deinitialize( &ID );
    return 0;
}