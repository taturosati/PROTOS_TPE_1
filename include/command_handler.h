#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <sys/select.h>
#include <defs.h>
#include <logger.h>
#include <util.h>
#include <string.h>
#include <parser.h>
#include <parser_utils.h>

extern int date_fmt;
extern unsigned int total_lines, invalid_lines, total_connections, invalid_datagrams;
extern void (*tcp_actions[TCP_COMMANDS])(t_client_ptr current, fd_set *writefds, t_buffer_ptr write_buffer,
								  char *in_buffer, int parse_end_idx, int cur_char);

void handle_echo(t_client_ptr client, fd_set *writefds, t_buffer_ptr write_buffer, char *in_buffer, int parse_end_idx, int cur_char);
void handle_time(t_client_ptr client, fd_set *writefds, t_buffer_ptr write_buffer, char *in_buffer, int parse_end_idx, int curr_char);
void handle_date(t_client_ptr client, fd_set *writefds, t_buffer_ptr write_buffer, char *in_buffer, int parse_end_idx, int curr_char);
void handle_udp_datagram(int udp_sock);

void parse_socket_read(t_client_ptr current, char* in_buffer, t_buffer_ptr write_buffer, int valread, fd_set* writefds);
void reset_socket(t_client_ptr client);

void write_to_socket(int socket, fd_set* writefds, t_buffer_ptr write_buffer, char* in_buffer, unsigned read_chars, unsigned copy_start);

#endif