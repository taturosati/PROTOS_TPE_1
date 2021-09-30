#ifndef TCPSERVERUTIL_H_
#define TCPSERVERUTIL_H_

#include <stdio.h>
#include <sys/socket.h>


// Create, bind, and listen a new TCP server socket
int setup_tcp_server_socket(int service);

// Accept a new TCP connection on a server socket
int accept_tcp_connection(int server_sock);

#endif 
