#include "client.h"



void deinitialize( IDCard* ID ){ 
    zmq_close(ID -> OS);
    zmq_close(ID -> IS);
    zmq_close(ID -> PS);
    zmq_ctx_destroy (ID -> CONTEXT);
}

void process_sigterm(int32_t sig) {
    deinitialize( &ID );
    exit(EXIT_SUCCESS);
}


void getIDCard( IDCard* inf, const int argc,  char** argv ){
    if ( argc != 4  ){
        printf("Usage <id> <in socket> <out socket> <ping socket>\n");
        exit ( EXIT_FAILURE );
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
        printf("Error: child %d signal not set\n", ID -> MY_ID);
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
