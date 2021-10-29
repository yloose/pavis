#include <stdlib.h>
#include <syslog.h>
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

  /* Create the recording stream */
  if ( user_config->input.input_device_name[0] ==  '\0' ) {
	if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, NULL, "Pulseaudio LED FFT record", &ss, NULL, NULL, &error))) {
	  syslog(LOG_ERR, "Could not initialize PulseAudio stream with default sink: %s", pa_strerror(error));
	  goto finish;
	}	
  } else {
	if ( !(s = pa_simple_new(NULL, "pavis", PA_STREAM_RECORD, user_config->input.input_device_name, "Pulseaudio LED FFT record", &ss, NULL, NULL, &error))) {
	  syslog(LOG_ERR, "Could not initialize PulseAudio stream given sink name: %s", pa_strerror(error));
	  goto finish;
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
	  fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
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
