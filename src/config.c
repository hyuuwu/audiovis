#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Set default configuration values
void config_set_defaults(config_t *config) {
  // Audio defaults
  strcpy(config->audio_source, "auto");
  config->sample_rate = 44100;
  config->buffer_size = 2048;

  // Visual defaults
  config->bar_count = 32;
  strcpy(config->bar_char, "█");
  config->use_colors = 1;
  config->gradient_mode = 1; // Rainbow
  strcpy(config->color_low, "blue");
  strcpy(config->color_mid, "cyan");
  strcpy(config->color_high, "magenta");

  // Processing defaults
  config->sensitivity = 1.5f;
  config->smoothing = 0.7f;
  config->bass_boost = 1.2f;
  config->min_freq = 20;
  config->max_freq = 20000;

  // Performance defaults
  config->fps = 60;
  config->sleep_timer = 1000;

  // Layout defaults
  config->orientation = 0; // Vertical
  config->reverse = 0;
  config->bar_width = 2;
  config->bar_spacing = 1;
}

// Parse a single line from the INI file
static void parse_line(const char *section, const char *key, const char *value,
                       config_t *config) {
  if (strcmp(section, "audio") == 0) {
    if (strcmp(key, "source") == 0) {
      strncpy(config->audio_source, value, sizeof(config->audio_source) - 1);
    } else if (strcmp(key, "sample_rate") == 0) {
      config->sample_rate = atoi(value);
    } else if (strcmp(key, "buffer_size") == 0) {
      config->buffer_size = atoi(value);
    }
  } else if (strcmp(section, "visual") == 0) {
    if (strcmp(key, "bar_count") == 0) {
      config->bar_count = atoi(value);
    } else if (strcmp(key, "bar_char") == 0) {
      strncpy(config->bar_char, value, sizeof(config->bar_char) - 1);
    } else if (strcmp(key, "use_colors") == 0) {
      config->use_colors = parse_bool(value);
    } else if (strcmp(key, "gradient_mode") == 0) {
      config->gradient_mode = atoi(value);
    } else if (strcmp(key, "color_low") == 0) {
      strncpy(config->color_low, value, sizeof(config->color_low) - 1);
    } else if (strcmp(key, "color_mid") == 0) {
      strncpy(config->color_mid, value, sizeof(config->color_mid) - 1);
    } else if (strcmp(key, "color_high") == 0) {
      strncpy(config->color_high, value, sizeof(config->color_high) - 1);
    }
  } else if (strcmp(section, "processing") == 0) {
    if (strcmp(key, "sensitivity") == 0) {
      config->sensitivity = atof(value);
    } else if (strcmp(key, "smoothing") == 0) {
      config->smoothing = clamp(atof(value), 0.0f, 1.0f);
    } else if (strcmp(key, "bass_boost") == 0) {
      config->bass_boost = atof(value);
    } else if (strcmp(key, "min_freq") == 0) {
      config->min_freq = atoi(value);
    } else if (strcmp(key, "max_freq") == 0) {
      config->max_freq = atoi(value);
    }
  } else if (strcmp(section, "performance") == 0) {
    if (strcmp(key, "fps") == 0) {
      config->fps = atoi(value);
    } else if (strcmp(key, "sleep_timer") == 0) {
      config->sleep_timer = atoi(value);
    }
  } else if (strcmp(section, "layout") == 0) {
    if (strcmp(key, "orientation") == 0) {
      config->orientation = atoi(value);
    } else if (strcmp(key, "reverse") == 0) {
      config->reverse = parse_bool(value);
    } else if (strcmp(key, "bar_width") == 0) {
      config->bar_width = atoi(value);
    } else if (strcmp(key, "bar_spacing") == 0) {
      config->bar_spacing = atoi(value);
    }
  }
}

// Load configuration from INI file
int config_load(const char *filename, config_t *config) {
  FILE *file = fopen(filename, "r");

  // Set defaults first
  config_set_defaults(config);

  if (!file) {
    return 0;
  }

  char line[512];
  char section[64] = "";

  while (fgets(line, sizeof(line), file)) {
    // Remove newline
    line[strcspn(line, "\r\n")] = 0;

    // Trim whitespace
    trim_whitespace(line);

    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == ';' || line[0] == '#') {
      continue;
    }

    // Check for section header
    if (line[0] == '[') {
      char *end = strchr(line, ']');
      if (end) {
        *end = '\0';
        strncpy(section, line + 1, sizeof(section) - 1);
        trim_whitespace(section);
      }
      continue;
    }

    // Parse key=value pair
    char *equals = strchr(line, '=');
    if (equals) {
      *equals = '\0';
      char *key = line;
      char *value = equals + 1;

      trim_whitespace(key);
      trim_whitespace(value);

      parse_line(section, key, value, config);
    }
  }

  fclose(file);
  return 1;
}

// Print current configuration (for debugging)
void config_print(const config_t *config) {
  printf("=== Audio Settings ===\n");
  printf("  Source: %s\n", config->audio_source);
  printf("  Sample Rate: %d\n", config->sample_rate);
  printf("  Buffer Size: %d\n", config->buffer_size);

  printf("\n=== Visual Settings ===\n");
  printf("  Bar Count: %d\n", config->bar_count);
  printf("  Bar Char: %s\n", config->bar_char);
  printf("  Use Colors: %d\n", config->use_colors);
  printf("  Gradient Mode: %d\n", config->gradient_mode);
  printf("  Colors: %s / %s / %s\n", config->color_low, config->color_mid,
         config->color_high);

  printf("\n=== Processing Settings ===\n");
  printf("  Sensitivity: %.2f\n", config->sensitivity);
  printf("  Smoothing: %.2f\n", config->smoothing);
  printf("  Bass Boost: %.2f\n", config->bass_boost);
  printf("  Freq Range: %d - %d Hz\n", config->min_freq, config->max_freq);

  printf("\n=== Performance Settings ===\n");
  printf("  FPS: %d\n", config->fps);
  printf("  Sleep Timer: %d ms\n", config->sleep_timer);

  printf("\n=== Layout Settings ===\n");
  printf("  Orientation: %d\n", config->orientation);
  printf("  Reverse: %d\n", config->reverse);
  printf("  Bar Width: %d\n", config->bar_width);
  printf("  Bar Spacing: %d\n", config->bar_spacing);
}
