#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <optional>
#include <cstdio>
#include <immintrin.h>

// Ensure capacity is a power of two at compile time
constexpr bool is_power_of_two(size_t x) {
    return x && !(x & (x - 1));
}

template <typename T, size_t Capacity>
class alignas(64) SPSCQueue {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        static_assert(is_power_of_two(Capacity), "Capacity must be a power of two");
    public:
        SPSCQueue() = default;

        inline bool push(const T& item) {
            const size_t local_tail = tail_.load(std::memory_order_relaxed);
            const size_t next_tail = (local_tail + 1) & index_mask;

            if (next_tail == head_.load(std::memory_order_relaxed)) [[unlikely]] {
                return false; // full
            }

            _mm_prefetch(reinterpret_cast<const char*>(&buffer_[next_tail]), _MM_HINT_T0);
            buffer_[local_tail] = item;
            tail_.store(next_tail, std::memory_order_release);
            return true;
        }

        inline std::optional<T> pop() {
            const size_t local_head = head_.load(std::memory_order_relaxed);

            if (local_head == tail_.load(std::memory_order_relaxed)) [[unlikely]] {
                return std::nullopt; // empty
            }

            _mm_prefetch(reinterpret_cast<const char*>(&buffer_[(local_head + 1) & index_mask]), _MM_HINT_T0);
            T item = buffer_[local_head];
            head_.store((local_head + 1) & index_mask, std::memory_order_release);
            return item;
        }

        // Check if the queue is empty
        inline bool empty() const {
            return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
        }

        // Check if the queue is full
        inline bool full() const {
            return ((tail_.load(std::memory_order_relaxed) + 1) & index_mask) == head_.load(std::memory_order_relaxed);
        }

        // Returns the size of the queue
        inline size_t size() const {
            const size_t h = head_.load(std::memory_order_relaxed);
            const size_t t = tail_.load(std::memory_order_relaxed);
            return (t - h) & index_mask;
        }

        // Debug helper - shows internal queue state
        inline void debug_print() const {
            printf("Head: %zu, Tail: %zu, Empty: %s, Full: %s\n", 
            head_.load(), tail_.load(), 
            empty() ? "yes" : "no", 
            full() ? "yes" : "no");
        }

        // Expose direct buffer access for potential SIMD optimisation
        const T* raw_buffer() const { return buffer_; }
 
    private:
        static constexpr size_t index_mask = Capacity - 1;

        // Storage array
        alignas(64) T buffer_[Capacity];

        // Atomic pointers to head and tail - thread-safe, prevent race conditions
        alignas(64) std::atomic<size_t> head_{0};
        alignas(64) std::atomic<size_t> tail_{0};
};