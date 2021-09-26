#include <server.h>

typedef struct t_buffer
{
	char *buffer;
	size_t len;	 // longitud del buffer
	size_t from; // desde donde falta escribir
} t_buffer;

typedef struct t_client
{
	int socket;
	ptr_parser parsers[TCP_COMMANDS];
	ptr_parser end_of_line_parser;
	unsigned action;
	unsigned may_match_count;
	int matched_command;
	int end_idx;
	int may_match[3];
	int read_counter;
} t_client;

static void reset_socket(t_client *client);
static void parseSocketRead(t_client *current, char *in_buffer, t_buffer *write_buffer, int valread, fd_set *writefds);
static void handleUdpDg(int udpSock, ptr_parser udp_parsers[TCP_COMMANDS]);
static void write_to_socket(int socket, fd_set *writefds, t_buffer *write_buffer, char *in_buffer, unsigned read_chars, unsigned copy_start);
static void handleEcho(t_client *current, fd_set *writefds, t_buffer_ptr write_buffer, char *in_buffer, int parse_end_idx, int curr_char);
static void handleGetTime(t_client *client, fd_set *writefds, t_buffer_ptr write_buffer);
static void handleGetDate(t_client *client, fd_set *writefds, t_buffer_ptr write_buffer);
void tolowerStr(char *in_str);
int date_fmt = DATE_ES;

unsigned total_lines = 0, invalid_lines = 0, total_connections = 0, invalid_datagrams = 0;

