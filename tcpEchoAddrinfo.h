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
#include "logger.h"
#include "tcpServerUtil.h"

#define max(n1,n2)     ((n1)>(n2) ? (n1) : (n2))

#define TRUE   1
#define FALSE  0
#define PORT 8888
#define MAX_SOCKETS 30
#define BUFFSIZE 1024
#define PORT_UDP 8888
#define MAX_PENDING_CONNECTIONS   3    // un valor bajo, para realizar pruebas

typedef struct t_buffer * t_buffer_ptr;

/**
  Se encarga de escribir la respuesta faltante en forma no bloqueante
  */
void handleWrite(int socket, t_buffer_ptr buffer, fd_set * writefds);
/**
  Limpia el buffer de escritura asociado a un socket
  */
void clear(t_buffer_ptr buffer);

/**
  Crea y "bindea" el socket server UDP
  */
int udpSocket(int port);

/**
  Lee el datagrama del socket, obtiene info asociado con getaddrInfo y envia la respuesta
  */
void handleAddrInfo(int socket);
