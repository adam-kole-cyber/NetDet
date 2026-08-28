#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void swap(int *x, int *y) {
	int temp = *x;
	*x = *y;
	*y = temp;
	return;
}

int sort_subarray(int *arr, int low_element, int high_element) {
	int selected_index = low_element + (rand() % (high_element - low_element));

	if (selected_index != high_element) {
		swap(&arr[selected_index], &arr[high_element]);
	}

	int selected_element = arr[high_element];
	int i = low_element;

	for (int j = low_element; j < high_element; j++) {
		if (arr[j] <= selected_element) {
			swap(&arr[i], &arr[j]);
			i++;
		}
	}

	swap(&arr[i], &arr[high_element]);

	return i;
}

void split_array(int *arr, int low_element, int high_element) {
	if (low_element < high_element) {
		int selected_element = sort_subarray(arr, low_element, high_element);

		split_array(arr, low_element, selected_element - 1);
		split_array(arr, selected_element + 1, high_element);
	}
	return;
}

void quicksort(int *arr, int length) {
	srand(time(NULL));
	split_array(arr, 0, length - 1);
	return;
}

int main(void) {
	int arr[] = {5, 28, 13, 19, 4, 38, 9, 33, 21};
	int length = 9;

	printf("Unsorted: ");
	for (int i = 0; i < length; i++) {
		printf("%d ", arr[i]);
	}

	quicksort(arr, length);

	printf("\nSorted: ");
	for (int i = 0; i < length; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");

	return 0;
}

// 6 7 5 2 1 3 6 9 4 2 8 [3]
//          2 1 3 [2] <- [3] -> 6 7 5 6 9 4 [8]
//       2 [1] <- [2] -> [3] -> 6 7 5 6 4 <- [8] -> 9
//         [1] -> 2
