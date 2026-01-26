#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <array>
#include <new>

namespace MarketData {

    struct Message {
        char instrument[16];
        double bid;
        double ask;
        std::int64_t timestamp_ns;
    };

    template <size_t BufferSize>
    struct RingBuffer {
        static constexpr size_t Capacity = BufferSize;

        alignas(64) std::atomic<size_t> write_idx{0};
        alignas(64) std::atomic<size_t> read_idx{0};
        
        Message buffer[BufferSize];

        size_t size() const {
             size_t w = write_idx.load(std::memory_order_acquire);
             size_t r = read_idx.load(std::memory_order_acquire);
             if (w >= r) return w - r;
             return Capacity - (r - w);
        }
        
        bool push(const Message& msg) {
            size_t w = write_idx.load(std::memory_order_relaxed);
            size_t next_w = (w + 1) % Capacity;
            size_t r = read_idx.load(std::memory_order_acquire);

            if (next_w == r) {
                return false; 
            }

            buffer[w] = msg;
            write_idx.store(next_w, std::memory_order_release);
            return true;
        }

        bool pop(Message& msg) {
            size_t r = read_idx.load(std::memory_order_relaxed);
            size_t w = write_idx.load(std::memory_order_acquire);

            if (r == w) {
                return false; 
            }

            msg = buffer[r];
            size_t next_r = (r + 1) % Capacity;
            read_idx.store(next_r, std::memory_order_release);
            return true;
        }
    };
}
