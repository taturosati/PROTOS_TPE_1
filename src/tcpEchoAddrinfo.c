#include <tcpEchoAddrinfo.h>

typedef struct t_buffer {
	char* buffer;
	size_t len;     // longitud del buffer
	size_t from;    // desde donde falta escribir
} t_buffer;

typedef struct t_client {
	int socket;
	ptr_parser parsers[TCP_COMMANDS];
	ptr_parser end_of_line_parser;
	unsigned action;
	unsigned may_match_count;
	unsigned matched_command;
	int end_idx;
} t_client;

int main(int argc, char* argv[]) {
	int opt = TRUE;
	int master_socket;  // IPv4 e IPv6 (si estan habilitados)
	int new_socket, max_clients = MAX_SOCKETS, activity, i, sd;

	struct t_client client_socket[MAX_SOCKETS];

	long valread;
	int max_sd;
	struct sockaddr_in address = { 0 };

	int port_used = PORT;

	if (argc > 1) {
		int received_port = atoi(argv[1]);
		log(DEBUG, "%s\n", argv[1]);

		if (received_port > 0 && received_port > MIN_PORT) {
			port_used = received_port;
		}
	}

	/*
	socklen_t addrlen = sizeof(address);
	struct sockaddr_storage clntAddr; // Client address
	socklen_t clntAddrLen = sizeof(clntAddr);
	*/

	char in_buffer[BUFFSIZE + 1];  //data buffer of 1K

	fd_set readfds; //set of socket descriptors

	t_buffer bufferWrite[MAX_SOCKETS]; //buffer de escritura asociado a cada socket, para no bloquear por escritura
	memset(bufferWrite, 0, sizeof bufferWrite);

	fd_set writefds;

	memset(client_socket, 0, sizeof(client_socket));

	master_socket = setupTCPServerSocket(port_used);

	int udpSock = udpSocket(PORT);
	if (udpSock < 0) {
		log(FATAL, "UDP socket failed");
	}
	else {
		log(DEBUG, "Waiting for UDP IPv4 on socket %d\n", udpSock);
	}

	//ESTO ESTA MAL ? --> no deberia ser parser_defs[3] ? total es fijo para todos

	struct parser_definition parser_defs[TCP_COMMANDS];
	// for (int i = 0; i < 3; i++) {
	// 	parser_defs[i] = malloc(sizeof(struct parser_definition));
	// }
	init_parser_defs(parser_defs);

	struct parser_definition end_of_line_parser_def = parser_utils_strcmpi("\r\n");

	// Limpiamos el conjunto de escritura
	FD_ZERO(&writefds);
	while (TRUE) {
		//clear the socket set
		FD_ZERO(&readfds);

		//add masters sockets to set
		FD_SET(master_socket, &readfds);
		FD_SET(udpSock, &readfds);

		max_sd = udpSock;

		// add child sockets to set
		for (i = 0; i < max_clients; i++)
		{
			// socket descriptor
			sd = client_socket[i].socket;

			// if valid socket descriptor then add to read list
			if (sd > 0) {
				FD_SET(sd, &readfds);
				max_sd = max(sd, max_sd);
			}
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select(max_sd + 1, &readfds, &writefds, NULL, NULL);

		log(DEBUG, "select has something...");

		if ((activity < 0) && (errno != EINTR))
		{
			log(ERROR, "select error, errno=%d", errno);
			continue;
		}

		// Servicio UDP
		if (FD_ISSET(udpSock, &readfds)) {
			handleAddrInfo(udpSock);
		}

		//If something happened on the TCP master socket , then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = acceptTCPConnection(master_socket)) < 0)
			{
				log(ERROR, "Accept error on master socket %d", master_socket);
				continue;
			}

			for (i = 0; i < max_clients; i++) {
				if (client_socket[i].socket == 0) //empty
				{
					client_socket[i].action = PARSING;
					client_socket[i].end_idx = -1;
					client_socket[i].may_match_count = TCP_COMMANDS;
					client_socket[i].socket = new_socket;
					init_parsers(client_socket[i].parsers, parser_defs);
					client_socket[i].end_of_line_parser = parser_init(parser_no_classes(), &end_of_line_parser_def);
					log(DEBUG, "Adding to list of sockets as %d\n", i);
					break;
				}
			}
		}

		for (i = 0; i < max_clients; i++) {
			sd = client_socket[i].socket;
			if (FD_ISSET(sd, &writefds)) {
				handleWrite(sd, bufferWrite + i, &writefds);

			}
		}

		//else its some IO operation on some other socket :)
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i].socket;
			if (FD_ISSET(sd, &readfds))
			{
				if ((valread = read(sd, in_buffer, BUFFSIZE)) <= 0)
				{
					close(sd); //Somebody disconnected or read failed
					client_socket[i].socket = 0;

					FD_CLR(sd, &writefds);
					clear(bufferWrite + i);

				}
				else {
					int may_match[] = { 1, 1, 1 };
					client_socket[i].may_match_count = TCP_COMMANDS;
					int j, parse_end_idx = TCP_COMMANDS;
					for (j = 0; j < valread; j++) {
						if (client_socket[i].action == PARSING) {
							log(DEBUG, "PARSING", NULL);

							for (int k = 0; k < TCP_COMMANDS && client_socket[i].matched_command == -1 && client_socket[i].may_match_count > 0; k++) {
								if (may_match[k]) {
									const struct parser_event* state = parser_feed(client_socket[i].parsers[k], in_buffer[j]);
									if (state->type == STRING_CMP_EQ) {//matcheo uno de los comandos (echo, date o time)
										log(DEBUG, "matched after %d bytes", j);

										parse_end_idx = j + 1;
										client_socket[i].action = EXECUTING;
										client_socket[i].matched_command = k;
										client_socket[i].end_idx = -1;
										parser_reset(client_socket[i].parsers[k]);
									}
									else if (state->type == STRING_CMP_NEQ) {//ya hay un comando q no matcheo
										may_match[k] = 0;
										client_socket[i].may_match_count--;
									}
								}
							}
							// comando invalido, consumir hasta \r\n
							if (client_socket[i].may_match_count == 0) {
								log(DEBUG, "Estoy en comando invalido");
								client_socket[i].action = INVALID;
							}
						}
						else {
							if (!US_ASCII(in_buffer[j]) && client_socket[i].end_idx == -1) {
								client_socket[i].end_idx = j;
							}

							const struct parser_event* state = parser_feed(client_socket[i].end_of_line_parser, in_buffer[j]);
							if (state->type == STRING_CMP_NEQ) {
								parser_reset(client_socket[i].end_of_line_parser);
							} else if (state->type == STRING_CMP_EQ) { //EOF
								if (client_socket[i].action == EXECUTING) {
									FD_SET(sd, &writefds);

									if (client_socket[i].end_idx == -1) {
										client_socket[i].end_idx = j + 1;
									}

									bufferWrite[i].buffer = realloc(bufferWrite[i].buffer, bufferWrite[i].len + client_socket[i].end_idx - parse_end_idx);
									memcpy(bufferWrite[i].buffer + bufferWrite[i].len, in_buffer + parse_end_idx, client_socket[i].end_idx - parse_end_idx);
									bufferWrite[i].len += client_socket[i].end_idx - parse_end_idx;
								}


								client_socket[i].end_idx = -1;
								client_socket[i].action = PARSING;
								reset_parsers(client_socket[i].parsers, may_match);
								client_socket[i].matched_command = -1;
								client_socket[i].may_match_count = TCP_COMMANDS;
							}
						}
					}

					if (client_socket[i].action == EXECUTING) {
						FD_SET(sd, &writefds);
						bufferWrite[i].buffer = realloc(bufferWrite[i].buffer, bufferWrite[i].len + j + 1 - parse_end_idx);
						memcpy(bufferWrite[i].buffer + bufferWrite[i].len, in_buffer + parse_end_idx, j + 1 - parse_end_idx);
						bufferWrite[i].len += j + 1 - parse_end_idx;
					}
				}
			}
		}
	}
	return 0;
}

