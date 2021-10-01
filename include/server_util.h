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

int setup_server_socket(int service, unsigned protocol);

int accept_tcp_connection(int server_sock);

#endif
