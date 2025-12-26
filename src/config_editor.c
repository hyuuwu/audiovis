#include "config_editor.h"
#include "config.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple input using bounded ncurses input */
static int get_input_simple(char *result, int max_len, const char *prompt) {
  int height, width;
  getmaxyx(stdscr, height, width);

  flushinp();

  for (int i = height - 3; i < height; i++) {
    move(i, 0);
    clrtoeol();
  }

  attron(COLOR_PAIR(3) | A_BOLD);
  mvprintw(height - 3, 1, "%s", prompt);
  attroff(COLOR_PAIR(3) | A_BOLD);
  mvprintw(height - 2, 1, "Type new value and press Enter:");
  mvprintw(height - 1, 1, "> ");
  refresh();

  keypad(stdscr, FALSE);
  echo();
  curs_set(1);

  char buf[256] = {0};
  int ret = 0;

  /* SAFE bounded input */
  if (wgetnstr(stdscr, buf, sizeof(buf) - 1) != ERR && buf[0] != '\0') {
    strncpy(result, buf, max_len - 1);
    result[max_len - 1] = '\0'; /* FIXED */
    ret = 1;
  }

  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  return ret;
}

int config_editor_run(config_t *config, const char *config_file) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
  }

  int current = 0;
  int running = 1;
  int modified = 0;

  struct {
    const char *name;
    int type; /* 0=int, 1=float, 2=string, 3=bool */
    void *ptr;
    float min_f, max_f;
    int min_i, max_i;
    int max_str_len;
  } fields[] = {
      {"Audio Source", 2, config->audio_source, 0, 0, 0, 0, 255},
      {"Sample Rate", 0, &config->sample_rate, 0, 0, 8000, 192000, 0},
      {"Buffer Size", 0, &config->buffer_size, 0, 0, 256, 8192, 0},
      {"Bar Count", 0, &config->bar_count, 0, 0, 8, 256, 0},
      {"Bar Character", 2, config->bar_char, 0, 0, 0, 0, 7},
      {"Use Colors (0/1)", 3, &config->use_colors, 0, 0, 0, 0, 0},
      {"Gradient Mode", 0, &config->gradient_mode, 0, 0, 0, 2, 0},
      {"Color Low", 2, config->color_low, 0, 0, 0, 0, 15},
      {"Color Mid", 2, config->color_mid, 0, 0, 0, 0, 15},
      {"Color High", 2, config->color_high, 0, 0, 0, 0, 15},
      {"Sensitivity", 1, &config->sensitivity, 0.1f, 10.0f, 0, 0, 0},
      {"Smoothing", 1, &config->smoothing, 0.0f, 1.0f, 0, 0, 0},
      {"Bass Boost", 1, &config->bass_boost, 0.5f, 5.0f, 0, 0, 0},
      {"Min Frequency", 0, &config->min_freq, 0, 0, 20, 20000, 0},
      {"Max Frequency", 0, &config->max_freq, 0, 0, 20, 20000, 0},
      {"FPS", 0, &config->fps, 0, 0, 1, 120, 0},
      {"Sleep Timer (ms)", 0, &config->sleep_timer, 0, 0, 0, 10000, 0},
      {"Orientation (0/1)", 0, &config->orientation, 0, 0, 0, 1, 0},
      {"Reverse (0/1)", 3, &config->reverse, 0, 0, 0, 0, 0},
      {"Bar Width", 0, &config->bar_width, 0, 0, 1, 10, 0},
      {"Bar Spacing", 0, &config->bar_spacing, 0, 0, 0, 10, 0},
  };

  int num_fields = sizeof(fields) / sizeof(fields[0]);

  while (running) {
    clear();
    int height, width;
    getmaxyx(stdscr, height, width);

    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, (width - 30) / 2, "AudioVis Configuration Editor");
    attroff(COLOR_PAIR(1) | A_BOLD);

    int y = 2;
    for (int i = 0; i < num_fields && y < height - 6; i++, y++) {
      if (i == current) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(y, 2, "> ");
      } else {
        attron(COLOR_PAIR(4));
        mvprintw(y, 2, "  ");
      }

      mvprintw(y, 4, "%-22s: ", fields[i].name);

      switch (fields[i].type) {
      case 0:
        printw("%d", *(int *)fields[i].ptr);
        break;
      case 1:
        printw("%.2f", *(float *)fields[i].ptr);
        break;
      case 2:
        printw("%s", (char *)fields[i].ptr);
        break;
      case 3:
        printw("%d", *(int *)fields[i].ptr);
        break;
      }

      attroff(COLOR_PAIR(3) | A_BOLD);
      attroff(COLOR_PAIR(4));
    }

    refresh();
    int ch = getch();

    switch (ch) {
    case KEY_UP:
      if (current > 0)
        current--;
      break;
    case KEY_DOWN:
      if (current < num_fields - 1)
        current++;
      break;

    case 10:
    case KEY_ENTER: {
      char prompt[128];
      snprintf(prompt, sizeof(prompt), "Edit %s", fields[current].name);

      char result[256];
      if (get_input_simple(result, sizeof(result), prompt)) {
        switch (fields[current].type) {
        case 0: {
          int v = atoi(result);
          if (v < fields[current].min_i)
            v = fields[current].min_i;
          if (v > fields[current].max_i)
            v = fields[current].max_i;
          *(int *)fields[current].ptr = v;
          break;
        }
        case 1: {
          float v = atof(result);
          if (v < fields[current].min_f)
            v = fields[current].min_f;
          if (v > fields[current].max_f)
            v = fields[current].max_f;
          *(float *)fields[current].ptr = v;
          break;
        }
        case 2: {
          int slen = fields[current].max_str_len;
          strncpy((char *)fields[current].ptr, result, slen - 1);
          ((char *)fields[current].ptr)[slen - 1] = '\0'; /* FIXED */
          break;
        }
        case 3:
          *(int *)fields[current].ptr = atoi(result) ? 1 : 0;
          break;
        }
        modified = 1;
      }
      break;
    }

    case 'q':
    case 'Q':
      running = 0;
      break;
    }
  }

  endwin();
  return 0;
}
