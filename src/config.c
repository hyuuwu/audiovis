#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Set default configuration values */
void config_set_defaults(config_t *config) {
  /* Audio defaults */
  strncpy(config->audio_source, "auto", sizeof(config->audio_source) - 1);
  config->audio_source[sizeof(config->audio_source) - 1] = '\0';

  config->sample_rate = 44100;
  config->buffer_size = 2048;

  /* Visual defaults */
  config->bar_count = 32;

  strncpy(config->bar_char, "█", sizeof(config->bar_char) - 1);
  config->bar_char[sizeof(config->bar_char) - 1] = '\0';

  config->use_colors = 1;
  config->gradient_mode = 1;

  strncpy(config->color_low, "blue", sizeof(config->color_low) - 1);
  config->color_low[sizeof(config->color_low) - 1] = '\0';

  strncpy(config->color_mid, "cyan", sizeof(config->color_mid) - 1);
  config->color_mid[sizeof(config->color_mid) - 1] = '\0';

  strncpy(config->color_high, "magenta", sizeof(config->color_high) - 1);
  config->color_high[sizeof(config->color_high) - 1] = '\0';

  /* Processing defaults */
  config->sensitivity = 1.5f;
  config->smoothing = 0.7f;
  config->bass_boost = 1.2f;
  config->min_freq = 20;
  config->max_freq = 20000;

  /* Performance defaults */
  config->fps = 60;
  config->sleep_timer = 1000;

  /* Layout defaults */
  config->orientation = 0;
  config->reverse = 0;
  config->bar_width = 2;
  config->bar_spacing = 1;
}

/* Parse a single line from the INI file */
static void parse_line(const char *section, const char *key, const char *value,
                       config_t *config) {

  if (strcmp(section, "audio") == 0) {
    if (strcmp(key, "source") == 0) {
      strncpy(config->audio_source, value, sizeof(config->audio_source) - 1);
      config->audio_source[sizeof(config->audio_source) - 1] = '\0';
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
      config->bar_char[sizeof(config->bar_char) - 1] = '\0';
    } else if (strcmp(key, "use_colors") == 0) {
      config->use_colors = parse_bool(value);
    } else if (strcmp(key, "gradient_mode") == 0) {
      config->gradient_mode = atoi(value);
    } else if (strcmp(key, "color_low") == 0) {
      strncpy(config->color_low, value, sizeof(config->color_low) - 1);
      config->color_low[sizeof(config->color_low) - 1] = '\0';
    } else if (strcmp(key, "color_mid") == 0) {
      strncpy(config->color_mid, value, sizeof(config->color_mid) - 1);
      config->color_mid[sizeof(config->color_mid) - 1] = '\0';
    } else if (strcmp(key, "color_high") == 0) {
      strncpy(config->color_high, value, sizeof(config->color_high) - 1);
      config->color_high[sizeof(config->color_high) - 1] = '\0';
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

/* Load configuration from INI file */
int config_load(const char *filename, config_t *config) {
  FILE *file = fopen(filename, "r");

  config_set_defaults(config);

  if (!file)
    return 0;

  char line[512];
  char section[64] = "";

  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\r\n")] = '\0';
    trim_whitespace(line);

    if (line[0] == '\0' || line[0] == ';' || line[0] == '#')
      continue;

    if (line[0] == '[') {
      char *end = strchr(line, ']');
      if (end) {
        *end = '\0';
        strncpy(section, line + 1, sizeof(section) - 1);
        section[sizeof(section) - 1] = '\0';
        trim_whitespace(section);
      }
      continue;
    }

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

/* Save configuration to INI file */
int config_save(const char *filename, const config_t *config) {
  FILE *file = fopen(filename, "w");
  if (!file)
    return -1;

  fprintf(file, "[audio]\n");
  fprintf(file, "source = %s\n", config->audio_source);
  fprintf(file, "sample_rate = %d\n", config->sample_rate);
  fprintf(file, "buffer_size = %d\n\n", config->buffer_size);

  fprintf(file, "[visual]\n");
  fprintf(file, "bar_count = %d\n", config->bar_count);
  fprintf(file, "bar_char = %s\n", config->bar_char);
  fprintf(file, "use_colors = %d\n", config->use_colors);
  fprintf(file, "gradient_mode = %d\n", config->gradient_mode);
  fprintf(file, "color_low = %s\n", config->color_low);
  fprintf(file, "color_mid = %s\n", config->color_mid);
  fprintf(file, "color_high = %s\n\n", config->color_high);

  fprintf(file, "[processing]\n");
  fprintf(file, "sensitivity = %.2f\n", config->sensitivity);
  fprintf(file, "smoothing = %.2f\n", config->smoothing);
  fprintf(file, "bass_boost = %.2f\n", config->bass_boost);
  fprintf(file, "min_freq = %d\n", config->min_freq);
  fprintf(file, "max_freq = %d\n\n", config->max_freq);

  fprintf(file, "[performance]\n");
  fprintf(file, "fps = %d\n", config->fps);
  fprintf(file, "sleep_timer = %d\n\n", config->sleep_timer);

  fprintf(file, "[layout]\n");
  fprintf(file, "orientation = %d\n", config->orientation);
  fprintf(file, "reverse = %d\n", config->reverse);
  fprintf(file, "bar_width = %d\n", config->bar_width);
  fprintf(file, "bar_spacing = %d\n\n", config->bar_spacing);

  fclose(file);
  return 0;
}