void clear(t_buffer_ptr buffer) {
	free(buffer->buffer);
	buffer->buffer = NULL;
	buffer->from = buffer->len = 0;
}

// Hay algo para escribir?
// Si está listo para escribir, escribimos. El problema es que a pesar de tener buffer para poder
// escribir, tal vez no sea suficiente. Por ejemplo podría tener 100 bytes libres en el buffer de
// salida, pero le pido que mande 1000 bytes.Por lo que tenemos que hacer un send no bloqueante,
// verificando la cantidad de bytes que pudo consumir TCP.
void handleWrite(int socket, t_buffer_ptr in_buffer, fd_set* writefds) {
	size_t bytesToSend = in_buffer->len - in_buffer->from;
	if (bytesToSend > 0) {// Puede estar listo para enviar, pero no tenemos nada para enviar
		log(INFO, "Trying to send %zu bytes to socket %d\n", bytesToSend, socket);
		size_t bytesSent = send(socket, in_buffer->buffer + in_buffer->from, bytesToSend, MSG_DONTWAIT);
		log(INFO, "Sent %zu bytes\n", bytesSent);

		if (bytesSent < 0) {
			// Esto no deberia pasar ya que el socket estaba listo para escritura
			// TODO: manejar el error
			log(FATAL, "Error sending to socket %d", socket);
		}
		else {
			size_t bytesLeft = bytesSent - bytesToSend;

			if (bytesLeft == 0) {
				clear(in_buffer);
				FD_CLR(socket, writefds); //ya no me interesa escribir porque ya mandé todo
			}
			else {
				in_buffer->from += bytesSent;
			}
		}
	}
}

