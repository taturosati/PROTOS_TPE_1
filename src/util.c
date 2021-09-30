#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <logger.h>
#include <util.h>

char*
print_address_port(const struct addrinfo* aip, char addr[])
{
	char abuf[INET6_ADDRSTRLEN];
	const char* addrAux;
	if (aip->ai_family == AF_INET) {
		struct sockaddr_in* sinp;
		sinp = (struct sockaddr_in*)aip->ai_addr;
		addrAux = inet_ntop(AF_INET, &sinp->sin_addr, abuf, INET_ADDRSTRLEN);
		if (addrAux == NULL)
			addrAux = "unknown";
		strcpy(addr, addrAux);
		if (sinp->sin_port != 0) {
			sprintf(addr + strlen(addr), ": %d", ntohs(sinp->sin_port));
		}
	}
	else if (aip->ai_family == AF_INET6) {
		struct sockaddr_in6* sinp;
		sinp = (struct sockaddr_in6*)aip->ai_addr;
		addrAux = inet_ntop(AF_INET6, &sinp->sin6_addr, abuf, INET6_ADDRSTRLEN);
		if (addrAux == NULL)
			addrAux = "unknown";
		strcpy(addr, addrAux);
		if (sinp->sin6_port != 0)
			sprintf(addr + strlen(addr), ": %d", ntohs(sinp->sin6_port));
	}
	else
		strcpy(addr, "unknown");
	return addr;
}


int
print_socket_address(const struct sockaddr* address, char* addrBuffer) {

	void* numericAddress;

	in_port_t port;

	switch (address->sa_family) {
	case AF_INET:
		numericAddress = &((struct sockaddr_in*)address)->sin_addr;
		port = ntohs(((struct sockaddr_in*)address)->sin_port);
		break;
	case AF_INET6:
		numericAddress = &((struct sockaddr_in6*)address)->sin6_addr;
		port = ntohs(((struct sockaddr_in6*)address)->sin6_port);
		break;
	default:
		strcpy(addrBuffer, "[unknown type]");    // Unhandled type
		return 0;
	}
	// Convert binary to printable address
	if (inet_ntop(address->sa_family, numericAddress, addrBuffer, INET6_ADDRSTRLEN) == NULL)
		strcpy(addrBuffer, "[invalid address]");
	else {
		if (port != 0)
			sprintf(addrBuffer + strlen(addrBuffer), ":%u", port);
	}
	return 1;
}


int get_date(int date_format, char date[12]) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int chars_read;
	if (date_format == DATE_EN)
		chars_read = sprintf(date, "%02d/%02d/%d\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
	else
		chars_read = sprintf(date, "%02d/%02d/%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

	if (chars_read < 0) {
		return -1;
	}
	else
		return chars_read;
}
int get_time(char time_str[10]) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int chars_read;
	if ((chars_read = sprintf(time_str, "%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec)) < 0)
		return -1;
	else
		return chars_read;
}
