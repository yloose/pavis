#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

double *fft_values;
unsigned short fft_values_length;
uint8_t ledcount;

uint8_t* calc() {

	double highest_number = fft_values[0];
	int hn_position = 0;
	for (size_t i = 0; i < fft_values_length; i++) {
		if (fft_values[i] > highest_number) {
			highest_number = fft_values[i];
			hn_position = i;
		}
	}

	uint8_t leds_on = (uint8_t) (highest_number / 1024 * ledcount);

	uint8_t *led_array = malloc(513 * sizeof(uint8_t));
	for (size_t i = 0; i < 513; i++) {
		led_array[i] = 0;
	}

	uint8_t first_half[leds_on * 3];

	for (size_t i = 0; i < leds_on; i++) {
		first_half[i * 3] = 0; // BLUE
		first_half[i * 3 + 1] = 255; // RED
		first_half[i * 3 + 2] = 0; // GREEN
	}

	for (size_t i = (ledcount / 2) * 3, j = 0; j < leds_on * 3; i++, j++) {
		led_array[i] = first_half[j];
	}

	for (size_t i = (ledcount / 2), j = 0; j < leds_on; i--, j++) {
		led_array[i * 3 - 3] = first_half[j * 3];
		led_array[i * 3 - 2] = first_half[j * 3 + 1];
		led_array[i * 3 - 1] = first_half[j * 3 + 2];
	}

	return led_array;
}

// Algorithm to calculate LED colors
void* algorithm_func(void *arguments) {

	struct algorithm_thread_args_t {
		double *fft_values;
		int fft_values_length;
		unsigned char *calc_values;
		uint8_t leds;
		int status;
	};

	struct algorithm_thread_args_t *args =
			(struct algorithm_thread_args_t*) arguments;
	fft_values = args->fft_values;
	fft_values_length = args->fft_values_length;
	ledcount = args->leds;

	while (1) {
		if (args->status == 0) {

			uint8_t *calc_values = calc();
			for (size_t i = 0; i < fft_values_length; i++) {
				args->calc_values[i] = calc_values[i];
			}
			free(calc_values);
			usleep(50000);

		} else if (args->status == 1) {
			usleep(250000);
			continue;
		} else if (args->status == -1) {
			break;
		}
	}

	return 0;

}
