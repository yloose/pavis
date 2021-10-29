#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>


#include "color_algorithm.h"
#include "e131_client.h"

char algorithms_path[100];
char available_algorithms[30][30];
char *current_algorithm;

void init_color_algorithms_config() {

	const char *home;
	if ((home = getenv("HOME")) == NULL)
		home = getpwuid(getuid())->pw_dir;
	strcat(algorithms_path, home);
	strcat(algorithms_path, "/.config/pavis/algorithms/");

	mkdir(algorithms_path, 0755);
	
}

char* concat(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}

int string_ends_with(const char * str, const char * suffix) {
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return
    (str_len >= suffix_len) &&
    (0 == strcmp(str + (str_len-suffix_len), suffix));
}

int scan_algorithms() {
  DIR *d;
  struct dirent *dir;
  d = opendir(algorithms_path);
  if (d) {
    uint8_t counter = 0;
    while ((dir = readdir(d)) != NULL) {
      if (counter == 30) {
        syslog(LOG_WARNING, "There are currently only a maximum number of 30 algorithms supported.");
        return -1;
      }
      if (string_ends_with(dir->d_name, ".so")) {
        strncpy(available_algorithms[counter], dir->d_name, strlen(dir->d_name) - 3);
        counter++;
      }
    }
    closedir(d);
  } else {
    return -1;
  }
  return 0;
}

int change_algorithm(char new_algorithm_name[], e131_args_t *e131_client_config) {

	int new_algorithm = -1;
	for (size_t i = 0; i < sizeof(available_algorithms) / sizeof(available_algorithms[0]); i++) {
		if (strcmp(new_algorithm_name, available_algorithms[i]) == 0) {
			new_algorithm = i;
			break;
		}
	}

	if (new_algorithm == -1) {
		syslog(LOG_WARNING, "Algorithm name was not found in list of available algorithms. Try to rescan for algorithms and make sure it's there.");
		return -1;
	} else {
		if (e131_client_config->algorithm_plugin_handle != NULL) {
			if (dlclose(e131_client_config->algorithm_plugin_handle) != 0) {
				syslog(LOG_ERR, "Could not close old algorithm plugin handle: %s", dlerror());
				return -1;
			}
		}

		char *algorithm_file_path = concat(concat(algorithms_path, available_algorithms[new_algorithm]), ".so");
		if (access(algorithm_file_path, F_OK) != -1) {
			e131_client_config->algorithm_plugin_handle = dlopen(algorithm_file_path, RTLD_LAZY);
			free(algorithm_file_path);
			if (e131_client_config->algorithm_plugin_handle == NULL) {
				syslog(LOG_ERR, "Could not open algorithm file: %s", dlerror());
				return -1;
			}

			algorithm_func_t *temp_algorithm_func = (algorithm_func_t*) dlsym(e131_client_config->algorithm_plugin_handle, "algorithm_func");

			if ( (temp_algorithm_func == NULL) ) {
				syslog(LOG_ERR, "Not all required functions given in algorithm file [algorithm_init,algorithm_func,algorithm_destroy].");
				return -1;
			}

			e131_client_config->algorithm_func = temp_algorithm_func;
			return 0;
		} else {
			syslog(LOG_ERR, "Could not find algorithm in file system. Try to rescan for algorithms, make sure it's there and don't change any filenames after scanning.");
			return -1;
		}

	}
}
