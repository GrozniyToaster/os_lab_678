#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <error.h>
#include <string.h>
#include <zmq.h>
#include "structs.h"


extern IDCard ID;
extern long long not_ended_result;

void deinitialize( IDCard* ID );
void process_sigterm(int32_t sig);

void getIDCard( IDCard* inf, const int argc,  char** argv );

void initialize( IDCard* ID );
void recreateInput();
void reconnect( message* m );
