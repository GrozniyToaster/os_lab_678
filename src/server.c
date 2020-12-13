#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <ftw.h>
#include <string.h>
#include <time.h>
#include <zmq.h>
#include <stdbool.h>
#include <ctype.h>

#include "structs.h"

#define MAX_CHILD 54
#define MAX_INTS_IN_PACKAGE 25

int COUNT_CHILD = 0;
int CHILDS[MAX_CHILD];
char TEMP_DIR[] = "/tmp/lab678.XXXXXX";
const char CHILD_NAME[] = "./client";
IDCard ID;


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


void child_init( int id ){
    if ( CHILDS[id] != -1 ){
        printf("Error: Already exists\n");
        return;
    }
    if ( COUNT_CHILD != 0 ){
        zmq_msg_t toRebuild;
        char sockIn[BUF_SIZE];
        sprintf( sockIn, "ipc://%s/%d.out", TEMP_DIR, id ); 
        zmq_messageInit( &toRebuild, ID.MY_ID, FIRST_WATCHED, MAIN_MODULE, RECONNECT, sockIn, 0, 0 );
        zmq_msg_send( &toRebuild, ID.OS,  0 );
        zmq_msg_close( &toRebuild );
        sleep(1);
        recreateOutput();
    }
    
    char argID[3];
    char arg_socket_in[BUF_SIZE];
    char arg_socket_out[BUF_SIZE];
    char arg_socket_ping[BUF_SIZE];
    sprintf(argID, "%d", id );
    sprintf(arg_socket_in, "%s", ID.OSN  );
    sprintf(arg_socket_out, "ipc://%s/%d.out", TEMP_DIR, id );
    sprintf(arg_socket_ping, "%s", ID.PSN  );

    int childPID = fork();
    if ( childPID == -1 ){
        printf("Fork err\n");
        return;
    }else if ( childPID == 0 ){
        execl( CHILD_NAME, argID, arg_socket_in, arg_socket_out, arg_socket_ping, NULL );
    }else{
        CHILDS[ id ] = childPID;
        COUNT_CHILD++;
    }
    printf( "Ok:%d\n", childPID );
    return;
}


void ping(){
    zmq_msg_t ping_message;
    char mes[] = "PING_TO_ALL";
    zmq_messageInit( &ping_message, MAIN_MODULE, TO_ALL, MAIN_MODULE, PING, mes, 0, 0  );
    zmq_msg_send( &ping_message, ID.OS, 0 );
    int pings[MAX_CHILD];
    for ( int i = 0; i < MAX_CHILD; i++ ){
        pings[i] = -1;
    }
    message data;
    errno = 0;
    int i = 0;
    while( i < 10 ){
        sleep(1);
        zmq_msg_t p;
        zmq_msg_init( &p );
        int size_of_message = zmq_msg_recv( &p, ID.PS, ZMQ_DONTWAIT);
        if ( size_of_message <= 0 ){ 
            i++;
            continue;
        }
        memcpy( &data, zmq_msg_data(&p),  sizeof(message));
        pings[ data.messageID ] = 1;
        zmq_msg_close( &p );
        sleep(1);
        zmq_msg_t r;
        zmq_messageInit( &r, MAIN_MODULE, data.sender, MAIN_MODULE, ANSWER, mes, 0, 0 );
        zmq_msg_send( &r, ID.PS, 0 );
        zmq_msg_close( &r );
        i++;
    }
    int tokill[MAX_CHILD];
    int count_to_kill = 0;
    for ( int i = 0 ; i < MAX_CHILD; i++ ){
        if (  CHILDS[i] != -1 && pings[i] != 1 ){
            tokill[ count_to_kill ] = i;
            count_to_kill++;
        }
    }
    char res[BUF_SIZE];
    if ( count_to_kill == 0 ){
        printf( "Ok:-1\n" );
        return;
    }
    res[0] = '\0';
    for ( int i = 0; i < count_to_kill; i++ ){
        char tmp[5];
        sprintf( tmp, "%d;", tokill[i] );
        strcat( res, tmp );
        kill( CHILDS[ tokill[i] ] , SIGTERM );
        CHILDS[ tokill[i] ] = -1;
        COUNT_CHILD--; 
    }
    printf("Ok:%s\n", res );
}


void remove_node( int id ){
    if ( CHILDS[id] == -1 ){
        printf( "Error: %d Not found\n", id );
        return;
    }
    char kek[] = "You must die";
    zmq_msg_t rem_message;
    zmq_messageInit( &rem_message, MAIN_MODULE, id, MAIN_MODULE, DELETE, kek, 0, 0  );
    zmq_msg_send( &rem_message, ID.OS, 0 );
    CHILDS[id] = -1;
    COUNT_CHILD--;
    printf("Ok\n");
}

void prepare_to_transfer( char* data, int* buf, int count_tok ){
    printf( "prepare %d toks", count_tok );
    data[0] = '\0';
    for ( int i = 0 ; i < min ( count_tok, MAX_INTS_IN_PACKAGE ) ; i++ ){
        char tmp_string[15];
        sprintf( tmp_string, "%d ", buf[i] );
        strcat( data, tmp_string );
    }
}


void calculate( int id, int k, int* buf  ){
    if ( CHILDS[id] == -1 ){
        printf( "Error: %d Not found\n", id );
        return;
    }
    int start_letter_pos = 0;
    for( ; start_letter_pos < k - MAX_INTS_IN_PACKAGE; start_letter_pos += MAX_INTS_IN_PACKAGE ){
        zmq_msg_t calc_message;
        char data[BUF_SIZE];
        prepare_to_transfer( data, buf + start_letter_pos, MAX_INTS_IN_PACKAGE );
        zmq_messageInit( &calc_message, MAIN_MODULE, id, MAIN_MODULE, CALCULATE, data, MORE_DATA, 0  );
        zmq_msg_send( &calc_message, ID.OS, 0 );
    }
    zmq_msg_t calc_message;
    char data[BUF_SIZE];
    prepare_to_transfer( data, buf + start_letter_pos , k - start_letter_pos );
    zmq_messageInit( &calc_message, MAIN_MODULE, id, MAIN_MODULE, CALCULATE, data, 0, 0  );
    zmq_msg_send( &calc_message, ID.OS, 0 );
}

void preCalulate(){
    int id;
    int k;
    char* line = NULL;
    size_t  line_size = 0;
    scanf( "%d %d", &id, &k );
    getline( &line, &line_size, stdin );
    int* buf = malloc( k * sizeof(int) );
    int pos = 0;
    for( char* tok = strtok( line, " " ); tok != NULL; tok = strtok(NULL, " ") ){
        sscanf( tok, "%d", buf + pos  );
        pos++;
    }
    free( line );

    calculate( id, k, buf  );
}

void cycle(){
    char command[BUF_SIZE];
    while ( scanf("%s", command) != EOF ){
        if ( strcmp( command, "exec" ) == 0 ){
            preCalulate();
        }else if ( strcmp( command, "pingall" ) == 0 ){
            ping();
        }else if ( strcmp( command, "create" ) == 0 ){
            int id;
            scanf( "%d", &id );
            child_init( id );
            scanf( "%d", &id );
        }else if ( strcmp( command, "remove" ) == 0 ){
            int id;
            scanf( "%d", &id );
            remove_node( id );
        }else if ( strcmp( command, "exit" ) == 0 ){
            break;
        }else{
            printf("Unknown command\n");
        }
        sleep(1);
    }
}


int main (void){
    initialize( &ID );
    cycle();
    deinitialize( &ID );
    exit( EXIT_SUCCESS );
}