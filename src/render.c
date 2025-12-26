#include "render.h"
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Color pairs
#define COLOR_PAIR_LOW 1
#define COLOR_PAIR_MID 2
#define COLOR_PAIR_HIGH 3

static int screen_height;
static int screen_width;

// Map color name to ncurses color
static int get_color_code(const char *color_name) {
  if (strcasecmp(color_name, "red") == 0)
    return COLOR_RED;
  if (strcasecmp(color_name, "green") == 0)
    return COLOR_GREEN;
  if (strcasecmp(color_name, "yellow") == 0)
    return COLOR_YELLOW;
  if (strcasecmp(color_name, "blue") == 0)
    return COLOR_BLUE;
  if (strcasecmp(color_name, "magenta") == 0)
    return COLOR_MAGENTA;
  if (strcasecmp(color_name, "cyan") == 0)
    return COLOR_CYAN;
  if (strcasecmp(color_name, "white") == 0)
    return COLOR_WHITE;
  return COLOR_WHITE;
}

// Initialize ncurses rendering
int render_init(const config_t *config) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  timeout(0); // Non-blocking input

  // Initialize colors if requested
  if (config->use_colors && has_colors()) {
    start_color();
    use_default_colors();

    // Define color pairs
    init_pair(COLOR_PAIR_LOW, get_color_code(config->color_low), -1);
    init_pair(COLOR_PAIR_MID, get_color_code(config->color_mid), -1);
    init_pair(COLOR_PAIR_HIGH, get_color_code(config->color_high), -1);
  }

  getmaxyx(stdscr, screen_height, screen_width);

  return 1;
}

// Get color pair based on height and gradient mode
static int get_color_for_height(float height, int gradient_mode) {
  if (gradient_mode == 0) {
    // Solid color
    return COLOR_PAIR_MID;
  } else if (gradient_mode == 1) {
    // Rainbow gradient based on height
    if (height < 0.33f)
      return COLOR_PAIR_LOW;
    if (height < 0.66f)
      return COLOR_PAIR_MID;
    return COLOR_PAIR_HIGH;
  } else {
    // Custom gradient
    if (height < 0.5f)
      return COLOR_PAIR_LOW;
    return COLOR_PAIR_HIGH;
  }
}

// Render a single frame
void render_frame(const float *magnitudes, int bar_count,
                  const config_t *config) {
  // Get current screen size (handle resize)
  getmaxyx(stdscr, screen_height, screen_width);

  // Clear screen
  erase();

  // Calculate bar dimensions
  int total_bar_width = config->bar_width + config->bar_spacing;
  int available_width = screen_width;
  int bars_to_draw = bar_count;

  // Adjust bar count if it doesn't fit
  if (total_bar_width * bar_count > available_width) {
    bars_to_draw = available_width / total_bar_width;
  }

  int start_x = (screen_width - (bars_to_draw * total_bar_width)) / 2;
  if (start_x < 0)
    start_x = 0;

  // Draw bars
  for (int i = 0; i < bars_to_draw; i++) {
    float magnitude = magnitudes[i];

    // Calculate bar height
    int bar_height = (int)(magnitude * (screen_height - 2));
    if (bar_height > screen_height - 1)
      bar_height = screen_height - 1;

    // Calculate bar position
    int x = start_x + (i * total_bar_width);

    if (config->orientation == 0) {
      // Vertical bars
      int start_y = config->reverse ? 0 : (screen_height - 1 - bar_height);
      int end_y = config->reverse ? bar_height : (screen_height - 1);

      for (int y = start_y; y <= end_y && y < screen_height; y++) {
        float height_ratio =
            (float)(y - start_y) / (bar_height > 0 ? bar_height : 1);

        if (config->use_colors) {
          int color = get_color_for_height(height_ratio, config->gradient_mode);
          attron(COLOR_PAIR(color));
        }

        // Draw bar width
        for (int w = 0; w < config->bar_width && (x + w) < screen_width; w++) {
          mvaddstr(y, x + w, config->bar_char);
        }

        if (config->use_colors) {
          attroff(COLOR_PAIR(
              get_color_for_height(height_ratio, config->gradient_mode)));
        }
      }
    } else {
      // Horizontal bars
      int bar_y = i * 2; // Spacing for horizontal mode
      if (bar_y >= screen_height)
        break;

      int start_x_bar = config->reverse ? (screen_width - 1 - bar_height) : 0;
      int end_x_bar = config->reverse ? (screen_width - 1) : bar_height;

      for (int x_bar = start_x_bar; x_bar <= end_x_bar && x_bar < screen_width;
           x_bar++) {
        float width_ratio =
            (float)(x_bar - start_x_bar) / (bar_height > 0 ? bar_height : 1);

        if (config->use_colors) {
          int color = get_color_for_height(width_ratio, config->gradient_mode);
          attron(COLOR_PAIR(color));
        }

        mvaddstr(bar_y, x_bar, config->bar_char);

        if (config->use_colors) {
          attroff(COLOR_PAIR(
              get_color_for_height(width_ratio, config->gradient_mode)));
        }
      }
    }
  }

  // Display controls hint
  attron(A_DIM);
  mvprintw(screen_height - 1, 0, "Press 'q' to quit");
  attroff(A_DIM);

  // Refresh screen
  refresh();
}

// Cleanup ncurses
void render_cleanup(void) { endwin(); }
