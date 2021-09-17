#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "logger.h" 
#include "util.h"
#define MAX_ADDR_BUFFER 128


int tcpClientSocket(const char *host, const char *service) {
	char addrBuffer[MAX_ADDR_BUFFER];
	struct addrinfo addrCriteria;                   // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;             // v4 or v6 is OK ---> ACA (*)
	addrCriteria.ai_socktype = SOCK_STREAM;         // Only streaming sockets -->Orientado a conexion
	addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol --> Si o si tcp

	// Get address(es)
	struct addrinfo *servAddr; // Holder for returned list of server addrs
                                                            //puntero donde me deja la lista
	int rtnVal = getaddrinfo(host, service, &addrCriteria, &servAddr);
	if (rtnVal != 0) {
		log(ERROR, "getaddrinfo() failed %s", gai_strerror(rtnVal))
		return -1;
	}

	int sock = -1;
	for (struct addrinfo *addr = servAddr; addr != NULL && sock == -1; addr = addr->ai_next) {
		// Create a reliable, stream socket using TCP
        //por cada estructura q nos devuelve tratamos de abrir el socket
        //no nos tenemos que preocupar por si es IPv4 o IPv6 porq ya lo hicimos arriba (* linea 16)
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (sock >= 0) {    //pudo crear el socket
			errno = 0;
			// Establish the connection to the server
            //intento hacer el connect
			if ( connect(sock, addr->ai_addr, addr->ai_addrlen) != 0) {
                //significa q no se pudo conectar y libero recursos
				log(INFO, "can't connectto %s: %s", printAddressPort(addr, addrBuffer), strerror(errno))
				close(sock); 	// Socket connection failed; try next address
				sock = -1;
			}
		} else {
			log(DEBUG, "Can't create client socket on %s",printAddressPort(addr, addrBuffer)) 
		}
	}

    //libero la lista que me devolvio addrinfo

	freeaddrinfo(servAddr); 
	return sock;
}