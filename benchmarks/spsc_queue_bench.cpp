#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include "common/spsc_queue.hpp"
#include "common/order.hpp"

int main() {
    constexpr int NUM_ITEMS = 1'000'000;
    SPSCQueue<Order, 8192> queue;
    std::atomic<bool> producer_done{false};

    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            Order order = {"BENCH", static_cast<uint64_t>(i), Side::BUY,
                           OrderType::LIMIT, 100.0, 10};
            while (!queue.push(order)) {
                std::this_thread::yield(); // wait if full
            }
        }
        producer_done = true;
    });

    int consumed = 0;
    std::thread consumer([&]() {
        while (!producer_done || !queue.empty()) {
            auto result = queue.pop();
            if (result.has_value()) {
                consumed++;
            } else {
                std::this_thread::yield(); // wait if empty
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_latency_ns = static_cast<double>(duration_ns) / (2 * NUM_ITEMS); // push + pop

    std::cout << "Processed " << NUM_ITEMS << " items.\n";
    std::cout << "Total time: " << duration_ns / 1e6 << " ms\n";
    std::cout << "Average latency: " << avg_latency_ns << " ns per operation (push + pop)\n";

    return 0;
}