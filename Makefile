.PHONY=clean all
COMPILER=gcc
CFLAGS = -Wall -fsanitize=address
all: tcpEchoAddrinfo
clean:	
	- rm -f *.o tcpEchoAddrinfo

COMMON =  logger.c util.c
tcpEchoClient:      
	$(COMPILER) $(CFLAGS) -o tcpEchoClient tcpEchoClient.c tcpClientUtil.c $(COMMON)

tcpEchoAddrinfo:      
	$(COMPILER) $(CFLAGS) -o tcpEchoAddrinfo tcpEchoAddrinfo.c tcpServerUtil.c util.c $(COMMON)
