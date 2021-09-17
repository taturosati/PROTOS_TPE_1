#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "logger.h"
#include "tcpClientUtil.h"

#define BUFSIZE 512

int main(int argc, char *argv[]) {

	if (argc != 4) {
		log(FATAL, "usage: %s <Server Name/Address> <Echo Word> <Server Port/Name>", argv[0]);
	}

	char *server = argv[1];     // First arg: server name IP address 
	char *echoString = argv[2]; // Second arg: string to echo

	// Third arg server port
	char * port = argv[3];

	// Create a reliable, stream socket using TCP
	int sock = tcpClientSocket(server, port);
	if (sock < 0) {
		log(FATAL, "socket() failed")
	}

	size_t echoStringLen = strlen(echoString); // Determine input length

	// Send the string to the server
	ssize_t numBytes = send(sock, echoString, echoStringLen, 0);
	if (numBytes < 0 || numBytes != echoStringLen)
		log(FATAL, "send() failed expected %zu sent %zu", echoStringLen, numBytes);

	// Receive the same string back from the server
	unsigned int totalBytesRcvd = 0; // Count of total bytes received
	log(INFO, "Received: ")     // Setup to print the echoed string
	while (totalBytesRcvd < echoStringLen && numBytes >=0) {
		char buffer[BUFSIZE]; 
		/* Receive up to the buffer size (minus 1 to leave space for a null terminator) bytes from the sender */
		numBytes = recv(sock, buffer, BUFSIZE - 1, 0);
		if (numBytes < 0) {
			log(ERROR, "recv() failed")
		}  
		else if (numBytes == 0)
			log(ERROR, "recv() connection closed prematurely")
		else {
			totalBytesRcvd += numBytes; // Keep tally of total bytes
			buffer[numBytes] = '\0';    // Terminate the string!
			log(INFO, "%s", buffer);      // Print the echo buffer
		}
	}

	close(sock);
	return 0;
}
