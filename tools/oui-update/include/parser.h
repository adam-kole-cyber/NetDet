#ifndef PARSER_H
#define PARSER_H

int mac_prefix_parser(char *buffer, int **mac_prefix, int mac_prefix_index, int *mac_prefix_capacity);
int vendor_name_parser(char *buffer, char *vendor_name, int vendor_name_count, int vendor_name_capacity);

#endif
