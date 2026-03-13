#pragma once

#include "coreaudio.h"

#include <vector>
#include <optional>
#include <atomic>
#include <cstdint>
#include <cstdio>

namespace coreaudio {

struct queue {
    std::vector<float> samples;
    std::atomic<uint64_t> read_index { 0 };
    std::atomic<uint64_t> write_index { 0 };

    queue(void) { }
    queue(size_t n) : samples(n, 0.0f) { }

    auto capacity(void) const -> uint64_t {
        return samples.size();
    }

    auto available_read(void) const -> uint64_t {
        return write_index.load(std::memory_order_acquire)
             - read_index.load(std::memory_order_acquire);
    }

    auto available_write(void) const -> uint64_t {
        return capacity() - available_read();
    }

    auto contiguous_write(void) const -> uint64_t {
        auto w = write_index.load(std::memory_order_acquire);
        auto r = read_index.load(std::memory_order_acquire);
        auto cap = capacity();
        auto used = w - r;
        auto free = cap - used;
        auto off = w % cap;
        auto until_wrap = cap - off;
        return (free < until_wrap) ? free : until_wrap;
    }

    auto contiguous_read(void) const -> uint64_t {
        auto w = write_index.load(std::memory_order_acquire);
        auto r = read_index.load(std::memory_order_acquire);
        auto ready = w - r;
        auto cap = capacity();
        auto off = r % cap;
        auto until_wrap = cap - off;
        return (ready < until_wrap) ? ready : until_wrap;
    }

    auto write_ptr(void) -> float * {
        auto cap = capacity();
        if (cap == 0) return nullptr;
        auto w = write_index.load(std::memory_order_acquire);
        return samples.data() + (w % cap);
    }

    auto read_ptr(void) -> float * {
        auto cap = capacity();
        if (cap == 0) return nullptr;
        auto r = read_index.load(std::memory_order_acquire);
        return samples.data() + (r % cap);
    }

    auto produce(uint64_t n) -> void {
        write_index.fetch_add(n, std::memory_order_release);
    }

    auto consume(uint64_t n) -> void {
        read_index.fetch_add(n, std::memory_order_release);
    }
};

struct julia_audio {
    queue q;
    definition d;
    std::optional<instance> audio;

    julia_audio(void) { }
    julia_audio(size_t capacity_frames) : q(capacity_frames) { }
};

} // namespace coreaudio

extern "C" {

typedef struct jlca_handle jlca_handle;

auto jlca_create(int sample_rate, int capacity_frames) -> jlca_handle *;
auto jlca_destroy(jlca_handle *h) -> void;

auto jlca_start(jlca_handle *h) -> int;
auto jlca_stop(jlca_handle *h) -> void;

auto jlca_set_volume(jlca_handle *h, int percent) -> int;

auto jlca_available_write(jlca_handle *h) -> int;
auto jlca_begin_write(jlca_handle *h) -> float *;
auto jlca_begin_write_frames(jlca_handle *h) -> int;
auto jlca_end_write(jlca_handle *h, int frames) -> void;
}
