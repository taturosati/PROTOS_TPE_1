#ifndef UTIL_H_
#define UTIL_H_

#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>

#define DATE_ES 0
#define DATE_EN 1


int printSocketAddress(const struct sockaddr* address, char* addrBuffer);

const char* printFamily(struct addrinfo* aip);
const char* printType(struct addrinfo* aip);
const char* printProtocol(struct addrinfo* aip);
void printFlags(struct addrinfo* aip);
char* printAddressPort(const struct addrinfo* aip, char addr[]);
int getDate(int date_format, char date[12]);
int getTime(char time_str[10]);

// Determina si dos sockets son iguales (misma direccion y puerto)
int sockAddrsEqual(const struct sockaddr* addr1, const struct sockaddr* addr2);

#endif 
