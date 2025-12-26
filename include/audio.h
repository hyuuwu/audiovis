#ifndef AUDIO_H
#define AUDIO_H

#include "config.h"

// Audio context structure
typedef struct audio_context audio_context_t;

// Function prototypes
audio_context_t *audio_init(const config_t *config);
int audio_get_buffer(audio_context_t *ctx, float *buffer, int size);
void audio_cleanup(audio_context_t *ctx);

#endif // AUDIO_H
