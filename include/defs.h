#ifndef __defs_h_
#define __defs_h_

#include <netinet/in.h>

#define BUFFSIZE 1024
#define TCP_COMMANDS 3
#define US_ASCII(x) ((x < 0 || x > 127) ? (0) : (1))

typedef struct parser* ptr_parser;

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

typedef struct t_client * t_client_ptr;

typedef enum { PARSING = 0, EXECUTING, INVALID, IDLE } action;
typedef enum { ECHO_C = 0, TIME, DATE } command;

#endif