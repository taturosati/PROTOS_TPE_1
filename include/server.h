#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> 
#include <sys/select.h>
#include <logger.h>
#include <server_util.h>
#include <util.h>
#include <parser_utils.h>
#include <parser.h>
#include <ctype.h>
#include <defs.h>
#include <command_handler.h>
#include <buffer.h>

#define max(n1,n2)     ((n1)>(n2) ? (n1) : (n2))

#define TRUE   1
#define FALSE  0
#define PORT 9999
#define MAX_SOCKETS 30
#define MAX_PENDING_CONNECTIONS 3
#define MIN_PORT 1024

#endif