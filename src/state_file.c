#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "state_file.h"

#define STATE_FILE_LOCATION "/var/lib/pavis/state_file"

typedef struct e131_state {
	char display_name[50];
	int status;
	char algorithm_name[30];
} e131_state_t;


// Rework logic in function
void modify_entry_in_state_file(char *display_name, int new_status, char *new_algorithm_name) {

	FILE *state_file;
	e131_state_t input;

	e131_state_t new_e131_state = { .status = 0 ? new_status < 0 : new_status };
	strcpy(new_e131_state.display_name, display_name);
	if (new_algorithm_name != NULL)
		strcpy(new_e131_state.algorithm_name, new_algorithm_name);

	int index = 0;

	// Create state file if it does not exist
	state_file = fopen(STATE_FILE_LOCATION, "a");
	fclose(state_file);

	if (state_file = fopen(STATE_FILE_LOCATION, "r+")) {

		while (fread(&input, sizeof(e131_state_t), 1, state_file)) {
			// First character of display name in file is sometimes new line or null terminator
			char *display_name_mod = input.display_name;
			if (input.display_name[0] == '\n' || input.display_name[0] == '\0')
				display_name_mod++;

			if (strcmp(display_name_mod, display_name) == 0) {
				if (new_status < 0)
					new_e131_state.status = input.status;
				printf("\nTEST: %s\n\n", input.algorithm_name);
				if (new_algorithm_name == NULL)
					strcpy(new_e131_state.algorithm_name, input.algorithm_name);
				goto found;
			}
			index++;
		}

		fseek(state_file, 0, SEEK_END);
		fwrite(&new_e131_state, sizeof(e131_state_t), 1, state_file);
		fclose(state_file);
		return;

	} else {
		syslog(LOG_WARNING, "Could not read state file.");
		return;
	}

	found:
	printf("\n\n");
	printf("%s\n", new_e131_state.algorithm_name);
	printf("%s", new_algorithm_name);
	printf("\n\n");
	fseek(state_file, index * sizeof(e131_state_t), SEEK_SET);
	fwrite(&new_e131_state, sizeof(e131_state_t), 1, state_file);
	fclose(state_file);

}

e131_state_t *get_entry_by_name_from_state_file(char *display_name) {

	FILE *state_file;
	e131_state_t *input = malloc(sizeof(*input));
	
	// Create state file if it does not exist
	state_file = fopen(STATE_FILE_LOCATION, "a");
	fclose(state_file);


	if (state_file = fopen(STATE_FILE_LOCATION, "r")) {

		while (fread(input, sizeof(e131_state_t), 1, state_file)) {
			// First character of display name in file is sometimes new line or null terminator
			char *display_name_mod = input->display_name;
			if (input->display_name[0] == '\n' || input->display_name[0] == '\0')
				display_name_mod++;

			if (strcmp(display_name_mod, display_name) == 0) {
				fclose(state_file);
				return input;
			}
		}

		fclose(state_file);
		modify_entry_in_state_file(display_name, 0, NULL);
		return NULL;
	} else {
		syslog(LOG_WARNING, "Could not read state file.");
		return NULL;
	}
}

void modify_status_in_state_file(char *display_name, int new_status) {
	modify_entry_in_state_file(display_name, new_status, NULL);
}
int get_status_by_name_from_state_file(char *display_name) {
	e131_state_t *struct_from_state_file = get_entry_by_name_from_state_file(display_name);
	if (struct_from_state_file == NULL) {
		free(struct_from_state_file);
		return 0;
	} else {
		int return_value = struct_from_state_file->status;
		free(struct_from_state_file);
		return return_value;
	}
}

void modify_algorithm_in_state_file(char *display_name, char *new_algorithm_name) {
	modify_entry_in_state_file(display_name, -1, new_algorithm_name);
}
char *get_algorithm_by_name_from_state_file(char *display_name) {
	e131_state_t *struct_from_state_file = get_entry_by_name_from_state_file(display_name);
	if (struct_from_state_file == NULL) {
		free(struct_from_state_file);
		return NULL;
	} else {
		char *algorithm_name_mod = struct_from_state_file->algorithm_name;
		if (algorithm_name_mod[0] == '\n' || algorithm_name_mod[0] == '\0')
			algorithm_name_mod++;

		free(struct_from_state_file);
		return algorithm_name_mod;
	}
}
