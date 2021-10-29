#include <sys/socket.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>

#include "color_algorithm.h"
#include "e131_client.h"

pthread_t socket_server_thread;
int8_t socket_server_status;
const char *socket_path = "/tmp/pavis.sock";

void handle_request(int rc, char buf[]) {

  syslog(LOG_INFO, "Received socket connection.");

  if (strncmp(buf, ":rescan", 7) == 0) {

    if (scan_algorithms())
      syslog(LOG_WARNING, "Failed to rescan for algorithms.");

  } else if (strncmp(buf, ":select", 7) == 0) {

	  buf[strcspn(buf, "\n")] = 0;
	  char *arguments = buf + 8;
	  char *algorithm_name = strtok(arguments, " ");
	  char *device_name = strtok(NULL, " ");

	  e131_thread_change_algorithm(device_name, algorithm_name);

  } else if (strncmp(buf, ":pause", 6) == 0) {

	  if (rc == 7) {
		  e131_thread_change_status_for_all(1);
	  } else {
		  char *device = buf + 7;
		  device[strlen(device)-1] = 0;
	  	  e131_thread_change_status(device, 1);
	  }

  } else if (strncmp(buf, ":resume", 7) == 0) {

	  if (rc == 8) {
	  	e131_thread_change_status_for_all(0);
	  } else {
		  char *device = buf + 8;
		  device[strlen(device)-1] = 0;
  	  	  e131_thread_change_status(device, 0);
	  }

  } else {
      syslog(LOG_WARNING, "Socket command not found.");
  }
}

void *socket_handler(void* arguments) {
  int8_t *status = (int8_t*) arguments;

  struct sockaddr_un addr;

  int fd,cl,rc;
  char buf[100];

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    syslog(LOG_ERR, "Could not create socket.");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
  unlink(socket_path);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    syslog(LOG_ERR, "Could not bind socket to adress.");
    exit(-1);
  }

  chmod(socket_path, 666);

  if (listen(fd, 5) == -1) {
    syslog(LOG_ERR, "Could not open socket for listening.");
    exit(-1);
  }

  // Wait for incoming requests on the socket

  while (status) {
    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      syslog(LOG_ERR, "Could not accept message from socket.");
      continue;
    }

    while ( (rc=read(cl, buf, sizeof(buf))) > 0) {
      handle_request(rc, buf);
    }

    if (rc == -1) {
      syslog(LOG_ERR, "Could not read from socket.");
      exit(-1);
    }
    else if (rc == 0) {
      close(cl);
    }
  }
  
  return NULL;
}

void start_socket_server() {
  pthread_create(&socket_server_thread, NULL, socket_handler, &socket_server_status);
}

void stop_socket_server() {
  socket_server_status = 0;
}
