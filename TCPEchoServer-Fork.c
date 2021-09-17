#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "logger.h"
#include "tcpServerUtil.h"

int
main(int argc, char *argv[]) {

	if (argc != 2) {
		log(FATAL, "usage: %s <Server Port>", argv[0]);
	}

  char *service = argv[1]; 
  
  	int servSock = setupTCPServerSocket(servPort);
	if (servSock < 0 )
		return 1;

  unsigned int childProcCount = 0; // Number of child processes
  while ( 1 ) {
    
    // Wait for a client to connect
	int clntSock = acceptTCPConnection(servSock);
    
    // Fork child process and report any errors
    pid_t processID = fork();
    if (processID < 0)
      log(ERROR, "fork() failed");
    else if (processID == 0) {      // If this is the child process
        close(servSock);            // Child closes parent socket
      	handleTCPEchoClient(clntSock);
        exit(0);                    // Child process terminates
    }

    log(DEBUG, "fork child process: %d\n", processID);
    close(clntSock);        // Parent closes child socket descriptor
    childProcCount++;       // Increment number of child processes

    while (childProcCount) {                // Clean up all zombies
      processID = waitpid((pid_t) - 1, NULL, WNOHANG); // Non-blocking wait
      if (processID < 0)                    // waitpid() error?
        log(ERROR, "waitpid() failed");
      else if (processID == 0)              // No zombie to wait on
        break;
      else
        childProcCount--;                   // Cleaned up after a child
    }
  }
}
