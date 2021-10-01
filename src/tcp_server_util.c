#include <tcp_server_util.h>

#define MAXPENDING 5 // Maximum outstanding connection requests
#define BUFSIZE 256
#define MAX_ADDR_BUFFER 128

static char address_buffer[MAX_ADDR_BUFFER];
/*
 ** Se encarga de resolver el número de puerto para service (puede ser un string con el numero o el nombre del servicio)
 ** y crear el socket pasivo, para que escuche en cualquier IP, ya sea v4 o v6
 */
int setup_server_socket(int service, unsigned protocol)
{
	char srvc[6] = {0};

	if (sprintf(srvc, "%d", service) < 0)
	{
		log(FATAL, "invalid port");
		return -1;
	}

	// Construct the server address structure
	struct addrinfo address_criteria;						// Criteria for address match
	memset(&address_criteria, 0, sizeof(address_criteria)); // Zero out structure
	address_criteria.ai_family = AF_INET6;					// Any address family
	address_criteria.ai_flags = AI_PASSIVE;					// Accept on any address/port
	
	if(protocol == IPPROTO_TCP){
		address_criteria.ai_socktype = SOCK_STREAM;
		address_criteria.ai_protocol = IPPROTO_TCP;
	} else{
		address_criteria.ai_socktype = SOCK_DGRAM;
		address_criteria.ai_protocol = IPPROTO_UDP;
	}

	struct addrinfo *server_address;										  // List of server addresses
	int rtnVal = getaddrinfo(NULL, srvc, &address_criteria, &server_address); //ME DEVUELVE LA LISTA DE LAS DIRECCIONES CON LOS REQUISITOS QUE PEDI
	if (rtnVal != 0)
	{
		log(FATAL, "getaddrinfo() failed %s", gai_strerror(rtnVal));
		return -1;
	}

	int server_sock = -1;
	for (struct addrinfo *addr = server_address; addr != NULL && server_sock == -1; addr = addr->ai_next)
	{
		errno = 0;
		// Create a TCP socket
		server_sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (server_sock < 0)
		{
			// log(DEBUG, "Cant't create socket on %s : %s ", print_address_port(addr, address_buffer), strerror(errno));
			continue; // Socket creation failed; try next address
		}

		int no = 0;
		if (setsockopt(server_sock, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)) < 0)
		{
			log(ERROR, "Set socket options failed");
			continue;
		}

		int can_bind = false;
		// Bind to ALL the address and set socket to listen
		if (bind(server_sock, addr->ai_addr, addr->ai_addrlen) == 0) {
			can_bind = true;
			if( (protocol == IPPROTO_TCP) && (listen(server_sock, MAXPENDING) != 0)){
				can_bind = false;
			}
		}
		if(can_bind){
			// Print local address of socket
			struct sockaddr_storage localAddr;
			socklen_t addrSize = sizeof(localAddr);
			if (getsockname(server_sock, (struct sockaddr *)&localAddr, &addrSize) >= 0)
			{
				// print_socket_address((struct sockaddr*)&localAddr, address_buffer);
				log(INFO, "Binding to %s", address_buffer);
			}
		} else {
			log(DEBUG, "Cant't bind %s", strerror(errno));
			close(server_sock); // Close and try with the next one
			server_sock = -1;
		}
	}
	freeaddrinfo(server_address);

	return server_sock;
}


int accept_tcp_connection(int server_sock)
{
	struct sockaddr_storage client_address; // Client address
	// Set length of client address structure (in-out parameter)
	socklen_t client_address_len = sizeof(client_address);

	// Wait for a client to connect
	int client_sock = accept(server_sock, (struct sockaddr *)&client_address, &client_address_len);
	if (client_sock < 0)
	{
		log(ERROR, "accept() failed");
		return -1;
	}

	// client_sock is connected to a client!
	log(INFO, "Handling client %s", address_buffer);

	return client_sock;
}