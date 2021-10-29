#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Modify this timing to set the timeout between calculation intervals.
// Keep in mind that the update interval of the leds itself is bottlenecked at the speed of the "packet_timeout" specified in the config file
#define CALC_TIMEOUT 50000

double *fft_values;
unsigned short fft_values_length;
uint8_t ledcount;

uint8_t* calc() {

	uint8_t *led_array = malloc(513 * sizeof(uint8_t));

	// Here you can implement logic to calculate your light data and write it to "led_array"
	// It is stored in rgb values for each led in the order of the leds on the strip controlled by the WLED device.
	// An example array for 10 leds with the first 5 being blue and last five being red:
	// [0, 0, 255, 0, 0, 255, 0, 0, 255, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 0, 0, 255, 0, 0, 255, 0, 0, 255, 0, 0]

	return led_array;
}


// This is helper function which handles the state of the algorithm thread and communicates with the pavis daemon
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
			usleep(CALC_TIMEOUT);

		} else if (args->status == 1) {
			usleep(250000);
			continue;
		} else if (args->status == -1) {
			break;
		}
	}

	return 0;

}
