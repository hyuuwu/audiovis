#include "audio.h"
#include "config.h"
#include "fft.h"
#include "render.h"
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static volatile int running = 1;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
  (void)sig; // Unused
  running = 0;
}

int main(int argc, char **argv) {
  // Parse command line arguments
  const char *config_file = "config.ini";
  if (argc > 1) {
    config_file = argv[1];
  }

  // Load configuration
  config_t config;
  config_load(config_file, &config);

  // Initialize subsystems
  audio_context_t *audio = audio_init(&config);
  if (!audio) {
    fprintf(stderr, "Failed to initialize audio capture\n");
    return 1;
  }

  fft_context_t *fft =
      fft_init(config.sample_rate, config.buffer_size, &config);
  if (!fft) {
    fprintf(stderr, "Failed to initialize FFT\n");
    audio_cleanup(audio);
    return 1;
  }

  if (!render_init(&config)) {
    fprintf(stderr, "Failed to initialize renderer\n");
    fft_cleanup(fft);
    audio_cleanup(audio);
    return 1;
  }

  // Set up signal handlers
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Allocate buffers
  float *audio_buffer = malloc(config.buffer_size * sizeof(float));
  float *magnitudes = malloc(config.bar_count * sizeof(float));

  if (!audio_buffer || !magnitudes) {
    fprintf(stderr, "Failed to allocate buffers\n");
    render_cleanup();
    fft_cleanup(fft);
    audio_cleanup(audio);
    return 1;
  }

  // Frame timing
  struct timespec frame_time;
  long frame_delay_ns = 1000000000L / config.fps;

  // Main loop
  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &frame_time);
    long frame_start_ns = frame_time.tv_sec * 1000000000L + frame_time.tv_nsec;

    // Check for user input
    int ch = getch();
    if (ch == 'q' || ch == 'Q' || ch == 27) { // q or ESC
      break;
    }

    // Get audio data
    audio_get_buffer(audio, audio_buffer, config.buffer_size);

    // Process audio with FFT
    fft_process(fft, audio_buffer, magnitudes, config.bar_count);

    // Render visualization
    render_frame(magnitudes, config.bar_count, &config);

    // Frame rate limiting
    clock_gettime(CLOCK_MONOTONIC, &frame_time);
    long frame_end_ns = frame_time.tv_sec * 1000000000L + frame_time.tv_nsec;
    long elapsed_ns = frame_end_ns - frame_start_ns;
    long sleep_ns = frame_delay_ns - elapsed_ns;

    if (sleep_ns > 0) {
      struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = sleep_ns};
      nanosleep(&sleep_time, NULL);
    }
  }

  // Cleanup

  free(magnitudes);
  free(audio_buffer);

  render_cleanup();
  fft_cleanup(fft);
  audio_cleanup(audio);

  return 0;
}
