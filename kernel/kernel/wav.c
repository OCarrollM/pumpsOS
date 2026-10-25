#include "wav.h"
#include "vfs.h"
#include "task.h"
#include "../arch/i386/ac97.h"
#include <stdio.h>
#include <string.h>

// RIFF parsing
typedef struct {
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
}__attribute__((packed)) wav_fmt_t;

// PCM ring
#define RING_FRAMES 48000
#define RING_SAMPLES (RING_FRAMES * AC97_CHANNELS)

static int16_t ring[RING_SAMPLES];
static volatile uint32_t ring_head = 0;
static volatile uint32_t ring_tail = 0;
static vfs_node_t* src_node = 0;
static const uint8_t* src_mem = 0;
static uint32_t src_mem_len = 0;

static uint32_t wav_data_offset = 0;
static uint32_t wav_data_size = 0;
static uint32_t wav_read_pos = 0;
static uint16_t wav_channels = 2;
static volatile bool wav_playing = false;
static volatile bool wav_file_done = false;

static uint32_t silence_frames = 0;

bool wav_is_playing(void) { return wav_playing; }

static uint32_t src_read(uint32_t offset, uint32_t len, uint8_t* out) {
    if (src_mem) {
        if (offset >= src_mem_len) return 0;
        uint32_t avail = src_mem_len - offset;
        if (len > avail) len = avail;
        memcpy(out, src_mem + offset, len);
        return len;
    }
    if (src_node) {
        return vfs_read(src_node, offset, len, out);
    }
    return 0;
}

static uint32_t ring_available(void) {
    return (ring_tail - ring_head + RING_SAMPLES) % RING_SAMPLES;
}

static uint32_t ring_space(void) {
    return RING_SAMPLES - ring_available() - 1;
}

// Reworking wav_fill to stop jittering after one second.
// This version will leave slots empty on purpose to wait for the
// next poll
static uint32_t wav_fill(int16_t* buf, uint32_t frames) {
    uint32_t want = frames * AC97_CHANNELS;
    uint32_t have = ring_available();

    if (have < want && !wav_file_done) return 0;

    if (have == 0) return 0;

    uint32_t take = have < want ? have : want;
    for (uint32_t i = 0; i < take; i++) {
        buf[i] = ring[ring_head];
        ring_head = (ring_head + 1) % RING_SAMPLES;
    }
    return take / AC97_CHANNELS;
}

static void wav_read_chunk(void) {
    uint32_t space = ring_space();
    if (space < 1024) return;

    uint32_t remaining = wav_data_size - wav_read_pos;
    if (remaining == 0) {
        wav_file_done = true;
        return;
    }

    static uint8_t chunk[4096];
    uint32_t to_read = sizeof(chunk);
    if (to_read > remaining) to_read = remaining;

    uint32_t max_bytes = (wav_channels == 1) ? (space / 2) * 2 : space * 2;
    if (to_read > max_bytes) to_read = max_bytes;
    if (to_read == 0) return;

    uint32_t got = src_read(wav_data_offset + wav_read_pos, to_read, chunk);
    if (got == 0) { wav_file_done = true; return; }
    wav_read_pos += got;

    const int16_t* samples = (const int16_t*)chunk;
    uint32_t count = got / sizeof(int16_t);

    for (uint32_t i = 0; i < count; i++) {
        ring[ring_tail] = samples[i];
        ring_tail = (ring_tail + 1) % RING_SAMPLES;
        if (wav_channels == 1) {
            ring[ring_tail] = samples[i];
            ring_tail = (ring_tail + 1) % RING_SAMPLES;
        }
    }
}

static void wav_reader(void* arg) {
    (void)arg;
    while (wav_playing && !wav_file_done) {
        while (!wav_file_done && ring_space() >= 1024) {
            wav_read_chunk();
        }
        task_yield();
    }
    printf("WAV reader finished\n");
}

