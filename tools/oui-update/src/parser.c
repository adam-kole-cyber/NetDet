#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int mac_prefix_parser(char *buffer, mac_prefix *mac) {
	int step_number = 0;
	int decrement = 0;
	int *tmp;
	int size;

	if (mac->data_type == MA_L) {
		((ma_l_entry *)mac->data)[mac->count].oui = 0;
		size = sizeof(((ma_l_entry *)mac->data)[mac->count].oui);
	} else if (mac->data_type == MA_M) {
		((ma_m_entry *)mac->data)[mac->count].oui = 0;
		size = sizeof(((ma_m_entry *)mac->data)[mac->count].oui);
	} else if (mac->data_type == MA_S) {
		((ma_s_entry *)mac->data)[mac->count].oui = 0;
		size = sizeof(((ma_s_entry *)mac->data)[mac->count].oui);
	}

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

		if (mac->count + 1 >= mac->capacity) {
			int new_capacity = mac->capacity << 1;
			tmp = realloc(mac->data, size * new_capacity);

			if (tmp == NULL) {
				perror("realloc");
				return -1;
			}

			mac->capacity = new_capacity;
			mac->data = tmp;
		}

		if (mac->data_type == MA_L) {
			((ma_l_entry *)mac->data)[mac->count].oui =
				(((ma_l_entry *)mac->data)[mac->count].oui << 4) | (buffer[step_number] - decrement);
		} else if (mac->data_type == MA_M) {
			((ma_m_entry *)mac->data)[mac->count].oui =
				(((ma_m_entry *)mac->data)[mac->count].oui << 4) | (buffer[step_number] - decrement);
		} else if (mac->data_type == MA_S) {
			((ma_s_entry *)mac->data)[mac->count].oui =
				(((ma_s_entry *)mac->data)[mac->count].oui << 4) | (buffer[step_number] - decrement);
		}

		step_number++;
	}

	mac->count++;

	return step_number;
}

int vendor_name_parser(char *buffer, vendor_name *vendor) {
	int step_number = 0;
	char vendor_delimiter;
	vendor_entry *entry = &vendor->data[vendor->count];

	if (buffer[0] == '"') {
		vendor_delimiter = '"';
		step_number++;
	} else {
		vendor_delimiter = ',';
	}

	while (buffer[step_number] != vendor_delimiter) {
		if (entry->count + 1 >= entry->capacity) {
			int new_capacity = entry->capacity << 1;
			char *tmp = realloc(entry->vendor_name, new_capacity);

			if (tmp == NULL) {
				perror("realloc");
				return -1;
			}

			entry->capacity = new_capacity;
			entry->vendor_name = tmp;
		}

		entry->vendor_name[entry->capacity] = buffer[step_number];
		step_number++;
	}

	entry->vendor_name[entry->count] = '\0';
	vendor->count++;

	return step_number;
}