int main(int argc, char *argv[])
{
	int master_socket; // IPv4 e IPv6 (si estan habilitados)
	int new_socket, max_clients = MAX_SOCKETS, activity, curr_client, sd;

	struct t_client client_socket[MAX_SOCKETS];

	long valread;
	int max_sd;

	int port_used = PORT;

	if (argc > 1)
	{
		int received_port = atoi(argv[1]);
		log(DEBUG, "%s\n", argv[1]);

		if (received_port > 0 && received_port > MIN_PORT)
		{
			port_used = received_port;
		}
	}

	/*
	socklen_t addrlen = sizeof(address);
	struct sockaddr_storage clntAddr; // Client address
	socklen_t clntAddrLen = sizeof(clntAddr);
	*/

	char in_buffer[BUFFSIZE + 1]; //data buffer of 1K

	fd_set readfds; //set of socket descriptors
	fd_set writefds;

	t_buffer bufferWrite[MAX_SOCKETS]; //buffer de escritura asociado a cada socket, para no bloquear por escritura
	memset(bufferWrite, 0, sizeof bufferWrite);

	memset(client_socket, 0, sizeof(client_socket));

	master_socket = setupTCPServerSocket(port_used);

	int udpSock = udpSocket(PORT);
	if (udpSock < 0)
	{
		log(FATAL, "UDP socket failed");
	}
	else
	{
		log(DEBUG, "Waiting for UDP IPv4 on socket %d\n", udpSock);
	}

	struct parser_definition parser_defs[TCP_COMMANDS];
	struct parser_definition parser_defs_udp[TCP_COMMANDS];

	init_parser_defs(parser_defs, "ECHO ", "GET TIME", "GET DATE");
	init_parser_defs(parser_defs, "SET locale en", "SET locale es", "STATS");

	struct parser_definition end_of_line_parser_def = parser_utils_strcmpi("\r\n");

	ptr_parser udp_parsers[TCP_COMMANDS];
	init_parsers(udp_parsers, parser_defs_udp);
	// Limpiamos el conjunto de escritura
	FD_ZERO(&writefds);
	while (TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add masters sockets to set
		FD_SET(master_socket, &readfds);
		FD_SET(udpSock, &readfds);

		max_sd = udpSock;

		// add child sockets to set
		for (curr_client = 0; curr_client < max_clients; curr_client++)
		{
			// socket descriptor
			sd = client_socket[curr_client].socket;

			// if valid socket descriptor then add to read list
			if (sd > 0)
			{
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
		if (FD_ISSET(udpSock, &readfds))
		{
			log(DEBUG, "algo en udp kkkkk");
			handleUdpDg(udpSock, udp_parsers);
			//handleAddrInfo(udpSock);
		}

		//If something happened on the TCP master socket , then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = acceptTCPConnection(master_socket)) < 0)
			{
				log(ERROR, "Accept error on master socket %d", master_socket);
				continue;
			}
			for (curr_client = 0; curr_client < max_clients; curr_client++)
			{
				if (client_socket[curr_client].socket == 0) //empty
				{
					total_connections++;
					reset_socket(&client_socket[curr_client]);
					client_socket[curr_client].socket = new_socket;
					init_parsers(client_socket[curr_client].parsers, parser_defs);
					client_socket[curr_client].end_of_line_parser = parser_init(parser_no_classes(), &end_of_line_parser_def);
					log(DEBUG, "Adding to list of sockets as %d\n", curr_client);
					break;
				}
			}
		}

		for (curr_client = 0; curr_client < max_clients; curr_client++)
		{
			sd = client_socket[curr_client].socket;
			if (FD_ISSET(sd, &writefds))
			{
				handleWrite(sd, bufferWrite + curr_client, &writefds);
			}
		}

		//else its some IO operation on some other socket :)
		for (curr_client = 0; curr_client < max_clients; curr_client++)
		{
			sd = client_socket[curr_client].socket;
			if (FD_ISSET(sd, &readfds))
			{
				if ((valread = read(sd, in_buffer, BUFFSIZE)) <= 0)
				{
					close(sd); //Somebody disconnected or read failed
					client_socket[curr_client].socket = 0;

					FD_CLR(sd, &writefds);
					clear(bufferWrite + curr_client);
				}
				else
				{
					parseSocketRead(&client_socket[curr_client], in_buffer, &bufferWrite[curr_client], valread, &writefds);
				}
			}
		}
	}
	return 0;
}

static void parseSocketRead(t_client *current, char *in_buffer, t_buffer *write_buffer, int valread, fd_set *writefds)
{
	int curr_char, parse_end_idx = 0;
	for (curr_char = 0; curr_char < valread; curr_char++)
	{
		if (current->read_counter == 100 && current->end_idx == -1)
		{
			current->end_idx = curr_char;
		}
		current->read_counter++;

		const struct parser_event *state = parser_feed(current->end_of_line_parser, in_buffer[curr_char]);
		if (state->type != STRING_CMP_NEQ)
		{
			if (state->type == STRING_CMP_EQ)
			{ //EOF
				total_lines++;
				if (current->action == EXECUTING)
				{
					switch (current->matched_command)
					{
					case ECHO_C:
						log(DEBUG, "ECHO");
						handleEcho(current, writefds, write_buffer, in_buffer, parse_end_idx, curr_char);
						break;
					case DATE:
						log(DEBUG, "DATE");
						handleGetDate(current, writefds, write_buffer);
						break;
					case TIME:
						log(DEBUG, "TIME");
						handleGetTime(current, writefds, write_buffer);
						break;
					default:
						log(DEBUG, "NO MATCH");
						break;
					}
				}
				parser_reset(current->end_of_line_parser);
				reset_parsers(current->parsers, current->may_match);
				reset_socket(current);
			}
		}
		else
		{
			parser_reset(current->end_of_line_parser);
			if (current->action == PARSING)
			{
				log(DEBUG, "PARSING");

				for (int k = 0; k < TCP_COMMANDS && current->matched_command == -1 && current->may_match_count > 0; k++)
				{
					if (current->may_match[k])
					{
						const struct parser_event *state = parser_feed(current->parsers[k], in_buffer[curr_char]);
						if (state->type == STRING_CMP_EQ)
						{ //matcheo uno de los comandos (echo, date o time)
							log(DEBUG, "matched after %d bytes", curr_char);
							current->matched_command = k;
							parse_end_idx = curr_char + 1;
							current->action = EXECUTING;
						}
						else if (state->type == STRING_CMP_NEQ)
						{ //ya hay un comando q no matcheo
							current->may_match[k] = 0;
							current->may_match_count--;
						}
					}
				}
				// comando invalido, consumir hasta \r\n
				if (current->may_match_count == 0)
				{
					log(DEBUG, "Estoy en comando invalido");
					invalid_lines++;
					current->action = INVALID;
				}
			}
			else
			{
				if (current->matched_command != ECHO_C)
				{
					invalid_lines++;
					current->action = INVALID;
				}
				if (!US_ASCII(in_buffer[curr_char]) && current->end_idx == -1)
				{
					current->end_idx = curr_char;
				}
			}
		}
	}

	if (current->action == EXECUTING)
	{
		int end_idx = current->end_idx;
		if (current->end_idx == -1)
		{
			end_idx = curr_char;
		}
		else
		{ // hay un no usascii en este mensaje o me pase de los 100
			current->action = IDLE;
		}

		write_to_socket(current->socket, writefds, write_buffer, in_buffer, end_idx - parse_end_idx, parse_end_idx);
	}
}

void clear(t_buffer_ptr buffer)
{
	free(buffer->buffer);
	buffer->buffer = NULL;
	buffer->from = buffer->len = 0;
}

// Hay algo para escribir?
// Si está listo para escribir, escribimos. El problema es que a pesar de tener buffer para poder
// escribir, tal vez no sea suficiente. Por ejemplo podría tener 100 bytes libres en el buffer de
// salida, pero le pido que mande 1000 bytes.Por lo que tenemos que hacer un send no bloqueante,
// verificando la cantidad de bytes que pudo consumir TCP.
void handleWrite(int socket, t_buffer_ptr in_buffer, fd_set *writefds)
{
	size_t bytesToSend = in_buffer->len - in_buffer->from;
	if (bytesToSend > 0)
	{ // Puede estar listo para enviar, pero no tenemos nada para enviar
		log(INFO, "Trying to send %zu bytes to socket %d\n", bytesToSend, socket);
		size_t bytesSent = send(socket, in_buffer->buffer + in_buffer->from, bytesToSend, MSG_DONTWAIT);
		log(INFO, "Sent %zu bytes\n", bytesSent);

		if (bytesSent < 0)
		{
			// Esto no deberia pasar ya que el socket estaba listo para escritura
			// TODO: manejar el error
			log(FATAL, "Error sending to socket %d", socket);
		}
		else
		{
			size_t bytesLeft = bytesSent - bytesToSend;

			if (bytesLeft == 0)
			{
				clear(in_buffer);
				FD_CLR(socket, writefds); //ya no me interesa escribir porque ya mandé todo
			}
			else
			{
				in_buffer->from += bytesSent;
			}
		}
	}
}

int udpSocket(int port)
{
	int sock;
	struct sockaddr_in serverAddr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		log(ERROR, "UDP socket creation failed, errno: %d %s", errno, strerror(errno));
		return sock;
	}
	log(DEBUG, "UDP socket %d created", sock);
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET; // IPv4cle
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(sock, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		log(ERROR, "UDP bind failed, errno: %d %s", errno, strerror(errno));
		close(sock);
		return -1;
	}
	log(DEBUG, "UDP socket bind OK ");

	return sock;
}

