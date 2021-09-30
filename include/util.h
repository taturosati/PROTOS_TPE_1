#ifndef UTIL_H_
#define UTIL_H_

#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>

#define DATE_ES 0
#define DATE_EN 1

#define YEAR_OFFSET 1900
#define MONTH_OFFSET 1

int get_date(int date_format, char date[12]);
int get_time(char time_str[10]);
void to_lower_str(char *in_str);


#endif