int udpSocket(int port) {

	int sock;
	struct sockaddr_in serverAddr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log(ERROR, "UDP socket creation failed, errno: %d %s", errno, strerror(errno));
		return sock;
	}
	log(DEBUG, "UDP socket %d created", sock);
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET; // IPv4cle
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(sock, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		log(ERROR, "UDP bind failed, errno: %d %s", errno, strerror(errno));
		close(sock);
		return -1;
	}
	log(DEBUG, "UDP socket bind OK ");

	return sock;
}


void handleAddrInfo(int socket) {
	// En el datagrama viene el nombre a resolver
	// Se le devuelve la informacion asociada

	char in_buffer[BUFFSIZE];
	unsigned int len, n;

	struct sockaddr_in clntAddr;

	// Es bloqueante, deberian invocar a esta funcion solo si hay algo disponible en el socket    
	n = recvfrom(socket, in_buffer, BUFFSIZE, 0, (struct sockaddr*)&clntAddr, &len);
	if (in_buffer[n - 1] == '\n') // Por si lo estan probando con netcat, en modo interactivo
		n--;
	in_buffer[n] = '\0';
	log(DEBUG, "UDP received:%s", in_buffer);
	// TODO: parsear lo recibido para obtener nombre, puerto, etc. Asumimos viene solo el nombre

	// Especificamos solo SOCK_STREAM para que no duplique las respuestas
	struct addrinfo addrCriteria;                   // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;             // Any address family
	addrCriteria.ai_socktype = SOCK_STREAM;         // Only stream sockets
	addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol


	// Armamos el datagrama con las direcciones de respuesta, separadas por \r\n
	// TODO: hacer una concatenacion segura
	// TODO: modificar la funcion printAddressInfo usada en sockets bloqueantes para que sirva
	//       tanto si se quiere obtener solo la direccion o la direccion mas el puerto
	char bufferOut[BUFFSIZE];
	bufferOut[0] = '\0';

	struct addrinfo* addrList;
	int rtnVal = getaddrinfo(in_buffer, NULL, &addrCriteria, &addrList);
	if (rtnVal != 0) {
		log(ERROR, "getaddrinfo() failed: %d: %s", rtnVal, gai_strerror(rtnVal));
		strcat(strcpy(bufferOut, "Can't resolve "), in_buffer);
	}
	else {
		for (struct addrinfo* addr = addrList; addr != NULL; addr = addr->ai_next) {
			struct sockaddr* address = addr->ai_addr;
			char addrBuffer[INET6_ADDRSTRLEN];

			void* numericAddress = NULL;
			switch (address->sa_family) {
			case AF_INET:
				numericAddress = &((struct sockaddr_in*)address)->sin_addr;
				break;
			case AF_INET6:
				numericAddress = &((struct sockaddr_in6*)address)->sin6_addr;
				break;
			}
			if (numericAddress == NULL) {
				strcat(bufferOut, "[Unknown Type]");
			}
			else {
				// Convert binary to printable address
				if (inet_ntop(address->sa_family, numericAddress, addrBuffer, sizeof(addrBuffer)) == NULL)
					strcat(bufferOut, "[invalid address]");
				else {
					strcat(bufferOut, addrBuffer);
				}
			}
			strcat(bufferOut, "\r\n");
		}
		freeaddrinfo(addrList);
	}

	// Enviamos respuesta (el sendto no bloquea)
	sendto(socket, bufferOut, strlen(bufferOut), 0, (const struct sockaddr*)&clntAddr, len);

	log(DEBUG, "UDP sent:%s", bufferOut);
}

void init_parser_defs(struct parser_definition defs[TCP_COMMANDS]) {
	int i = 0;
	defs[i++] = parser_utils_strcmpi("ECHO ");
	defs[i++] = parser_utils_strcmpi("GET TIME");
	defs[i] = parser_utils_strcmpi("GET DATE");
}

void init_parsers(ptr_parser parsers[TCP_COMMANDS], struct parser_definition defs[TCP_COMMANDS]) {
	for (int i = 0; i < TCP_COMMANDS; i++) {
		parsers[i] = parser_init(parser_no_classes(), &defs[i]);
	}
}

void reset_parsers(ptr_parser parsers[TCP_COMMANDS], int* may_match) {
	for (int i = 0; i < TCP_COMMANDS; i++) {
		parser_reset(parsers[i]);
		may_match[i] = 1;
	}
}