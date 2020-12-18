#include "client.h"


IDCard ID;
long long not_ended_result = 0;

void forward( message* m ){
    zmq_msg_t answer;
    zmq_messageInit( &answer, m -> sender, m -> recipient, ID.MY_ID, m -> type, m -> data, m -> moreData, m -> messageID );
    zmq_msg_send( &answer, ID.OS, 0 );
    zmq_msg_close( &answer );
}

void ping(){
    zmq_msg_t ping_message;
    char mes [BUF_SIZE];
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
    exit( EXIT_SUCCESS );
}

void parseMessage( message* m ){
    if( m -> recipient != ID.MY_ID && 
        m -> recipient != TO_ALL && 
        m -> recipient != FIRST_WATCHED ){ 
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
        zmq_msg_t received;
        zmq_msg_init_size( &received, sizeof(message) );
        zmq_setsockopt(ID.IS, ZMQ_SUBSCRIBE, 0, 0);
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