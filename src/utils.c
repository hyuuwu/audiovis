#include "utils.h"
#include <ctype.h>
#include <math.h>
#include <string.h>

// Trim leading and trailing whitespace from a string
void trim_whitespace(char *str) {
  if (!str)
    return;

  // Trim leading whitespace
  char *start = str;
  while (isspace((unsigned char)*start))
    start++;

  // Trim trailing whitespace
  char *end = start + strlen(start) - 1;
  while (end > start && isspace((unsigned char)*end))
    end--;

  // Write new string
  size_t len = (end - start + 1);
  memmove(str, start, len);
  str[len] = '\0';
}

// Parse boolean from string (1/0, true/false, yes/no, on/off)
int parse_bool(const char *str) {
  if (!str)
    return 0;

  if (strcmp(str, "1") == 0 || strcasecmp(str, "true") == 0 ||
      strcasecmp(str, "yes") == 0 || strcasecmp(str, "on") == 0) {
    return 1;
  }

  return 0;
}

// Clamp a value between min and max
float clamp(float value, float min, float max) {
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}
