#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int mac_prefix_parser(char *buffer, int **mac_prefix, int mac_prefix_index, int *mac_prefix_capacity) {
	int step_number = 0;
	int decrement = 0;
	int *tmp;

	while (buffer[step_number] != ',') {
		if (buffer[step_number] >= '0' && buffer[step_number] <= '9') {
			decrement = 48;
		} else if (buffer[step_number] >= 'A' && buffer[step_number] <= 'F') {
			decrement = 55;
		} else if (buffer[step_number] >= 'a' && buffer[step_number] <= 'f') {
			decrement = 87;
		} else {
			printf("%c\n", buffer[step_number]);
			printf("Offset is not in right spot.\n");
			return -1;
		}

		if (mac_prefix_index + 1 >= *mac_prefix_capacity) {
			int new_capacity = *mac_prefix_capacity << 1;
			tmp = realloc(*mac_prefix, sizeof(unsigned int) * new_capacity);

			if (tmp == NULL) {
				perror("realloc");
				return -1;
			}

			*mac_prefix_capacity = new_capacity;
			*mac_prefix = tmp;
		}

		(*mac_prefix)[mac_prefix_index] = ((*mac_prefix)[mac_prefix_index] << 4) | (buffer[step_number] - decrement);

		step_number++;
	}

	return step_number;
}

int vendor_name_parser(char *buffer, char *vendor_name, int vendor_name_count, int vendor_name_capacity) {
	int step_number = 0;
	char vendor_delimiter;

	if (buffer[0] == '"') {
		vendor_delimiter = '"';
		step_number++;
	} else {
		vendor_delimiter = ',';
	}

	while (buffer[step_number] != vendor_delimiter) {
		if (vendor_name_count + 1 >= vendor_name_capacity) {
			vendor_name_capacity = vendor_name_capacity << 1;
			vendor_name = realloc(vendor_name, sizeof(char) * vendor_name_capacity);

			if (vendor_name == NULL) {
				perror("realloc");
				return -1;
			}
		}

		vendor_name[vendor_name_count++] = buffer[step_number];
		step_number++;
	}

	vendor_name[vendor_name_count] = '\0';

	return step_number;
}
