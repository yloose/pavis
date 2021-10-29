#include <stdlib.h>
#include <syslog.h>
#include <signal.h>

#include "socket_server.h"
#include "audio_handling.h"
#include "e131_client.h"
#include "config_parser.h"
#include "color_algorithm.h"


void int_handler(int number);

int main(int argc, char*argv[]) {

  openlog("pavis", 0, LOG_DAEMON);

  signal(SIGINT, int_handler);

  init_config();
  init_color_algorithms_config();

  start_socket_server();

  start_recording_loop();
}

void int_handler(int number) {
  stop_socket_server();
  e131_thread_change_status_for_all(-1);
  stop_recording_loop();

 exit(0);
}
