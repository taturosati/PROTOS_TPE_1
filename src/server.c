#include <server.h>

int date_fmt = DATE_ES;
unsigned int total_lines = 0, invalid_lines = 0, total_connections = 0, invalid_datagrams = 0;

static void handle_write(int socket, t_buffer_ptr in_buffer, fd_set* writefds);
static void run_server(int master_socket, int udp_sock, struct parser_definition* tcp_parsers,
	struct parser_definition* end_of_line_parser_def,
	t_client_ptr client_socket, t_buffer_ptr buffer_write);

void(*tcp_actions[TCP_COMMANDS])(t_client_ptr current, fd_set* writefds, t_buffer_ptr write_buffer,
	char* in_buffer, int parse_end_idx, int cur_char) = {handle_echo, handle_time, handle_date};

int main(int argc, char* argv[]) {
	int master_socket;
	t_client client_socket[MAX_SOCKETS];
	int port_used = PORT;
	if (argc > 1) {
		int received_port = atoi(argv[1]);
		log(DEBUG, "%s\n", argv[1]);
		if (received_port > 0 && received_port > MIN_PORT) {
			port_used = received_port;
		}
	}

	t_buffer buffer_write[MAX_SOCKETS];
	memset(buffer_write, 0, sizeof buffer_write);
	memset(client_socket, 0, sizeof(client_socket));
	master_socket = setup_server_socket(port_used, IPPROTO_TCP);
	int udp_sock = setup_server_socket(port_used, IPPROTO_UDP);
	if (udp_sock < 0) {
		log(FATAL, "UDP socket failed");
	}
	else {
		log(DEBUG, "Waiting for UDP IPv4 on socket %d\n", udp_sock);
	}
	struct parser_definition tcp_parsers[TCP_COMMANDS];
	char* tcp_strings[] = { "ECHO ", "GET TIME", "GET DATE" };
	init_parser_defs(tcp_parsers, tcp_strings, TCP_COMMANDS);
	struct parser_definition end_of_line_parser_def = parser_utils_strcmpi("\r\n");
	run_server(master_socket, udp_sock, tcp_parsers, &end_of_line_parser_def, client_socket, buffer_write);
	return 0;
}


static int fillSet(fd_set * readfds, int master_socket, int udp_sock, t_client_ptr client_socket){
	FD_ZERO(readfds);
	FD_SET(master_socket, readfds);
	FD_SET(udp_sock, readfds);

	int sd, max_sd = udp_sock;
	for (int curr_client = 0; curr_client < MAX_SOCKETS; curr_client++) {
		sd = client_socket[curr_client].socket;
		if (sd > 0) {
			FD_SET(sd, readfds);
			max_sd = max(sd, max_sd);
		}
	}
	return max_sd;
}

static void run_server(int master_socket, int udp_sock, struct parser_definition* tcp_parsers,
	struct parser_definition* end_of_line_parser_def,
	t_client_ptr client_socket, t_buffer_ptr buffer_write) {
	fd_set readfds;
	fd_set writefds;

	FD_ZERO(&writefds);

	int new_socket, max_clients = MAX_SOCKETS, activity, curr_client, sd, max_sd;
	long valread;

	char in_buffer[BUFFSIZE + 1];

	while (TRUE) {
		max_sd = fillSet(&readfds, master_socket, udp_sock, client_socket);
		activity = select(max_sd + 1, &readfds, &writefds, NULL, NULL);
		log(DEBUG, "select has something...");
		
		if ((activity < 0) && (errno != EINTR)) {
			log(ERROR, "select error, errno=%d", errno);
			continue;
		}
		if (FD_ISSET(udp_sock, &readfds)) {
			handle_udp_datagram(udp_sock);
		}
		if (FD_ISSET(master_socket, &readfds)) {
			if ((new_socket = accept_tcp_connection(master_socket)) < 0) {
				log(ERROR, "Accept error on master socket %d", master_socket);
				continue;
			}
			for (curr_client = 0; curr_client < max_clients; curr_client++) {
				if (client_socket[curr_client].socket == 0) {
					uint8_t* buff_data = malloc(BUFFER_SIZE);
					if(buff_data == NULL){
						log(DEBUG, "No more space for connections");
						break;
					} 
					total_connections++;
					buffer_init(&buffer_write[curr_client], BUFFER_SIZE, buff_data);
					reset_socket(&client_socket[curr_client]);
					client_socket[curr_client].socket = new_socket;
					init_parsers(client_socket[curr_client].parsers, tcp_parsers, TCP_COMMANDS);
					client_socket[curr_client].end_of_line_parser = parser_init(parser_no_classes(), end_of_line_parser_def);
					log(DEBUG, "Adding to list of sockets as %d\n", curr_client);
					break;
				}
			}
		}

		for (curr_client = 0; curr_client < max_clients; curr_client++) {
			sd = client_socket[curr_client].socket;
			if (FD_ISSET(sd, &writefds)) {
				handle_write(sd, buffer_write + curr_client, &writefds);
			}
		}

		for (curr_client = 0; curr_client < max_clients; curr_client++) {
			sd = client_socket[curr_client].socket;
			if (FD_ISSET(sd, &readfds)) {
				if ((valread = read(sd, in_buffer, BUFFSIZE)) <= 0) {
					close(sd);
					client_socket[curr_client].socket = 0;
					for (int curr_parser = 0; curr_parser < TCP_COMMANDS; curr_parser++)
						parser_destroy(client_socket[curr_client].parsers[curr_parser]);

					parser_destroy(client_socket[curr_client].end_of_line_parser);
					free(buffer_write[curr_client].data);
					FD_CLR(sd, &writefds);
				}
				else {
					parse_socket_read(&client_socket[curr_client], in_buffer, &buffer_write[curr_client], valread, &writefds);
				}
			}
		}
	}
}

static void handle_write(int socket, t_buffer_ptr in_buffer, fd_set* writefds) {
	size_t bytes_to_send = buffer_pending_read(in_buffer);
	if (bytes_to_send > 0) {
		size_t bytes_aux = bytes_to_send;
		log(INFO, "Trying to send %zu bytes to socket %d\n", bytes_to_send, socket);
		long bytes_sent = send(socket, buffer_read_ptr(in_buffer, &bytes_aux), bytes_to_send, MSG_DONTWAIT);
		buffer_read_adv(in_buffer, bytes_sent);
		log(INFO, "Sent %zu bytes\n", bytes_sent);

		if (bytes_sent < 0) {
			log(FATAL, "Error sending to socket %d", socket);
		}
		else {
			size_t bytesLeft = bytes_sent - bytes_to_send;
			if (bytesLeft == 0) {
				FD_CLR(socket, writefds);
			}
		}
	}
}
