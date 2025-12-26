#ifndef RENDER_H
#define RENDER_H

#include "config.h"

// Function prototypes
int render_init(const config_t *config);
void render_frame(const float *magnitudes, int bar_count,
                  const config_t *config);
void render_cleanup(void);

#endif // RENDER_H
