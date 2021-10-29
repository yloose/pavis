#include <stdint.h>

#ifndef LEDCOUNT
#define LEDCOUNT 52

#include "e131_client.h"

extern char algorithms_path[100];
extern char available_algorithms[30][30];
extern char *current_algorithm;

void init_color_algorithms_config();
int scan_algorithms();
int string_ends_with(const char * str, const char * suffix);
int select_algorithm(char new_algorithm_name[15]);
unsigned char *apply_algorithm(double fft_values[], unsigned short fft_values_length, uint8_t ledcount);

int change_algorithm(char new_algorithm_name[], e131_args_t *e131_client_config);

#endif
