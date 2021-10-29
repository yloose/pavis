#ifndef E131_CLIENT
#define E131_CLIENT

#include <stdint.h>

typedef void* algorithm_func_t(void* arguments);

typedef struct e131_args {
	char display_name[50];
	uint8_t leds;
	char ip[15];
	unsigned short port;
	double *data;
	int status;
	void *algorithm_plugin_handle;
	algorithm_func_t *algorithm_func;
	pthread_t thread;
} e131_args_t;

double* e131_start_thread();
void e131_thread_change_algorithm(char *display_name, char new_algorithm_name[]);
void e131_thread_change_status(char *name, int new_status);
void e131_thread_change_status_for_all(int new_status);

#endif
