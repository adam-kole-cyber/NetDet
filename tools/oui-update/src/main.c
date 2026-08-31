#include "parser.h"
#include "quicksort.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHUNK_SIZE 10240

int main(void) {
	char buffer[CHUNK_SIZE + 1];
	int mac_prefix_capacity = CHUNK_SIZE;
	int *mac_prefix = calloc(mac_prefix_capacity, sizeof(unsigned int));
	int mac_prefix_index = 0;
	int vendor_name_capacity = 100;
	char *vendor_name = calloc(vendor_name_capacity, sizeof(char));
	int vendor_name_count = 0;
	int read_size = 0;
	int current_read_size = 0;
	char *leftover_buff;
	int leftover_size = 0;
	int file_fd = open("/home/adam/C/NetDet/data/oui/ma-l.csv", O_RDONLY);

	if (file_fd == -1) {
		perror("open");
		free(vendor_name);
		free(mac_prefix);
		return -1;
	}

	while (read(file_fd, &buffer[0], 1) > 0 && buffer[0] != '\n') {
		// to get rid of header
	}

	while ((current_read_size = read(file_fd, buffer + read_size, CHUNK_SIZE - read_size)) > 0) {
		read_size += current_read_size;

		if (read_size < CHUNK_SIZE && current_read_size > 0) {
			continue;
		}

		int step_number = 0;
		char *current_index = buffer;
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
			free(vendor_name);
			free(mac_prefix);
			close(file_fd);
			return -1;
		}

		memcpy(leftover_buff, end_index, leftover_size);
		end_index[0] = '\0';

		while (current_index != end_index) {
			current_index = strchr(current_index, ',') + 1; // we need it to point right after the ','
			mac_prefix[mac_prefix_index] = 0;
			step_number = mac_prefix_parser(current_index, &mac_prefix, mac_prefix_index, &mac_prefix_capacity);
			if (step_number == -1) {
				free(vendor_name);
				free(mac_prefix);
				close(file_fd);
				return -1;
			}
			printf("%06x ", mac_prefix[mac_prefix_index]);

			mac_prefix_index++;
			current_index = current_index + step_number + 1; // this is first character of the vendors name

			step_number = vendor_name_parser(current_index, vendor_name, vendor_name_count, vendor_name_capacity);
			if (step_number == -1) {
				free(vendor_name);
				free(mac_prefix);
				close(file_fd);
				return -1;
			}
			printf("%s\n", vendor_name);

			current_index = strchr(current_index + step_number, '\n') + 1;
		}

		memcpy(buffer, leftover_buff, leftover_size);

		read_size = leftover_size;
		leftover_size = 0;

		free(leftover_buff);
		leftover_buff = NULL;
	}

	if (current_read_size < 0) {
		perror("read");
		free(vendor_name);
		free(mac_prefix);
		close(file_fd);
		return -1;
	} else {
		int step_number = 0;
		char *current_index = buffer;
		char *end_index = &buffer[read_size];

		while (current_index != end_index) {
			current_index = strchr(current_index, ',') + 1; // we need it to point right after the ','
			mac_prefix[mac_prefix_index] = 0;
			step_number = mac_prefix_parser(current_index, &mac_prefix, mac_prefix_index, &mac_prefix_capacity);
			if (step_number == -1) {
				free(vendor_name);
				free(mac_prefix);
				close(file_fd);
				return -1;
			}
			printf("%6x <- ", mac_prefix[mac_prefix_index]);

			mac_prefix_index++;
			current_index = current_index + step_number + 1; // this is first character of the vendors name

			step_number = vendor_name_parser(current_index, vendor_name, vendor_name_count, vendor_name_capacity);
			if (step_number == -1) {
				free(vendor_name);
				free(mac_prefix);
				close(file_fd);
				return -1;
			}
			printf("%s\n", vendor_name);

			current_index = strchr(current_index + step_number, '\n') + 1;
		}
	}

	free(vendor_name);
	free(mac_prefix);
	close(file_fd);
	return 0;
}
