#include "julia_audio.h"

#include <cstring>
#include <iostream>

struct jlca_handle {
    coreaudio::julia_audio impl;

    jlca_handle(size_t capacity_frames) : impl(capacity_frames) { }
};

static OSStatus jlca_render(
    void *in_ref,
    AudioUnitRenderActionFlags *io_flags,
    const AudioTimeStamp *in_timestamp,
    UInt32 in_bus_number,
    UInt32 in_frames_count,
    AudioBufferList *io_data
) {
    (void) io_flags;
    (void) in_timestamp;
    (void) in_bus_number;

    auto *h = (jlca_handle *) in_ref;
    if (!h) return noErr;

    auto &q = h->impl.q;

    // mono, non-interleaved float, same as your current setup
    Float32 *buffer = (Float32 *) io_data->mBuffers[0].mData;
    UInt32 frame = 0;

    while (frame < in_frames_count) {
        auto ready = q.contiguous_read();
        if (ready == 0) {
            break;
        }

        auto take = (uint64_t) (in_frames_count - frame);
        if (take > ready) take = ready;

        float *src = q.read_ptr();
        std::memcpy(buffer + frame, src, take * sizeof(float));

        q.consume(take);
        frame += (UInt32) take;
    }

    // underrun -> silence
    for (; frame < in_frames_count; ++frame) {
        buffer[frame] = 0.0f;
    }

    return noErr;
}

extern "C"
auto jlca_create(int sample_rate, int capacity_frames) -> jlca_handle * {
    auto *h = new jlca_handle(capacity_frames);

    auto &d = h->impl.d;
    d.channels = 1;
    d.asbd_flags = static_cast<AudioFormatFlags>(
        kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved
    );
    d.sample_rate = sample_rate;
    d.sample_bits = 32;
    d.sample_bytes = sizeof(float);
    d.aurcs.inputProc = jlca_render;
    d.aurcs.inputProcRefCon = h;

    return h;
}

extern "C"
auto jlca_destroy(jlca_handle *h) -> void {
    if (!h) return;
    h->impl.audio.reset();
    delete h;
}

extern "C"
auto jlca_start(jlca_handle *h) -> int {
    if (!h) return -1;

    auto audio = coreaudio::instance::make(h->impl.d);
    if (audio == std::nullopt) {
        std::fprintf(stderr, "Error: jlca_start().\n");
        return -1;
    }

    h->impl.audio.emplace(std::move(*audio));
    return 0;
}

extern "C"
auto jlca_stop(jlca_handle *h) -> void {
    if (!h) return;
    h->impl.audio.reset();
}

extern "C"
auto jlca_set_volume(jlca_handle *h, int percent) -> int {
    if (!h) return -1;
    if (h->impl.audio == std::nullopt) return percent;
    return h->impl.audio->volume(percent);
}

extern "C"
auto jlca_available_write(jlca_handle *h) -> int {
    if (!h) return 0;
    return (int) h->impl.q.available_write();
}

extern "C"
auto jlca_begin_write(jlca_handle *h) -> float * {
    if (!h) return nullptr;
    if (h->impl.q.contiguous_write() == 0) return nullptr;
    return h->impl.q.write_ptr();
}

extern "C"
auto jlca_begin_write_frames(jlca_handle *h) -> int {
    if (!h) return 0;
    return (int) h->impl.q.contiguous_write();
}

extern "C"
auto jlca_end_write(jlca_handle *h, int frames) -> void {
    if (!h) return;
    if (frames <= 0) return;

    auto free = h->impl.q.available_write();
    auto take = (uint64_t) frames;
    if (take > free) take = free;

    h->impl.q.produce(take);
}

extern "C"
auto jlca_available_read(jlca_handle *h) -> int {
    if (!h) return 0;
    return (int) h->impl.q.available_read();
}