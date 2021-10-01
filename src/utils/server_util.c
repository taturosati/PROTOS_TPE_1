#include <server_util.h>

#define MAXPENDING 5

int setup_server_socket(int service, unsigned protocol) {
	char srvc[6] = {0};

	if (sprintf(srvc, "%d", service) < 0) {
		log(FATAL, "invalid port");
		return -1;
	}

	struct addrinfo address_criteria;
	memset(&address_criteria, 0, sizeof(address_criteria));
	address_criteria.ai_family = AF_INET6;
	address_criteria.ai_flags = AI_PASSIVE; 

	if (protocol == IPPROTO_TCP) {
		address_criteria.ai_socktype = SOCK_STREAM;
		address_criteria.ai_protocol = IPPROTO_TCP;
	}
	else {
		address_criteria.ai_socktype = SOCK_DGRAM;
		address_criteria.ai_protocol = IPPROTO_UDP;
	}

	struct addrinfo* server_address;
	int rtnVal = getaddrinfo(NULL, srvc, &address_criteria, &server_address);
	if (rtnVal != 0) {
		log(FATAL, "getaddrinfo() failed %s", gai_strerror(rtnVal));
		return -1;
	}

	int server_sock = -1;
	for (struct addrinfo* addr = server_address; addr != NULL && server_sock == -1; addr = addr->ai_next) {
		errno = 0;
		server_sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (server_sock < 0) {
			continue; 
		}

		int no = 0;
		if (setsockopt(server_sock, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&no, sizeof(no)) < 0) {
			log(ERROR, "Set socket options failed");
			continue;
		}

		int can_bind = false;
		if (bind(server_sock, addr->ai_addr, addr->ai_addrlen) == 0) {
			can_bind = true;
			if ((protocol == IPPROTO_TCP) && (listen(server_sock, MAXPENDING) != 0)) {
				can_bind = false;
			}
		}
		if (!can_bind) {
			log(DEBUG, "Cant't bind %s", strerror(errno));
			close(server_sock);
			server_sock = -1;
		}
	}
	freeaddrinfo(server_address);

	return server_sock;
}

int accept_tcp_connection(int server_sock) {
	struct sockaddr_storage client_address; 
	socklen_t client_address_len = sizeof(client_address);

	int client_sock = accept(server_sock, (struct sockaddr*)&client_address, &client_address_len);
	if (client_sock < 0) {
		log(ERROR, "accept() failed");
		return -1;
	}
	return client_sock;
}