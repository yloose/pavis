#ifndef STATE_FILE
#define STATE_FILE

void modify_status_in_state_file(char *display_name, int new_status);
int get_status_by_name_from_state_file(char *display_name);

void modify_algorithm_in_state_file(char *display_name, char *new_algorithm_name);
char *get_algorithm_by_name_from_state_file(char *display_name);

#endif
