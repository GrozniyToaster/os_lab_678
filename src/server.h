#pragma once
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

#define min(a,b) ((a) < (b) ? (a) : (b))

extern int COUNT_CHILD;
extern int CHILDS[MAX_CHILD];
extern char TEMP_DIR[19];
extern const char CHILD_NAME[9];
extern IDCard ID;

void initialize( IDCard* ID ); 

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

int rmrf(char *path);

void deinitialize( IDCard* ID );

void recreateOutput();

void prepare_to_transfer( char* data, int* buf, int count_tok );


