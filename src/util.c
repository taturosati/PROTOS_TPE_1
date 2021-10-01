#include <stdio.h>

#include <string.h>

#include <time.h>

#include <stdlib.h>

#include <logger.h>

#include <util.h>

#include <ctype.h>

static void get_time_params(struct tm tm, int params[FORMAT_ARGUMENTS]);

static void get_date_params_en(struct tm tm, int params[FORMAT_ARGUMENTS]);

static void get_date_params_es(struct tm tm, int params[FORMAT_ARGUMENTS]);

static int get_date_time(char time_str[FORMAT_SIZE], char* fmt, void(*get_params)(struct tm, int[FORMAT_ARGUMENTS]));

int get_date(int date_format, char date[FORMAT_SIZE]) {
	if (date_format == DATE_EN)
		return get_date_time(date, "%02d/%02d/%d\n", &get_date_params_en);
	else
		return get_date_time(date, "%02d/%02d/%d\n", &get_date_params_es);
}

int get_time(char time_str[FORMAT_SIZE]) {
	return get_date_time(time_str, "%02d:%02d:%02d\n", &get_time_params);
}

void to_lower_str(char* in_str) {
	for (int i = 0; in_str[i]; i++) {
		in_str[i] = tolower(in_str[i]);
	}
}

static int get_date_time(char time_str[FORMAT_SIZE], char* fmt, void(*get_params)(struct tm, int[FORMAT_ARGUMENTS])) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int chars_read;
	int params[3];
	get_params(tm, params);
	if ((chars_read = sprintf(time_str, fmt, params[0], params[1], params[2])) < 0)
		return -1;
	else
		return chars_read;
}

static void get_time_params(struct tm tm, int params[FORMAT_ARGUMENTS]) {
	params[0] = tm.tm_hour;
	params[1] = tm.tm_min;
	params[2] = tm.tm_sec;
}

static void get_date_params_en(struct tm tm, int params[FORMAT_ARGUMENTS]) {
	params[0] = tm.tm_mon + 1;
	params[1] = tm.tm_mday;
	params[2] = tm.tm_year + YEAR_OFFSET;
}

static void get_date_params_es(struct tm tm, int params[FORMAT_ARGUMENTS]) {
	params[0] = tm.tm_mday;
	params[1] = tm.tm_mon + 1;
	params[2] = tm.tm_year + YEAR_OFFSET;
}