static bool wav_parse(void) {
    uint8_t header[12];
    if (src_read(0, sizeof(header), header) != sizeof(header)) {
        printf("WAV cannot read header\n");
        return false;
    }
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        printf("WAV not a free RIFF/WAVE file\n");
        return false;
    }

    bool have_fmt = false;
    bool have_data = false;
    wav_fmt_t fmt;
    uint32_t pos = 12;

    for (int i = 0; i < 32 && !(have_fmt && have_data); i++) {
        uint8_t ch[8];
        if (src_read(pos, sizeof(ch), ch) != sizeof(ch)) {
            break;
        }
        uint32_t size = (uint32_t)ch[4]
                      | ((uint32_t)ch[5] << 8)
                      | ((uint32_t)ch[6] << 16)
                      | ((uint32_t)ch[7] << 24);
        
        if (memcmp(ch, "fmt ", 4) == 0) {
            if (src_read(pos + 8, sizeof(fmt), (uint8_t*)&fmt) != sizeof(fmt)) {
                break;
            }
            have_fmt = true;
        } else if (memcmp(ch, "data", 4) == 0) {
            wav_data_offset = pos + 8;
            wav_data_size = size;
            have_data = true;
        }

        pos += 8 + size + (size % 1);
    }

    if (!have_fmt || !have_data) {
        printf("WAV missing fmt or data chunk\n");
        return false;
    }

    printf("[WAV] %d Hz, %d ch, %d bit, %d bytes of audio\n",
           fmt.sample_rate, fmt.channels, fmt.bits_per_sample, wav_data_size);
 
    if (fmt.audio_format != 1) {
        printf("[WAV] not uncompressed PCM\n");
        return false;
    }
    if (fmt.bits_per_sample != 16) {
        printf("[WAV] need 16-bit samples\n");
        return false;
    }
    if (fmt.sample_rate != AC97_SAMPLE_RATE) {
        printf("[WAV] need %d Hz (no resampling)\n", AC97_SAMPLE_RATE);
        return false;
    }
    if (fmt.channels != 1 && fmt.channels != 2) {
        printf("[WAV] need mono or stereo\n");
        return false;
    }

    wav_channels = fmt.channels;
    printf("WAV array %d bytes, data chunk claims %d\n", src_mem_len, wav_data_size);
    return true;
}

bool wav_start(void) {
    wav_read_pos = 0;
    ring_head = 0;
    ring_tail = 0;
    wav_file_done = false;

    if (!wav_parse()) return false;

    wav_playing = true;

    while (!wav_file_done && ring_space() > 1024) {
        wav_read_chunk();
    }

    task_t* t = thread_create("wav-reader", wav_reader, 0, PRIORITY_NORMAL);
    printf("[WAV] thread_create returned %x\n", (uint32_t)t);

    printf("WAV STARTING PLAYBACK\n");
    if (!ac97_play(wav_fill)) {
        printf("WAV could not start playback\n");
        wav_playing = false;
        return false;
    }
    printf("WAV PLAYBACK STARTED\n");
    return true;
}

bool wav_play(const char* path) {
    if (wav_playing) { printf("[WAV] already playing\n"); return false; }
 
    vfs_node_t* node = vfs_lookup(path);
    if (!node) { printf("[WAV] not found: %s\n", path); return false; }
 
    src_node = node;
    src_mem  = 0;
 
    if (!wav_start()) return false;
    printf("[WAV] playing %s\n", path);
    return true;
}

bool wav_play_memory(const uint8_t* data, uint32_t len) {
    if (wav_playing) { printf("[WAV] already playing\n"); return false; }
 
    src_mem = data;
    src_mem_len = len;
    src_node = 0;
 
    if (!wav_start()) return false;
    printf("[WAV] playing from memory (%d bytes)\n", len);
    return true;
}

void wav_stop(void) {
    wav_playing = false;
    ac97_stop();
    src_node = 0;
    src_mem = 0;
}