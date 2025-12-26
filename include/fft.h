#ifndef FFT_H
#define FFT_H

#include "config.h"

// FFT context structure
typedef struct fft_context fft_context_t;

// Function prototypes
fft_context_t *fft_init(int sample_rate, int buffer_size,
                        const config_t *config);
void fft_process(fft_context_t *ctx, const float *audio_buffer,
                 float *magnitudes, int bar_count);
void fft_cleanup(fft_context_t *ctx);

#endif // FFT_H
