#include <command_handler.h>

#include <errno.h>

void handle_echo(t_client_ptr current, fd_set* writefds, t_buffer_ptr write_buffer, char* in_buffer, int parse_end_idx, int cur_char) {
	int end_idx = current->end_idx;
	if (current->end_idx == -1) {
		end_idx = cur_char + 1;
	}

	write_to_socket(current->socket, writefds, write_buffer, in_buffer, end_idx - parse_end_idx, parse_end_idx);
	if (current->end_idx != -1 || current->action == IDLE) {
		write_to_socket(current->socket, writefds, write_buffer, "\r\n", 2, 0);
	}
}

void handle_time(t_client_ptr client, fd_set* writefds, t_buffer_ptr write_buffer, char* in_buffer, int parse_end_idx, int curr_char) {
	char curr_time[FORMAT_SIZE] = {
	  0
	};
	get_time(curr_time);
	write_to_socket(client->socket, writefds, write_buffer, curr_time, FORMAT_SIZE, 0);
}

void handle_date(t_client_ptr client, fd_set* writefds, t_buffer_ptr write_buffer, char* in_buffer, int parse_end_idx, int curr_char) {
	char curr_date[FORMAT_SIZE] = {
	  0
	};
	get_date(date_fmt, curr_date);
	write_to_socket(client->socket, writefds, write_buffer, curr_date, FORMAT_SIZE, 0);
}

void handle_udp_datagram(int udp_sock) {
	char buffer[BUFFSIZE];
	struct sockaddr_in6 client_address;
	unsigned int read_chars, len = sizeof(client_address);

	read_chars = recvfrom(udp_sock, buffer, BUFFSIZE, 0, (struct sockaddr*)&client_address, &len);

	log(DEBUG, "Read %d chars", read_chars);

	if (buffer[read_chars - 1] == '\n') // Por si lo estan probando con netcat, en modo interactivo
		read_chars--;
	buffer[read_chars] = '\0';
	log(DEBUG, "UDP received:%s", buffer);

	char* set_str, * locale_str, * language_str;
	to_lower_str(buffer);

	if (strcmp(buffer, "stats") == 0) {
		char buffer_out[BUFFSIZE] = {
		  0
		};

		sprintf(buffer_out, "Connections: %d\r\nIncorrect lines: %d\r\nCorrect lines: %d\r\nInvalid datagrams: %d\r\n",
			total_connections, invalid_lines, total_lines - invalid_lines, invalid_datagrams);

		errno = 0;
		if (sendto(udp_sock, buffer_out, strlen(buffer_out), 0, (const struct sockaddr*)&client_address, len) < 0) {
			log(DEBUG, "Error sending response");
		} else {
			log(DEBUG, "UDP sent:%s", buffer_out);
		}
	}
	else if (sscanf(buffer, "%ms %ms %ms", &set_str, &locale_str, &language_str) == 3) {
		if (strcmp(set_str, "set") == 0 && strcmp(locale_str, "locale") == 0) {

			if (strcmp(language_str, "en") == 0) {
				log(DEBUG, "Setting date format EN");
				date_fmt = DATE_EN;
			}
			else if (strcmp(language_str, "es") == 0) {
				log(DEBUG, "Setting date format ES");
				date_fmt = DATE_ES;
			}
		}
		free(set_str);
		free(locale_str);
		free(language_str);
	}
	else {
		invalid_datagrams++;
	}
}

void parse_socket_read(t_client_ptr current, char* in_buffer, t_buffer_ptr write_buffer, int valread, fd_set* writefds) {
	int curr_char, parse_end_idx = 0;
	for (curr_char = 0; curr_char < valread; curr_char++) {
		if (current->read_counter == 100 && current->end_idx == -1) {
			current->end_idx = curr_char;
		}
		current->read_counter++;

		const struct parser_event* state = parser_feed(current->end_of_line_parser, in_buffer[curr_char]);
		if (state->type != STRING_CMP_NEQ) {
			if (state->type == STRING_CMP_EQ) { //EOF
				total_lines++;
				if (current->action == EXECUTING)
					tcp_actions[current->matched_command](current, writefds, write_buffer, in_buffer, parse_end_idx, curr_char);
				else if (current->action != IDLE) {
					write_to_socket(current->socket, writefds, write_buffer, "Invalid command\r\n", 18, 0);
					if (current->action == PARSING)
						invalid_lines++;
				}

				parser_reset(current->end_of_line_parser);
				reset_parsers(current->parsers, current->may_match, TCP_COMMANDS);
				reset_socket(current);
			}
		}
		else {
			parser_reset(current->end_of_line_parser);
			if (current->action == PARSING) {

				for (int k = 0; k < TCP_COMMANDS && current->matched_command == -1 && current->may_match_count > 0; k++) {
					if (current->may_match[k]) {
						const struct parser_event* state = parser_feed(current->parsers[k], in_buffer[curr_char]);
						if (state->type == STRING_CMP_EQ) { //matcheo uno de los comandos (echo, date o time)
							log(DEBUG, "matched after %d bytes", curr_char);
							current->matched_command = k;
							parse_end_idx = curr_char + 1;
							current->action = EXECUTING;
						}
						else if (state->type == STRING_CMP_NEQ) { //ya hay un comando q no matcheo
							current->may_match[k] = 0;
							current->may_match_count--;
						}
					}
				}
				// comando invalido, consumir hasta \r\n
				if (current->may_match_count == 0) {
					invalid_lines++;
					current->action = INVALID;
				}
			}
			else {
				if (current->matched_command != ECHO_C && current->action != INVALID && current->action != IDLE) {
					invalid_lines++;
					current->action = INVALID;
				}
				if (!US_ASCII(in_buffer[curr_char]) && current->end_idx == -1) {
					current->end_idx = curr_char;
				}
			}
		}
	}

	if (current->action == EXECUTING) {
		int end_idx = current->end_idx;
		if (current->end_idx == -1) {
			end_idx = curr_char;
		}
		else { // hay un no usascii en este mensaje o me pase de los 100
			current->action = IDLE;
		}

		write_to_socket(current->socket, writefds, write_buffer, in_buffer, end_idx - parse_end_idx, parse_end_idx);
	}
}

void reset_socket(t_client_ptr client) {
	client->action = PARSING;
	client->end_idx = -1;
	client->may_match_count = TCP_COMMANDS;
	client->matched_command = -1;
	client->read_counter = 0;
	for (int i = 0; i < TCP_COMMANDS; i++) {
		client->may_match[i] = 1;
	}
}

void write_to_socket(int socket, fd_set* writefds, t_buffer_ptr write_buffer, char* in_buffer, unsigned read_chars, unsigned copy_start) {
	FD_SET(socket, writefds);

	size_t read_c = (size_t)read_chars;

	uint8_t* buff = buffer_write_ptr(write_buffer, &read_c);

	log(DEBUG, "Trying to send %d bytes", read_chars);
	memcpy(buff, in_buffer + copy_start, read_chars);
	buffer_write_adv(write_buffer, read_chars);
}