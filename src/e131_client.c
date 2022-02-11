#include <e131.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include "e131_client.h"
#include "color_algorithm.h"
#include "config_parser.h"
#include "state_file.h"

e131_args_t *arguments[10];

void* e131_thread_entry(void *arguments) {
	e131_args_t *args = (e131_args_t*) arguments;

	int sockfd;
	e131_packet_t packet;
	e131_addr_t dest;

	// create a socket for E1.31
	if ((sockfd = e131_socket()) < 0) {
		syslog(LOG_ERR, "Could not initialize e131 socket.");
		exit(-1);
	}

	// initialize the new E1.31 packet in universe 1 with 24 slots in preview mode
	e131_pkt_init(&packet, 1, 512);
	memcpy(&packet.frame.source_name, "E1.31 Test Client", 18);

	if (e131_set_option(&packet, E131_OPT_PREVIEW, false) < 0) {
		syslog(LOG_ERR, "Could not initialize E1.31 socket with options.");
		exit(-1);
	}

	// set remote system destination as unicast address
	if (e131_unicast_dest(&dest, args->ip, args->port) < 0) {
		syslog(LOG_ERR,
				"Could not setup E1.31 socket with given network address.");
		exit(-1);
	}

	pthread_t algorithm_thread = 0;
	struct algorithm_thread_args_t {
		double *fft_values;
		int fft_values_length;
		unsigned char *calc_values;
		uint8_t leds;
		int status;
	};

	unsigned char calc_values[513] = { 0 };

	struct algorithm_thread_args_t algorithm_thread_args = { args->data, 513, calc_values, args->leds, args->status };

	init_thread:

	if (args->algorithm_func != NULL)
		pthread_create(&algorithm_thread, NULL, args->algorithm_func, &algorithm_thread_args);

	while (1) {
		if (args->algorithm_func == NULL) {
			sleep(500);
			continue;
		}
		if (algorithm_thread == 0)
			goto init_thread;

		if (algorithm_thread_args.status == 1) {
		  usleep(250000);
		  continue;
		}

		for (size_t i = 0; i < 513; i++) {
			packet.dmp.prop_val[i] = algorithm_thread_args.calc_values[i];
		}
		if (e131_send(sockfd, &packet, &dest) < 0) {
			syslog(LOG_WARNING, "Failed to send E1.31 packet.");
		}
		packet.frame.seq_number++;
		usleep(user_config->output.packet_timeout);

	}

	return 0;
}

e131_args_t* get_thread_by_display_name(char *display_name) {
	for (size_t i = 0; i < user_config->output.wled_devices_count; i++) {
		if (strcmp(arguments[i]->display_name, display_name) == 0) {
			return arguments[i];
		}
	}
	return 0;
}

double* e131_start_thread() {

	double temp[513] = { 0 };
	double *data = malloc(sizeof(double) * 513);
	memcpy(data, &temp, 513);

	for (size_t i = 0; i < user_config->output.wled_devices_count; i++) {

		arguments[i] = calloc(1, sizeof(e131_args_t));

		arguments[i]->data = data;
		arguments[i]->status = get_status_by_name_from_state_file(
				user_config->output.wled_devices[i].name);

		strcpy(arguments[i]->display_name,
				user_config->output.wled_devices[i].name);
		arguments[i]->leds = user_config->output.wled_devices[i].leds;
		strcpy(arguments[i]->ip, user_config->output.wled_devices[i].ip);
		arguments[i]->port = user_config->output.wled_devices[i].port;

		char *state_saved_algorithm = get_algorithm_by_name_from_state_file(
				user_config->output.wled_devices[i].name);
		if (state_saved_algorithm != NULL && state_saved_algorithm[0] != '\0') {
			if (change_algorithm(state_saved_algorithm, arguments[i]) == 0) {
				arguments[i]->status = 0;
			} else {
				arguments[i]->status = 1;
			}
		} else {
			arguments[i]->status = 1;
		}

		pthread_create(&arguments[i]->thread, NULL, e131_thread_entry,
				arguments[i]);
	}

	return data;
}

void e131_thread_change_algorithm(char *display_name, char new_algorithm_name[]) {

	e131_args_t *thread;

	if (thread = get_thread_by_display_name(display_name)) {
		if (change_algorithm(new_algorithm_name, thread) != 0)
			return;
		modify_algorithm_in_state_file(display_name, new_algorithm_name);
		syslog(LOG_INFO, "Changing algorithm of %s to %s.", display_name, new_algorithm_name);

	} else {
		syslog(LOG_ERR, "Could not change algorithm. Device name not found.");
	}
}

void e131_thread_change_status(char *display_name, int new_status) {

	e131_args_t *thread;

	if (thread = get_thread_by_display_name(display_name)) {
		if (new_status != -1)
			modify_status_in_state_file(display_name, new_status);
		thread->status = new_status;
	} else {
		syslog(LOG_WARNING, "Could not change status of E1.31 thread.");
	}
}

void e131_thread_change_status_for_all(int new_status) {
	for (size_t i = 0; i < user_config->output.wled_devices_count; i++) {
		if (new_status != -1)
			modify_status_in_state_file(arguments[i]->display_name, new_status);
		arguments[i]->status = new_status;
	}
}
