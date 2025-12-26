#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
  // Audio settings
  char audio_source[256]; // PipeWire source name (or "auto")
  int sample_rate;        // Audio sample rate
  int buffer_size;        // Audio buffer size

  // Visual settings
  int bar_count;       // Number of frequency bars
  char bar_char[8];    // Character for bars
  int use_colors;      // Enable/disable colors
  int gradient_mode;   // 0=solid, 1=rainbow, 2=custom
  char color_low[16];  // Color for low frequencies
  char color_mid[16];  // Color for mid frequencies
  char color_high[16]; // Color for high frequencies

  // Processing settings
  float sensitivity; // Overall sensitivity multiplier
  float smoothing;   // Temporal smoothing (0.0-1.0)
  float bass_boost;  // Bass frequency boost
  int min_freq;      // Minimum frequency to visualize
  int max_freq;      // Maximum frequency to visualize

  // Performance settings
  int fps;         // Target frames per second
  int sleep_timer; // Sleep when no audio (ms)

  // Layout settings
  int orientation; // 0=vertical, 1=horizontal
  int reverse;     // Reverse bar direction
  int bar_width;   // Width of each bar in chars
  int bar_spacing; // Spacing between bars
} config_t;

// Function prototypes
int config_load(const char *filename, config_t *config);
void config_set_defaults(config_t *config);
void config_print(const config_t *config);

#endif // CONFIG_H
