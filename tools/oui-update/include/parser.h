#ifndef PARSER_H
#define PARSER_H

typedef struct {
	int *data;
	int count;
	int capacity;
} mac_prefix;

typedef struct {
	char *data;
	int count;
	int capacity;
} vendor_name;

int mac_prefix_parser(char *buffer, mac_prefix *mac);
int vendor_name_parser(char *buffer, char *vendor_name, int vendor_name_count, int vendor_name_capacity);

#endif
