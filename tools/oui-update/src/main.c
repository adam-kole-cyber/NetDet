#include "quicksort.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
	char *line = NULL;
	char character;
	int capacity = 128;
	int size = 0;
	int mac_table[10];
	int count = 0;

	int file_fd = open("~/C/NetDet/data/oui/test.csv", O_RDONLY);
	line = calloc(capacity, sizeof(char));

	while (read(file_fd, &character, 1) > 0 && character != '\n') {
	}

	while (read(file_fd, &character, 1) > 0) {
		if (size + 1 >= capacity) {
			capacity = capacity << 1;
			line = realloc(line, capacity);

			if (line == NULL) {
				perror("realloc");
				return -1;
			}
		}

		if (character == '\n') {
			char *oui = strchr(line, ',') + 1;
			int i = 0;
			mac_table[count] = 0;
			int decrement;

			while (oui[i] != ',') {
				if (oui[i] > '9') {
					decrement = 55;
				} else {
					decrement = 48;
				}

				mac_table[count] = mac_table[count] << 4;
				mac_table[count] = mac_table[count] | (oui[i] - decrement);

				i++;
			}

			count++;
			line[size] = '\0';
			size = 0;
			continue;
		}

		line[size++] = character;
	}

	line[size] = '\0';

	printf("unsorted: ");
	for (int i = 0; i < count; i++) {
		printf("%x ", mac_table[i]);
	}
	printf("\n");

	quicksort(mac_table, 10);

	printf("sorted: ");
	for (int i = 0; i < count; i++) {
		printf("%x ", mac_table[i]);
	}
	printf("\n");

	free(line);
	close(file_fd);
	return 0;
}
