#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdint.h>
#include "wled_device_config.h"

struct input_t {
	char input_device_name[100];
};
struct output_t {
	int packet_timeout;
	device_wled wled_devices[10];
	uint8_t wled_devices_count;
};

typedef struct user_config_t {
	struct input_t input;
	struct output_t output;
} user_config_t;

extern user_config_t *user_config;

void init_config();

#endif
