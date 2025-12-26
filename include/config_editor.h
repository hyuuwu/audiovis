#ifndef CONFIG_EDITOR_H
#define CONFIG_EDITOR_H

#include "config.h"

// Field types for the editor
typedef enum {
  FIELD_INT,
  FIELD_FLOAT,
  FIELD_STRING,
  FIELD_BOOL,
  FIELD_COLOR,
  FIELD_ENUM
} field_type_t;

// Field metadata for editing
typedef struct {
  const char *name;          // Display name
  const char *description;   // Help text
  field_type_t type;         // Field type
  void *value_ptr;           // Pointer to value in config_t
  int min_int;               // Min value for int fields
  int max_int;               // Max value for int fields
  float min_float;           // Min value for float fields
  float max_float;           // Max value for float fields
  int max_len;               // Max string length
  const char **enum_options; // Enum options (NULL-terminated)
} field_info_t;

// Section information
typedef struct {
  const char *name;
  field_info_t *fields;
  int field_count;
} section_info_t;

// Main config editor function
int config_editor_run(config_t *config, const char *config_file);

#endif // CONFIG_EDITOR_H
