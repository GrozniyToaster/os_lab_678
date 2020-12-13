#include "server.h"

void prepare_to_transfer( char* data, int* buf, int count_tok ){
    data[0] = '\0';
    for ( int i = 0 ; i < min ( count_tok, MAX_INTS_IN_PACKAGE ) ; i++ ){
        char tmp_string[15];
        sprintf( tmp_string, "%d ", buf[i] );
        strcat( data, tmp_string );
    }
}

void initialize( IDCard* ID ){
    char *dir_name = mkdtemp(TEMP_DIR);

    if(dir_name == NULL){
        printf("Error: Cant create temp directory\n");
        exit( EXIT_FAILURE );
    }

    sprintf( ID -> OSN, "ipc://%s/master.out", TEMP_DIR  );
    sprintf( ID -> PSN, "ipc://%s/ping", TEMP_DIR  );

    ID -> MY_ID = MAIN_MODULE;
    ID -> CONTEXT = zmq_ctx_new ();

    ID -> OS = zmq_socket( ID -> CONTEXT, ZMQ_PUB );
    int rc = zmq_bind( ID -> OS, ID -> OSN );
    assert (rc == 0);

    ID -> PS = zmq_socket( ID -> CONTEXT, ZMQ_REP );
    rc = zmq_bind( ID -> PS, ID -> PSN );
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
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS); 
}

void deinitialize( IDCard* ID ){
    for ( int i = 0; i < MAX_CHILD; ++i ){
        if ( CHILDS[i] != -1 ){
            kill( CHILDS[i] , SIGTERM );
        }
    }
    zmq_close(ID -> OS );
    zmq_close(ID -> PS );
    zmq_ctx_destroy (ID -> CONTEXT);
    rmrf( TEMP_DIR );
}

void recreateOutput(){
    zmq_close (ID.OS);
    sleep(1);
    ID.OS = zmq_socket ( ID.CONTEXT, ZMQ_PUB );
    int rc = zmq_bind( ID.OS, ID.OSN );
    assert (rc == 0);
}