#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <math.h>
#include <fftw3.h>

#include "e131_client.h"
#include "color_algorithm.h"
#include "config_parser.h"

#define BUFSIZE 1024
#define SAMPLE_RATE 44100

int record_loop_status;

fftw_plan fftw_plan_audio;


fftw_plan fftw_init(double *input_samples_buf, fftw_complex *transformed_samples_buf) {
  return fftw_plan_dft_r2c_1d(BUFSIZE, input_samples_buf, transformed_samples_buf, 0);
}

/* Read buffer */
void read_buffer(uint8_t *buffer, size_t size, double *e131_data, double *input_samples_buf, fftw_complex *transformed_samples_buf) {
  int16_t *samples = (int16_t*) buffer;

  for (size_t i = 0; i < BUFSIZE/2; i++) {
	input_samples_buf[i] = (double) samples[i] / 32768.0;
  }

  fftw_execute(fftw_plan_audio);

  for (size_t i = 0; i < BUFSIZE/2; i++) {
	e131_data[i] = sqrt(transformed_samples_buf[i][0]*transformed_samples_buf[i][0] + transformed_samples_buf[i][1]*transformed_samples_buf[i][1]);
  }

}

int get_default_audio_sink(char *default_sink) {

  FILE *fp;
  char path[1035];

  /* Open the command for reading. */
  fp = popen("pacmd list-sinks | awk '/* index/, /name/' | grep name | sed 's/.$//' | sed 's/\\s*name: <//' | sed 's/$/.monitor/'", "r");
  if (fp == NULL) {
	return 0;
  }


  // Read output
  fgets(default_sink, 200, fp);

  // Remove trailing newline
  default_sink[strcspn(default_sink, "\n")] = 0;

  /* close */
  pclose(fp);

  return 1;

}


void start_recording_loop() {

  // Temporary
  scan_algorithms();

  // Create fftw plan
  double* input_samples_buf = (double*) malloc(sizeof(double) * (2*(BUFSIZE/2+1)) );
  fftw_complex* transformed_samples_buf = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * BUFSIZE);
  fftw_plan_audio = fftw_init(input_samples_buf, transformed_samples_buf);

  double *e131_data = e131_start_thread();

  /* The sample type to use */
  static const pa_sample_spec ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = SAMPLE_RATE,
	.channels = 1
  };
  pa_simple *s = NULL;
  int error;

  // TODO: Sanitize the following if-else chain
  /* Create the recording stream */
  if ( user_config->input.input_device_name[0] == '\0' ) {
	syslog(LOG_WARNING, "Not pulseaudio device specified using the default devices monitor.");

	char default_sink[200];
	if (!(get_default_audio_sink(default_sink))) {

	  syslog(LOG_WARNING, "Could not get default audio sink with pacmd, using the default audio sink from pulseaudio.");

	  if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, default_sink, "Pavis FFT record", &ss, NULL, NULL, &error))) {
		syslog(LOG_ERR, "Could not initialize Pulseaudio stream with default sink.");
		goto finish;
	  }

	} else {

	  if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, default_sink, "Pavis FFT record", &ss, NULL, NULL, &error))) {
		syslog(LOG_WARNING, "Could not initialize Pulseaudio stream with default sink from pacmd: %s", default_sink);

		if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, NULL, "Pavis FFT record", &ss, NULL, NULL, &error))) {
		  syslog(LOG_ERR, "Could not initialize Pulseaudio stream with default sink.");
		  goto finish;
		}
	  }	  

	}	
  } else {

	if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, user_config->input.input_device_name, "Pavis FFT record", &ss, NULL, NULL, &error))) {
	  syslog(LOG_WARNING, "Could not initialize Pulseaudio stream with sink given in config file: %s. Using the default device.", user_config->input.input_device_name);
	}

	char default_sink[200];
	if (!(get_default_audio_sink(default_sink))) {

	  syslog(LOG_WARNING, "Could not get default audio sink with pacmd, using the default audio sink from pulseaudio.");

	  if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, NULL, "Pavis FFT record", &ss, NULL, NULL, &error))) {
		syslog(LOG_ERR, "Could not initialize Pulseaudio stream with default sink.");
		goto finish;
	  }

	} else {

	  if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, default_sink, "Pavis FFT record", &ss, NULL, NULL, &error))) {
		syslog(LOG_WARNING, "Could not initialize Pulseaudio stream with default sink from pacmd: %s", default_sink);

		if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, NULL, "Pavis FFT record", &ss, NULL, NULL, &error))) {
		  syslog(LOG_ERR, "Could not initialize Pulseaudio stream with default sink.");
		  goto finish;
		}
	  }	  
	} 
  }

 
  for (;;) {
	if (record_loop_status == 1) {
	  if (s)
		pa_simple_free(s);
	  return;
	}
	uint8_t buf[BUFSIZE];
	/* Record some data ... */
	if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
	  syslog(LOG_ERR, "Could not read from PulseAudio stream: %s", pa_strerror(error));
	  goto finish;
	}

	read_buffer(buf, sizeof(buf), e131_data, input_samples_buf, transformed_samples_buf);
  }
finish:
  if (s)
	pa_simple_free(s);
  exit(-1);
}

void stop_recording_loop() {
  record_loop_status = 1;
}
