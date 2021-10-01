#ifndef TCPSERVERUTIL_H_
#define TCPSERVERUTIL_H_

#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <logger.h>
#include <util.h>
#include <defs.h>
#include <parser.h>
#include <command_handler.h>

// Create, bind, and listen a new TCP server socket
int setup_tcp_server_socket(int service);

// Accept a new TCP connection on a server socket
int accept_tcp_connection(int server_sock);

#endif
