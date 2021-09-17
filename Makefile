.PHONY=clean all
COMPILER=gcc
CFLAGS = -Wall -fsanitize=address
all: addrinfo tcpEchoClient tcpEchoServer udpEchoServer udpEchoClient tcpEchoAddrinfo
clean:	
	- rm -f *.o  addrinfo tcpEchoclient tcpEchoServer udpEchoServer udpEchoClient tcpEchoAddrinfo

COMMON =  logger.c util.c
addrinfo:      
	$(COMPILER) $(CFLAGS) -o addrinfo addrinfo.c $(COMMON) 
tcpEchoClient:      
	$(COMPILER) $(CFLAGS) -o tcpEchoClient tcpEchoClient.c tcpClientUtil.c $(COMMON)
tcpEchoServer:      
	$(COMPILER) $(CFLAGS) -o tcpEchoServer tcpEchoServer.c tcpServerUtil.c $(COMMON)
udpEchoServer:      
	$(COMPILER) $(CFLAGS) -o udpEchoServer udpEchoServer.c $(COMMON)
udpEchoClient:
	$(COMPILER) $(CFLAGS) -o udpEchoClient udpEchoClient.c $(COMMON)
tcpEchoAddrinfo:      
	$(COMPILER) $(CFLAGS) -o tcpEchoAddrinfo tcpEchoAddrinfo.c tcpServerUtil.c $(COMMON)
