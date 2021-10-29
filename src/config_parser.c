#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <libconfig.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include "config_parser.h"

user_config_t *user_config;

void init_config() {
	user_config_t user_config_temp;
	memset(&user_config_temp, 0, sizeof(user_config_t));
	config_t cfg;

	const char *home;
	if ((home = getenv("HOME")) == NULL)
		home = getpwuid(getuid())->pw_dir;
	char config_file[100];
	strcpy(config_file, home);
	strcat(config_file, "/.config/pavis/pavis.conf");

	// Create folder config folder if not there yet
	char config_dir[100];
	strcat(config_dir, home);
	strcat(config_dir, "/.config/pavis");
	mkdir(config_dir, 0755);

	config_init(&cfg);

	if(!config_read_file(&cfg, config_file)) {
		// syslog(LOG_ERR, stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));

		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
		            config_error_line(&cfg), config_error_text(&cfg));

	    config_destroy(&cfg);
		syslog(LOG_ERR, "Config file not accessible, exiting.");
	    exit(-1);
	}


	const char *input_device_name = NULL;
	config_lookup_string(&cfg, "input.input_device_name", &input_device_name);
	if (input_device_name != NULL) {
		strcpy(user_config_temp.input.input_device_name, input_device_name);
	} else {
		syslog(LOG_WARNING, "Config warning: Input device name is empty.");
	}

	int packet_timeout;
	if (config_lookup_int(&cfg, "output.packet_timeout", &packet_timeout)) {
		user_config_temp.output.packet_timeout = packet_timeout;
	} else {
		syslog(LOG_WARNING, "No packet timeout given, defaulting to 250000usec.");
		user_config_temp.output.packet_timeout = 250000;
	}



	config_setting_t *output_wled_devices = config_lookup(&cfg, "output.wled_devices");
	if (output_wled_devices != NULL) {

		int count = config_setting_length(output_wled_devices);
		user_config_temp.output.wled_devices_count = count;
		int i;

		for(i = 0; i < count; ++i) {
			config_setting_t *wled_device = config_setting_get_elem(output_wled_devices, i);

			const char *name;
			int leds;
			const char *ip;
			int port;

			if(config_setting_lookup_string(wled_device, "name", &name)
			           && config_setting_lookup_int(wled_device, "leds", &leds)
			           && config_setting_lookup_string(wled_device, "ip", &ip)
			           && config_setting_lookup_int(wled_device, "port", &port)) {

				strcpy(user_config_temp.output.wled_devices[i].name, name);
				user_config_temp.output.wled_devices[i].leds = leds;
				strcpy(user_config_temp.output.wled_devices[i].ip, ip);
				user_config_temp.output.wled_devices[i].port = port;


			} else {
				syslog(LOG_WARNING, "Config error: 'wled_devices' contains errors. Skipping.");
				continue;
			}
		}

	} else {
		syslog(LOG_ERR, "Config error: No output devices specified.");
	    config_destroy(&cfg);
		exit(-1);
	}

	user_config = malloc(sizeof(user_config_temp));
	memcpy(user_config, &user_config_temp, sizeof(user_config_temp));

}
