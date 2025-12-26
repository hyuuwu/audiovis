#include "fft.h"
#include "utils.h"
#include <fftw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FFT context structure
struct fft_context {
  int sample_rate;
  int buffer_size;

  // FFTW3 structures
  fftwf_plan plan;
  float *input;
  fftwf_complex *output;

  // Configuration
  float sensitivity;
  float smoothing;
  float bass_boost;
  int min_freq;
  int max_freq;

  // Smoothing buffers
  float *prev_magnitudes;
  int num_bars;
};

// Initialize FFT processing
fft_context_t *fft_init(int sample_rate, int buffer_size,
                        const config_t *config) {
  fft_context_t *ctx = calloc(1, sizeof(fft_context_t));
  if (!ctx) {
    fprintf(stderr, "Failed to allocate FFT context\n");
    return NULL;
  }

  ctx->sample_rate = sample_rate;
  ctx->buffer_size = buffer_size;
  ctx->sensitivity = config->sensitivity;
  ctx->smoothing = config->smoothing;
  ctx->bass_boost = config->bass_boost;
  ctx->min_freq = config->min_freq;
  ctx->max_freq = config->max_freq;
  ctx->num_bars = config->bar_count;

  // Allocate FFTW buffers
  ctx->input = fftwf_malloc(sizeof(float) * buffer_size);
  ctx->output = fftwf_malloc(sizeof(fftwf_complex) * (buffer_size / 2 + 1));

  if (!ctx->input || !ctx->output) {
    fprintf(stderr, "Failed to allocate FFT buffers\n");
    if (ctx->input)
      fftwf_free(ctx->input);
    if (ctx->output)
      fftwf_free(ctx->output);
    free(ctx);
    return NULL;
  }

  // Create FFT plan
  ctx->plan =
      fftwf_plan_dft_r2c_1d(buffer_size, ctx->input, ctx->output, FFTW_MEASURE);
  if (!ctx->plan) {
    fprintf(stderr, "Failed to create FFT plan\n");
    fftwf_free(ctx->input);
    fftwf_free(ctx->output);
    free(ctx);
    return NULL;
  }

  // Allocate smoothing buffer
  ctx->prev_magnitudes = calloc(config->bar_count, sizeof(float));

  return ctx;
}

// Process audio buffer and generate frequency magnitudes
void fft_process(fft_context_t *ctx, const float *audio_buffer,
                 float *magnitudes, int bar_count) {
  if (!ctx || !audio_buffer || !magnitudes)
    return;

  // Apply Hann window and copy to FFT input
  for (int i = 0; i < ctx->buffer_size; i++) {
    float window =
        0.5f * (1.0f - cosf(2.0f * M_PI * i / (ctx->buffer_size - 1)));
    ctx->input[i] = audio_buffer[i] * window;
  }

  // Execute FFT
  fftwf_execute(ctx->plan);

  // Calculate frequency bin size
  float freq_per_bin = (float)ctx->sample_rate / ctx->buffer_size;
  int num_bins = ctx->buffer_size / 2 + 1;

  // Find frequency bins for range
  int min_bin = (int)(ctx->min_freq / freq_per_bin);
  int max_bin = (int)(ctx->max_freq / freq_per_bin);
  if (max_bin >= num_bins)
    max_bin = num_bins - 1;

  // Logarithmic frequency binning
  float log_min = logf(fmaxf(1.0f, (float)ctx->min_freq));
  float log_max = logf((float)ctx->max_freq);
  float log_range = log_max - log_min;

  for (int bar = 0; bar < bar_count; bar++) {
    // Calculate frequency range for this bar (logarithmic)
    float bar_log_min = log_min + (log_range * bar) / bar_count;
    float bar_log_max = log_min + (log_range * (bar + 1)) / bar_count;

    int start_bin = (int)(expf(bar_log_min) / freq_per_bin);
    int end_bin = (int)(expf(bar_log_max) / freq_per_bin);

    if (start_bin < min_bin)
      start_bin = min_bin;
    if (end_bin > max_bin)
      end_bin = max_bin;
    if (start_bin >= end_bin)
      start_bin = end_bin - 1;

    // Average magnitude in this frequency range
    float magnitude = 0.0f;
    int bin_count = 0;

    for (int bin = start_bin; bin <= end_bin; bin++) {
      float real = ctx->output[bin][0];
      float imag = ctx->output[bin][1];
      float mag = sqrtf(real * real + imag * imag);

      // Apply bass boost for lower frequencies
      if (bin < num_bins * 0.1f) {
        mag *= ctx->bass_boost;
      }

      magnitude += mag;
      bin_count++;
    }

    if (bin_count > 0) {
      magnitude /= bin_count;
    }

    // Apply sensitivity
    magnitude *= ctx->sensitivity;

    // Normalize (scale to 0-1 range, with some headroom)
    magnitude = sqrtf(magnitude) / 100.0f;
    magnitude = clamp(magnitude, 0.0f, 1.0f);

    // Apply temporal smoothing
    if (ctx->prev_magnitudes) {
      magnitude = ctx->prev_magnitudes[bar] * ctx->smoothing +
                  magnitude * (1.0f - ctx->smoothing);
      ctx->prev_magnitudes[bar] = magnitude;
    }

    magnitudes[bar] = magnitude;
  }
}

// Cleanup FFT processing
void fft_cleanup(fft_context_t *ctx) {
  if (!ctx)
    return;

  if (ctx->plan) {
    fftwf_destroy_plan(ctx->plan);
  }

  if (ctx->input) {
    fftwf_free(ctx->input);
  }

  if (ctx->output) {
    fftwf_free(ctx->output);
  }

  if (ctx->prev_magnitudes) {
    free(ctx->prev_magnitudes);
  }

  free(ctx);
}
