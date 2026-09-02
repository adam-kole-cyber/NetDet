#include "quicksort.h"
#include "parser.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static void swap(void *x, void *y, ma_type data_type) {
	if (data_type == MA_L) {
		ma_l_entry temp = *((ma_l_entry *)x);
		*((ma_l_entry *)x) = *((ma_l_entry *)y);
		*((ma_l_entry *)y) = temp;
	} else if (data_type == MA_M) {
		ma_m_entry temp = *((ma_m_entry *)x);
		*((ma_m_entry *)x) = *((ma_m_entry *)y);
		*((ma_m_entry *)y) = temp;
	} else if (data_type == MA_S) {
		ma_s_entry temp = *((ma_s_entry *)x);
		*((ma_s_entry *)x) = *((ma_s_entry *)y);
		*((ma_s_entry *)y) = temp;
	}

	return;
}

static int sort_subarray(void *arr, ma_type data_type, int low_element, int high_element) {
	int selected_index = low_element + (rand() % (high_element - low_element));

	if (selected_index != high_element) {
		if (data_type == MA_L) {
			swap(&((ma_l_entry *)arr)[selected_index], &((ma_l_entry *)arr)[high_element], data_type);
		} else if (data_type == MA_M) {
			swap(&((ma_m_entry *)arr)[selected_index], &((ma_m_entry *)arr)[high_element], data_type);
		} else if (data_type == MA_S) {
			swap(&((ma_s_entry *)arr)[selected_index], &((ma_s_entry *)arr)[high_element], data_type);
		}
	}

	// int selected_element = arr[high_element];
	int i = low_element;

	for (int j = low_element; j < high_element; j++) {
		if (data_type == MA_L) {
			if (((ma_l_entry *)arr)[j].oui <= ((ma_l_entry *)arr)[high_element].oui) {
				swap(&((ma_l_entry *)arr)[i], &((ma_l_entry *)arr)[j], data_type);
				i++;
			}
		} else if (data_type == MA_M) {
			if (((ma_m_entry *)arr)[j].oui <= ((ma_m_entry *)arr)[high_element].oui) {
				swap(&((ma_m_entry *)arr)[i], &((ma_m_entry *)arr)[j], data_type);
				i++;
			}
		} else if (data_type == MA_S) {
			if (((ma_s_entry *)arr)[j].oui <= ((ma_s_entry *)arr)[high_element].oui) {
				swap(&((ma_s_entry *)arr)[i], &((ma_s_entry *)arr)[j], data_type);
				i++;
			}
		}
		/*if (arr[j] <= selected_element) {
			swap(&arr[i], &arr[j]);
			i++;
		}*/
	}

	if (data_type == MA_L) {
		swap(&((ma_l_entry *)arr)[i], &((ma_l_entry *)arr)[high_element], data_type);
	} else if (data_type == MA_M) {
		swap(&((ma_m_entry *)arr)[i], &((ma_m_entry *)arr)[high_element], data_type);
	} else if (data_type == MA_S) {
		swap(&((ma_s_entry *)arr)[i], &((ma_s_entry *)arr)[high_element], data_type);
	}
	// swap(&arr[i], &arr[high_element]);

	return i;
}

static void split_array(void *arr, ma_type data_type, int low_element, int high_element) {
	if (low_element < high_element) {
		int selected_element = sort_subarray(arr, data_type, low_element, high_element);

		split_array(arr, data_type, low_element, selected_element - 1);
		split_array(arr, data_type, selected_element + 1, high_element);
	}
	return;
}

void quicksort_mac(mac_prefix *mac) {
	srand(time(NULL));
	split_array(mac->data, mac->data_type, 0, mac->count - 1);
	return;
}