void init_parser_defs(struct parser_definition defs[TCP_COMMANDS], char *first, char *second, char *third)
{
	int i = 0;
	defs[i++] = parser_utils_strcmpi(first);
	defs[i++] = parser_utils_strcmpi(second);
	defs[i] = parser_utils_strcmpi(third);
}

void init_parsers(ptr_parser parsers[TCP_COMMANDS], struct parser_definition defs[TCP_COMMANDS])
{
	for (int i = 0; i < TCP_COMMANDS; i++)
	{
		parsers[i] = parser_init(parser_no_classes(), &defs[i]);
	}
}

static void reset_socket(struct t_client *client)
{
	client->action = PARSING;
	client->end_idx = -1;
	client->may_match_count = TCP_COMMANDS;
	client->matched_command = -1;
	client->read_counter = 0;
	for (int i = 0; i < TCP_COMMANDS; i++)
	{
		client->may_match[i] = 1;
	}
}

static void write_to_socket(int socket, fd_set *writefds, t_buffer *write_buffer, char *in_buffer, unsigned read_chars, unsigned copy_start)
{
	FD_SET(socket, writefds);
	write_buffer->buffer = realloc(write_buffer->buffer, write_buffer->len + read_chars);
	memcpy(write_buffer->buffer + write_buffer->len, in_buffer + copy_start, read_chars);
	write_buffer->len += read_chars;
}

