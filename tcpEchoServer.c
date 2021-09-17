#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "logger.h"
#include "tcpServerUtil.h"


int main(int argc, char *argv[]) {

	if (argc != 2) {
		log(FATAL, "usage: %s <Server Port>", argv[0]);
	}

	char * servPort = argv[1];

	int servSock = setupTCPServerSocket(servPort);
	if (servSock < 0 )
		return 1;

	while (1) { // Run forever
		// Wait for a client to connect
		int clntSock = acceptTCPConnection(servSock);
		if (clntSock < 0)
			log(ERROR, "accept() failed")
		else {
			handleTCPEchoClient(clntSock);
		}
	}
}
