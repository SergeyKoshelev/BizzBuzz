#ifndef _myserver_h
#define _myserver_h

#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define PATH "/tmp/mysock"
#define BUFSZ 256

#define DUMMY_STR "Putin is the president of Russia"

#define PRINT "PRINT"
#define EXIT "EXIT"

#endif