void reset_parsers(ptr_parser parsers[TCP_COMMANDS], int *may_match)
{
	for (int i = 0; i < TCP_COMMANDS; i++)
	{
		parser_reset(parsers[i]);
		may_match[i] = 1;
	}
}

static void handleEcho(t_client *current, fd_set *writefds, t_buffer_ptr write_buffer, char *in_buffer, int parse_end_idx, int cur_char)
{
	int end_idx = current->end_idx;
	if (current->end_idx == -1)
	{
		end_idx = cur_char + 1;
	}

	write_to_socket(current->socket, writefds, write_buffer, in_buffer, end_idx - parse_end_idx, parse_end_idx);
	if (current->end_idx != -1 || current->action == IDLE)
	{
		write_to_socket(current->socket, writefds, write_buffer, "\r\n", 2, 0);
	}
}

static void handleGetTime(t_client *client, fd_set *writefds, t_buffer_ptr write_buffer)
{
	char curr_time[10] = {0};
	getTime(curr_time);
	write_to_socket(client->socket, writefds, write_buffer, curr_time, 10, 0);
}

static void handleGetDate(t_client *client, fd_set *writefds, t_buffer_ptr write_buffer)
{
	char curr_date[12] = {0};
	getDate(date_fmt, curr_date);
	write_to_socket(client->socket, writefds, write_buffer, curr_date, 12, 0);
}

void tolowerStr(char *in_str)
{
	char *out_str = in_str;
	while (*out_str)
	{
		*in_str = tolower((unsigned char)*out_str);
		out_str++;
	}
}

static void handleUdpDg(int udpSock, ptr_parser udp_parsers[TCP_COMMANDS])
{
	char buffer[BUFFSIZE];
	unsigned int len, read_chars;

	struct sockaddr_in clntAddr;

	read_chars = recvfrom(udpSock, buffer, BUFFSIZE, 0, (struct sockaddr *)&clntAddr, &len);

	if (buffer[read_chars - 1] == '\n') // Por si lo estan probando con netcat, en modo interactivo
		read_chars--;
	buffer[read_chars] = '\0';
	log(DEBUG, "UDP received:%s", buffer);

	char set_str[3], locale_str[6], language_str[2];
	to_lower_str(buffer);

	if (sscanf(buffer, "%s %s %s", set_str, locale_str, language_str) == 3)
	{
		if (strcmp(set_str, "set") == 0 && strcmp(locale_str, "locale") == 0)
		{
			if (strcmp(language_str, "en") == 0)
			{
				date_fmt = DATE_EN;
			}
			else if (strcmp(language_str, "es") == 0)
			{
				date_fmt = DATE_ES;
			}
		}
	}
	else if (strcmp(buffer, "stats") == 0)
	{
		char bufferOut[BUFFSIZE];

		sprintf(bufferOut, "Connections: %d\r\nIncorrect lines: %d\r\nCorrect lines: %d\r\nInvalida datagrams: %d\r\n",
		total_connections, invalid_lines, total_lines - invalid_lines, invalid_datagrams);

		sendto(udpSock, bufferOut, strlen(bufferOut), 0, (const struct sockaddr *)&clntAddr, len);

		log(DEBUG, "UDP sent:%s", bufferOut);
	}
	else
	{
		invalid_datagrams++;
	}
}

