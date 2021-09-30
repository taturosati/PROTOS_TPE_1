#ifndef UTIL_H_
#define UTIL_H_

#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>

#define DATE_ES 0
#define DATE_EN 1


int print_socket_address(const struct sockaddr* address, char* addrBuffer);

const char* print_family(struct addrinfo* aip);
const char* print_type(struct addrinfo* aip);
const char* print_protocol(struct addrinfo* aip);
void print_flags(struct addrinfo* aip);
char* print_address_port(const struct addrinfo* aip, char addr[]);
int get_date(int date_format, char date[12]);
int get_time(char time_str[10]);

// Determina si dos sockets son iguales (misma direccion y puerto)
int sock_address_equal(const struct sockaddr* addr1, const struct sockaddr* addr2);

#endif 
