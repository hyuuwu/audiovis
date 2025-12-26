#include "audio.h"
#include <pipewire/pipewire.h>
#include <pthread.h>
#include <spa/param/audio/format-utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RING_BUFFER_SIZE 8192

// Audio context structure
struct audio_context {
  struct pw_main_loop *loop;
  struct pw_stream *stream;
  struct pw_thread_loop *thread_loop;

  float ring_buffer[RING_BUFFER_SIZE];
  int write_pos;
  int read_pos;
  pthread_mutex_t mutex;

  int sample_rate;
  int channels;
};

// Callback when audio data is available
static void on_process(void *userdata) {
  audio_context_t *ctx = (audio_context_t *)userdata;
  struct pw_buffer *b;
  struct spa_buffer *buf;
  float *samples;
  uint32_t n_samples;

  if ((b = pw_stream_dequeue_buffer(ctx->stream)) == NULL) {
    return;
  }

  buf = b->buffer;
  if (buf->datas[0].data == NULL) {
    goto done;
  }

  samples = (float *)buf->datas[0].data;
  n_samples = buf->datas[0].chunk->size / sizeof(float);

  // Write to ring buffer
  pthread_mutex_lock(&ctx->mutex);
  for (uint32_t i = 0; i < n_samples; i += ctx->channels) {
    // Mix all channels to mono
    float sample = 0.0f;
    for (int ch = 0; ch < ctx->channels && ch < 2; ch++) {
      sample += samples[i + ch];
    }
    sample /= ctx->channels;

    ctx->ring_buffer[ctx->write_pos] = sample;
    ctx->write_pos = (ctx->write_pos + 1) % RING_BUFFER_SIZE;
  }
  pthread_mutex_unlock(&ctx->mutex);

done:
  pw_stream_queue_buffer(ctx->stream, b);
}

// Stream events
static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

// Initialize PipeWire audio capture
audio_context_t *audio_init(const config_t *config) {
  audio_context_t *ctx = calloc(1, sizeof(audio_context_t));
  if (!ctx) {
    fprintf(stderr, "Failed to allocate audio context\n");
    return NULL;
  }

  ctx->sample_rate = config->sample_rate;
  ctx->channels = 2; // Stereo
  ctx->write_pos = 0;
  ctx->read_pos = 0;
  pthread_mutex_init(&ctx->mutex, NULL);

  // Initialize PipeWire
  pw_init(NULL, NULL);

  ctx->thread_loop = pw_thread_loop_new("audiovis", NULL);
  if (!ctx->thread_loop) {
    fprintf(stderr, "Failed to create PipeWire thread loop\n");
    free(ctx);
    return NULL;
  }

  struct pw_loop *loop = pw_thread_loop_get_loop(ctx->thread_loop);

  // Create stream
  ctx->stream = pw_stream_new_simple(
      loop, "audiovis-capture",
      pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                        "Capture", PW_KEY_MEDIA_ROLE, "Music", NULL),
      &stream_events, ctx);

  if (!ctx->stream) {
    fprintf(stderr, "Failed to create PipeWire stream\n");
    pw_thread_loop_destroy(ctx->thread_loop);
    free(ctx);
    return NULL;
  }

  // Audio format parameters
  uint8_t buffer[1024];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

  const struct spa_pod *params[1];
  params[0] = spa_format_audio_raw_build(
      &b, SPA_PARAM_EnumFormat,
      &SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32,
                               .channels = ctx->channels,
                               .rate = ctx->sample_rate));

  // Connect stream
  pw_thread_loop_lock(ctx->thread_loop);

  int ret = pw_stream_connect(ctx->stream, PW_DIRECTION_INPUT, PW_ID_ANY,
                              PW_STREAM_FLAG_AUTOCONNECT |
                                  PW_STREAM_FLAG_MAP_BUFFERS |
                                  PW_STREAM_FLAG_RT_PROCESS,
                              params, 1);

  if (ret < 0) {
    fprintf(stderr, "Failed to connect stream: error code %d\n", ret);
    pw_thread_loop_unlock(ctx->thread_loop);
    pw_stream_destroy(ctx->stream);
    pw_thread_loop_destroy(ctx->thread_loop);
    free(ctx);
    return NULL;
  }

  pw_thread_loop_unlock(ctx->thread_loop);

  // Start the thread loop
  pw_thread_loop_start(ctx->thread_loop);

  return ctx;
}

// Get audio buffer for processing
int audio_get_buffer(audio_context_t *ctx, float *buffer, int size) {
  if (!ctx)
    return 0;

  pthread_mutex_lock(&ctx->mutex);

  int available =
      (ctx->write_pos - ctx->read_pos + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
  int to_read = (size < available) ? size : available;

  for (int i = 0; i < to_read; i++) {
    buffer[i] = ctx->ring_buffer[ctx->read_pos];
    ctx->read_pos = (ctx->read_pos + 1) % RING_BUFFER_SIZE;
  }

  // Fill rest with zeros if not enough data
  for (int i = to_read; i < size; i++) {
    buffer[i] = 0.0f;
  }

  pthread_mutex_unlock(&ctx->mutex);

  return to_read;
}

// Cleanup audio capture
void audio_cleanup(audio_context_t *ctx) {
  if (!ctx)
    return;

  if (ctx->thread_loop) {
    pw_thread_loop_stop(ctx->thread_loop);
  }

  if (ctx->stream) {
    pw_stream_destroy(ctx->stream);
  }

  if (ctx->thread_loop) {
    pw_thread_loop_destroy(ctx->thread_loop);
  }

  pthread_mutex_destroy(&ctx->mutex);
  pw_deinit();

  free(ctx);
}
