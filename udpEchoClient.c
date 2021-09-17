#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include "logger.h"
#include "util.h"


/* En esta version no iteramos por las posibles IPs del servidor Echo, como se hizo para TCP
** Realizar las modificaciones necesarias para que intente por todas las IPs
*/
int udpClientSocket(const char *host, const char *service, struct addrinfo **servAddr) {
  // Pedimos solamente para UDP, pero puede ser IPv4 o IPv6
  struct addrinfo addrCriteria;                   
  memset(&addrCriteria, 0, sizeof(addrCriteria)); 
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

  // Tomamos la primera de la lista
  int rtnVal = getaddrinfo(host, service, &addrCriteria, servAddr);
  if (rtnVal != 0) {
    log(FATAL, "getaddrinfo() failed: %s", gai_strerror(rtnVal));
	return -1;
  }

  // Socket cliente UDP
  return socket((*servAddr)->ai_family, (*servAddr)->ai_socktype, (*servAddr)->ai_protocol); // Socket descriptor for client
  
}

int main(int argc, char *argv[]) {

  if (argc < 3 || argc > 4) 
    log(FATAL, "Usage: %s <Server Address/Name> <Echo Word> <Server Port/Service>", argv[0]);

  // A diferencia de TCP, guardamos a que IP/puerto se envia la data, para verificar
  // que la respuesta sea del mismo host
  struct addrinfo *servAddr; 
  
  char *server = argv[1];     
  char *echoString = argv[2]; 

  ssize_t echoStringLen = strlen(echoString);

  char *servPort = (argc == 4) ? argv[3] : "echo";

  errno = 0;
  int sock = udpClientSocket(server, servPort, &servAddr);
  if (sock < 0)
    log(FATAL, "socket() failed: %s", strerror(errno));

  // Enviamos el string
  ssize_t numBytes = sendto(sock, echoString, echoStringLen, 0, servAddr->ai_addr, servAddr->ai_addrlen);
  if (numBytes < 0) {
    log(FATAL, "sendto() failed: %s", strerror(errno))
  } else if (numBytes != echoStringLen) {
    log(FATAL, "sendto() error, sent unexpected number of bytes");
  }

  // Guardamos la direccion/puerto de respuesta para verificar que coincida con el servidor
  struct sockaddr_storage fromAddr; // Source address of server
  socklen_t fromAddrLen = sizeof(fromAddr);
  char buffer[echoStringLen + 1];                 
  
  // Establecemos un timeout de 5 segundos para la respuesta
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      log(ERROR, "setsockopt error: %s", strerror(errno))
  }

  numBytes = recvfrom(sock, buffer, echoStringLen, 0, (struct sockaddr *) &fromAddr, &fromAddrLen);
  if (numBytes < 0) {
    log(ERROR, "recvfrom() failed: %s", strerror(errno))
  } else {
    if (numBytes != echoStringLen)
    	log(ERROR, "recvfrom() error. Received unexpected number of bytes, expected:%zu received:%zu ", echoStringLen, numBytes);

    // "Autenticamos" la respuesta
    if (!sockAddrsEqual(servAddr->ai_addr, (struct sockaddr *) &fromAddr))
       log(ERROR, "recvfrom() received a packet from other source");

    buffer[numBytes] = '\0';     
    printf("Received: %s\n", buffer); 
  }
  freeaddrinfo(servAddr);
  close(sock);
  return 0;
}
