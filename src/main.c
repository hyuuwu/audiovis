#include "audio.h"
#include "config.h"
#include "config_editor.h"
#include "fft.h"
#include "render.h"
#include <errno.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define CONFIG_DIR ".config/audiovis"
#define CONFIG_FILE "config.ini"

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

/* Get the config file path: ~/.config/audiovis/config.ini */
static int get_config_path(char *path, size_t size) {
  const char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "Could not get HOME directory\n");
    return -1;
  }
  snprintf(path, size, "%s/%s/%s", home, CONFIG_DIR, CONFIG_FILE);
  return 0;
}

/* Create config directory if it doesn't exist */
static int ensure_config_dir(void) {
  const char *home = getenv("HOME");
  if (!home)
    return -1;

  char dir[512];
  snprintf(dir, sizeof(dir), "%s/%s", home, CONFIG_DIR);

  struct stat st;
  if (stat(dir, &st) == 0)
    return 0; /* Already exists */

  /* Create ~/.config if needed */
  char config_dir[512];
  snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
  mkdir(config_dir, 0755);

  /* Create ~/.config/audiovis */
  if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Failed to create config directory: %s\n", dir);
    return -1;
  }
  return 0;
}

/* Create default config file if it doesn't exist */
static void create_default_config(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0)
    return; /* Already exists */

  FILE *f = fopen(path, "w");
  if (!f)
    return;

  fprintf(f, "[audio]\n");
  fprintf(f, "source = auto\n");
  fprintf(f, "sample_rate = 44100\n");
  fprintf(f, "buffer_size = 2048\n\n");

  fprintf(f, "[visual]\n");
  fprintf(f, "bar_count = 32\n");
  fprintf(f, "bar_char = █\n");
  fprintf(f, "use_colors = 1\n");
  fprintf(f, "gradient_mode = 1\n");
  fprintf(f, "color_low = blue\n");
  fprintf(f, "color_mid = cyan\n");
  fprintf(f, "color_high = magenta\n\n");

  fprintf(f, "[processing]\n");
  fprintf(f, "sensitivity = 1.50\n");
  fprintf(f, "smoothing = 0.70\n");
  fprintf(f, "bass_boost = 1.20\n");
  fprintf(f, "min_freq = 20\n");
  fprintf(f, "max_freq = 20000\n\n");

  fprintf(f, "[performance]\n");
  fprintf(f, "fps = 60\n");
  fprintf(f, "sleep_timer = 1000\n\n");

  fprintf(f, "[layout]\n");
  fprintf(f, "orientation = 0\n");
  fprintf(f, "reverse = 0\n");
  fprintf(f, "bar_width = 2\n");
  fprintf(f, "bar_spacing = 1\n");

  fclose(f);
  printf("Created default config: %s\n", path);
}

int main(int argc, char **argv) {
  int editor_mode = 0;

  /* Parse command line arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-c") == 0) {
      editor_mode = 1;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printf("audiovis - Terminal audio visualizer\n\n");
      printf("Usage: audiovis [OPTIONS]\n\n");
      printf("Options:\n");
      printf("  -c, --config    Open configuration editor\n");
      printf("  -h, --help      Show this help message\n\n");
      printf("Config file: ~/.config/audiovis/config.ini\n");
      printf("Controls: q/ESC to quit\n");
      return 0;
    }
  }

  /* Setup config path */
  char config_path[512];
  if (get_config_path(config_path, sizeof(config_path)) != 0)
    return 1;

  /* Ensure config directory exists and create default config if needed */
  ensure_config_dir();
  create_default_config(config_path);

  /* Load configuration */
  config_t config;
  config_load(config_path, &config);

  /* Launch config editor if requested */
  if (editor_mode) {
    return config_editor_run(&config, config_path);
  }

  /* Initialize subsystems */
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

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  float *audio_buffer = malloc(config.buffer_size * sizeof(float));
  float *magnitudes = malloc(config.bar_count * sizeof(float));

  if (!audio_buffer || !magnitudes) {
    fprintf(stderr, "Failed to allocate buffers\n");
    render_cleanup();
    fft_cleanup(fft);
    audio_cleanup(audio);
    return 1;
  }

  struct timespec frame_time;
  long frame_delay_ns = 1000000000L / config.fps;

  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &frame_time);
    long frame_start_ns = frame_time.tv_sec * 1000000000L + frame_time.tv_nsec;

    int ch = getch();
    if (ch == 'q' || ch == 'Q' || ch == 27)
      break;

    audio_get_buffer(audio, audio_buffer, config.buffer_size);
    fft_process(fft, audio_buffer, magnitudes, config.bar_count);
    render_frame(magnitudes, config.bar_count, &config);

    clock_gettime(CLOCK_MONOTONIC, &frame_time);
    long frame_end_ns = frame_time.tv_sec * 1000000000L + frame_time.tv_nsec;
    long sleep_ns = frame_delay_ns - (frame_end_ns - frame_start_ns);

    if (sleep_ns > 0) {
      struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = sleep_ns};
      nanosleep(&sleep_time, NULL);
    }
  }

  free(magnitudes);
  free(audio_buffer);
  render_cleanup();
  fft_cleanup(fft);
  audio_cleanup(audio);

  return 0;
}
