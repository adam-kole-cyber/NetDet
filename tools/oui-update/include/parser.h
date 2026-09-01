#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

typedef struct {
	char *vendor_name;
	int count;
	int capacity;
	uint32_t id;
} vendor_entry;

typedef struct {
	vendor_entry *data;
	int count;
	int capacity;
} vendor_name;

typedef enum { MA_L, MA_M, MA_S } ma_type;

typedef struct {
	void *data;
	ma_type data_type;
	int count;
	int capacity;
} mac_prefix;

typedef struct {
	uint32_t oui; // 24 bits are used
	uint32_t vendor_id;
} ma_l_entry;

typedef struct {
	uint32_t oui; // 28 bits are used
	uint32_t vendor_id;
} ma_m_entry;

typedef struct {
	uint64_t oui; // 36 bits are used
	uint32_t vendor_id;
} ma_s_entry;

int mac_prefix_parser(char *buffer, mac_prefix *mac);
int vendor_name_parser(char *buffer, vendor_name *vendor);

#endif
