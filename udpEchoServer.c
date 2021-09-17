#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "logger.h"
#include "util.h"
#include <errno.h>

#define MAX_ADDR_BUFFER 128
#define MAXSTRINGLENGTH 64

static   char addrBuffer[MAX_ADDR_BUFFER];

/*
 ** Se encarga de resolver el número de puerto para service (puede ser un string con el numero o el nombre del servicio)
 ** y crear el socket UDP, para que escuche en cualquier IP, ya sea v4 o v6
 ** Funcion muy parecida a setupTCPServerSocket, solo cambia el tipo de servicio y que no es necesario invocar a listen()
 */
int setupUDPServerSocket(const char *service) {
	// Construct the server address structure
	struct addrinfo addrCriteria;                   // Criteria for address
	memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;             // Any address family
	addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
	addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram socket
	addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP socket

	struct addrinfo *servAddr; 			// List of server addresses
	int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
	if (rtnVal != 0)
		log(FATAL, "getaddrinfo() failed %s", gai_strerror(rtnVal))

	int servSock = -1;
	// Intentamos ponernos a escuchar en alguno de los puertos asociados al servicio, sin especificar una IP en particular
	// Iteramos y hacemos el bind por alguna de ellas, la primera que funcione, ya sea la general para IPv4 (0.0.0.0) o IPv6 (::/0) .
	// Con esta implementación estaremos escuchando o bien en IPv4 o en IPv6, pero no en ambas
	for (struct addrinfo *addr = servAddr; addr != NULL && servSock == -1; addr = addr->ai_next) {
		errno = 0;
		// Create UDP socket
		servSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (servSock < 0) {
			log(DEBUG, "Cant't create socket on %s : %s ", printAddressPort(addr, addrBuffer), strerror(errno));  
			continue;       // Socket creation failed; try next address
		}

		// Bind to ALL the address
		if (bind(servSock, addr->ai_addr, addr->ai_addrlen) == 0 ) {
			// Print local address of socket
			struct sockaddr_storage localAddr;
			socklen_t addrSize = sizeof(localAddr);
			if (getsockname(servSock, (struct sockaddr *) &localAddr, &addrSize) >= 0) {
				printSocketAddress((struct sockaddr *) &localAddr, addrBuffer);
				log(INFO, "Binding to %s", addrBuffer);
			}
		} else {
			log(DEBUG, "Cant't bind %s", strerror(errno));  
			close(servSock);  // Close and try with the next one
			servSock = -1;
		}
	}

	freeaddrinfo(servAddr);

	return servSock;
}


int main(int argc, char *argv[]) {

  if (argc != 2) {
     log(FATAL, "usage: %s <Server Port>", argv[0])
  }

  char *service = argv[1]; 	// First arg:  local port

  // Create socket for incoming connections
  int sock = setupUDPServerSocket(service);
  if (sock < 0)
    log(FATAL, "socket() failed: %s ", strerror(errno))

  for (;;) { 
    struct sockaddr_storage clntAddr; 			// Client address
    // Set Length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    // Block until receive message from a client
    char buffer[MAXSTRINGLENGTH];
 
    errno = 0;
    // Como alternativa a recvfrom se puede usar recvmsg, que es mas completa, por ejemplo permite saber
    // si el mensaje recibido es de mayor longitud a MAXSTRINGLENGTH
    ssize_t numBytesRcvd = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0, (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (numBytesRcvd < 0) {
      log(ERROR, "recvfrom() failed: %s ", strerror(errno))
      continue;
    } 

    printSocketAddress((struct sockaddr *) &clntAddr, addrBuffer);
    log(INFO, "Handling client %s, received %zu bytes", addrBuffer, numBytesRcvd);

    // Send received datagram back to the client
    ssize_t numBytesSent = sendto(sock, buffer, numBytesRcvd, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr));
    if (numBytesSent < 0)
      log(ERROR, "sendto() failed")
    else if (numBytesSent != numBytesRcvd)
      log(ERROR, "sendto() sent unexpected number of bytes");
  }
  // NOT REACHED
}
