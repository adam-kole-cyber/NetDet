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

void cleanup(vendor_name *vendor, mac_prefix *mac_prefix, int file_fd) {
	for (int i = 0; i < vendor->count; i++) {
		free(vendor->data[i].vendor_name);
	}

	free(vendor->data);
	free(mac_prefix->data);
	close(file_fd);

	return;
}

int process_data(char *buffer, char *end_index, mac_prefix *mac, vendor_name *vendor) {
	int step_number = 0;
	char *current_index = buffer;

	while (current_index != end_index) {
		current_index = strchr(current_index, ',') + 1; // we need it to point right after the ','

		step_number = mac_prefix_parser(current_index, mac);
		if (step_number == -1) {
			return -1;
		}

		if (mac->data_type == MA_L) {
			printf("%06x <- ", ((ma_l_entry *)mac->data)[mac->count - 1].oui);
		} else if (mac->data_type == MA_M) {
			printf("%06x <- ", ((ma_m_entry *)mac->data)[mac->count - 1].oui);
		} else if (mac->data_type == MA_S) {
			printf("%lx <- ", ((ma_s_entry *)mac->data)[mac->count - 1].oui);
		}

		current_index = current_index + step_number + 1; // this is first character of the vendors name

		step_number = vendor_name_parser(current_index, vendor);
		if (step_number == -1) {
			return -1;
		}
		printf("%s\n", vendor->data[vendor->count - 1].vendor_name);

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
	int file_fd = open("/home/adam/C/NetDet/data/oui/test-ma-s.csv", O_RDONLY);

	mac.capacity = CHUNK_SIZE;
	mac.count = 0;
	mac.data_type = MA_S;
	mac.data = calloc(mac.capacity, sizeof(ma_s_entry));

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
			cleanup(&vendor, &mac, file_fd);
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
			cleanup(&vendor, &mac, file_fd);
			return -1;
		}

		memcpy(leftover_buff, end_index, leftover_size);
		end_index[0] = '\0';

		if (process_data(buffer, end_index, &mac, &vendor)) {
			cleanup(&vendor, &mac, file_fd);
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
			cleanup(&vendor, &mac, file_fd);
			return -1;
		}
	} else {
		perror("read");
		cleanup(&vendor, &mac, file_fd);
		return -1;
	}

	quicksort_mac(&mac);
	for (int i = 0; i < mac.count; i++) {
		if (mac.data_type == MA_L) {
			printf("%06x\n", ((ma_l_entry *)mac.data)[i].oui);
		} else if (mac.data_type == MA_M) {
			printf("%06x\n", ((ma_m_entry *)mac.data)[i].oui);
		} else if (mac.data_type == MA_S) {
			printf("%06lx\n", ((ma_s_entry *)mac.data)[i].oui);
		}
	}

	cleanup(&vendor, &mac, file_fd);
	return 0;
}
