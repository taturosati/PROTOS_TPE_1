#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include <sys/select.h>
#include <logger.h>
#include <tcp_server_util.h>
#include <util.h>
#include <parser_utils.h>
#include <parser.h>
#include <ctype.h>

#define max(n1,n2)     ((n1)>(n2) ? (n1) : (n2))

#define TRUE   1
#define FALSE  0
#define PORT 9999
#define MAX_SOCKETS 30
#define BUFFSIZE 1024
#define MAX_PENDING_CONNECTIONS   3    // un valor bajo, para realizar pruebas
#define MIN_PORT 1024

#define TCP_COMMANDS 3
#define UDP_COMMANDS 3


#define US_ASCII(x) ((x < 0 || x > 127) ? (0) : (1))

typedef enum { PARSING = 0, EXECUTING, INVALID, IDLE } action;

typedef enum { ECHO_C = 0, TIME, DATE } command;

typedef struct t_buffer* t_buffer_ptr;

//  Se encarga de escribir la respuesta faltante en forma no bloqueante
void handle_write(int socket, t_buffer_ptr buffer, fd_set* writefds);


//  Limpia el buffer de escritura asociado a un socket
void clear(t_buffer_ptr buffer);

//Crea y "bindea" el socket server UDP
int udp_socket(int port);
