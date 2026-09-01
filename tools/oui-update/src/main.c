#include "parser.h"
#include "quicksort.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEADER_SIZE 59
#define CHUNK_SIZE 10240

void error_close(vendor_entry *vendor_name, int *mac_prefix, int file_fd) {
	free(vendor_name);
	free(mac_prefix);
	close(file_fd);

	return;
}

int process_data(char *buffer, char *end_index, mac_prefix *mac, vendor_name *vendor) {
	int step_number = 0;
	char *current_index = buffer;
	uint64_t oui;

	if (mac->data_type == MA_L) {
		oui = ((ma_l_entry *)mac->data)[mac->count].oui = 0;
	} else if (mac->data_type == MA_M) {
		oui = ((ma_m_entry *)mac->data)[mac->count].oui = 0;
	} else if (mac->data_type == MA_S) {
		oui = ((ma_s_entry *)mac->data)[mac->count].oui = 0;
	}

	while (current_index != end_index) {
		current_index = strchr(current_index, ',') + 1; // we need it to point right after the ','

		step_number = mac_prefix_parser(current_index, mac);
		if (step_number == -1) {
			return -1;
		}
		printf("%06lx <- ", oui);

		current_index = current_index + step_number + 1; // this is first character of the vendors name

		step_number = vendor_name_parser(current_index, vendor);
		if (step_number == -1) {
			return -1;
		}
		printf("%s\n", vendor->data[vendor->count].vendor_name);

		current_index = strchr(current_index + step_number, '\n') + 1;
	}

	return 0;
}

int main(void) {
	char buffer[CHUNK_SIZE + 1];
	mac_prefix mac;
	vendor_name vendor;
	char *leftover_buff;
	int leftover_size = 0;
	int read_size = 0;
	int current_read_size = 0;
	int file_fd = open("/home/adam/C/NetDet/data/oui/ma-l.csv", O_RDONLY);

	mac.capacity = CHUNK_SIZE;
	mac.count = 0;
	mac.data_type = MA_L;
	mac.data = calloc(mac.capacity, sizeof(ma_l_entry));

	vendor.capacity = 100;
	vendor.count = 0;
	vendor.data = calloc(vendor.capacity, sizeof(vendor_entry));

	if (file_fd == -1) {
		perror("open");
		free(vendor.data);
		free(mac.data);
		return -1;
	}

	while (read_size != HEADER_SIZE) {
		// reads the header, including the newline character at the end of the header
		current_read_size = read(file_fd, buffer, HEADER_SIZE);
		if (current_read_size <= 0) {
			printf("Something went wrong while processing the file header.\n");
			error_close(vendor.data, mac.data, file_fd);
			return -1;
		}

		read_size += current_read_size;
	}

	buffer[HEADER_SIZE] = '\0';
	read_size = 0;

	while ((current_read_size = read(file_fd, buffer + read_size, CHUNK_SIZE - read_size)) > 0) {
		read_size += current_read_size;

		if (read_size < CHUNK_SIZE && current_read_size > 0) {
			continue;
		}

		char *end_index;
		int i = CHUNK_SIZE - 1;

		while (i >= 0 && buffer[i] != '\n') {
			leftover_size++;
			i--;
		}

		end_index = &buffer[i + 1];

		leftover_buff = calloc(leftover_size, sizeof(char));
		if (leftover_buff == NULL) {
			perror("calloc");
			error_close(vendor.data, mac.data, file_fd);
			return -1;
		}

		memcpy(leftover_buff, end_index, leftover_size);
		end_index[0] = '\0';

		if (process_data(buffer, end_index, &mac, &vendor)) {
			error_close(vendor.data, mac.data, file_fd);
			return -1;
		}

		memcpy(buffer, leftover_buff, leftover_size);

		read_size = leftover_size;
		leftover_size = 0;

		free(leftover_buff);
		leftover_buff = NULL;
	}

	if (current_read_size >= 0) {
		char *end_index = &buffer[read_size];
		if (process_data(buffer, end_index, &mac, &vendor) < 0) {
			error_close(vendor.data, mac.data, file_fd);
			return -1;
		}
	} else {
		perror("read");
		error_close(vendor.data, mac.data, file_fd);
		return -1;
	}

	/*quicksort(mac.data, mac.count);
	for (int i = 0; i < mac.count; i++) {
		printf("%06x\n", mac.data[i]);
	}*/

	free(vendor.data);
	free(mac.data);
	close(file_fd);
	return 0;
}
