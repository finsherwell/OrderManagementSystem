#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include "common/spsc_queue.hpp"
#include "common/order.hpp"
#include <iostream>

TEST_CASE("SPSC Queue Basic Operations", "[spsc_queue]") {
    SPSCQueue<int, 8> queue;
    
    SECTION("Empty queue") {
        REQUIRE(queue.empty());
        REQUIRE_FALSE(queue.full());
        REQUIRE_FALSE(queue.pop().has_value());
    }
    
    SECTION("Single push/pop") {
        REQUIRE(queue.push(42));
        REQUIRE_FALSE(queue.empty());
        
        auto result = queue.pop();
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 42);
        REQUIRE(queue.empty());
    }
    
    SECTION("Fill to capacity") {
        // Can store 7 items (capacity - 1)
        for (int i = 0; i < 7; ++i) {
            REQUIRE(queue.push(i));
        }
        REQUIRE(queue.full());
        REQUIRE_FALSE(queue.push(999)); // Should fail
        
        // Pop all items
        for (int i = 0; i < 7; ++i) {
            auto result = queue.pop();
            REQUIRE(result.has_value());
            REQUIRE(result.value() == i);
        }
        REQUIRE(queue.empty());
    }
}

TEST_CASE("SPSC Queue with Order Struct", "[spsc_queue][order]") {
    SPSCQueue<Order, 16> order_queue;
    
    SECTION("Order creation and processing") {
        Order buy_order;
        std::memset(&buy_order, 0, sizeof(Order));
        std::strncpy(buy_order.symbol, "AAPL", sizeof(buy_order.symbol));
        buy_order.order_id = 12345;
        buy_order.side = Side::BUY;
        buy_order.order_type = OrderType::LIMIT;
        buy_order.price = 150.25;
        buy_order.quantity = 100;

        
        REQUIRE(order_queue.push(buy_order));
        
        auto result = order_queue.pop();
        REQUIRE(result.has_value());
        
        Order retrieved = result.value();
        REQUIRE(std::string(retrieved.symbol) == "AAPL");
        REQUIRE(retrieved.order_id == 12345);
        REQUIRE(retrieved.side == Side::BUY);
        REQUIRE(retrieved.order_type == OrderType::LIMIT);
        REQUIRE(retrieved.price == 150.25);
        REQUIRE(retrieved.quantity == 100);
    }
    
    SECTION("Multiple orders") {
        std::vector<Order> orders = {
            {"GOOGL", 1, Side::SELL, OrderType::MARKET, 2500.0, 50},
            {"MSFT", 2, Side::BUY, OrderType::LIMIT, 300.0, 200},
            {"TSLA", 3, Side::BUY, OrderType::LIMIT, 800.0, 75}
        };
        
        // Push all orders
        for (const auto& order : orders) {
            REQUIRE(order_queue.push(order));
        }
        
        // Pop and verify
        for (size_t i = 0; i < orders.size(); ++i) {
            auto result = order_queue.pop();
            REQUIRE(result.has_value());
            REQUIRE(result->order_id == orders[i].order_id);
        }
    }
}

TEST_CASE("SPSC Queue Thread Safety", "[spsc_queue][threading]") {
    SPSCQueue<int, 1024> queue;
    constexpr int NUM_ITEMS = 10000;
    std::atomic<int> consumer_sum{0};
    std::atomic<bool> producer_done{false};
    
    SECTION("Producer-Consumer test") {
        // Producer thread
        std::thread producer([&queue, &producer_done, NUM_ITEMS]() {
            for (int i = 1; i <= NUM_ITEMS; ++i) {
                while (!queue.push(i)) {
                    std::this_thread::yield(); // Busy wait if full
                }
            }
            producer_done = true;
        });

        std::thread consumer([&queue, &consumer_sum, &producer_done, NUM_ITEMS]() {
            int expected_sum = NUM_ITEMS * (NUM_ITEMS + 1) / 2; // Sum 1 to NUM_ITEMS

            while (!producer_done || !queue.empty()) {
                if (auto item = queue.pop()) {
                    consumer_sum += item.value();
                } else {
                    std::this_thread::yield();
                }
            }
        });
        
        producer.join();
        consumer.join();
        
        int expected_sum = NUM_ITEMS * (NUM_ITEMS + 1) / 2;
        REQUIRE(consumer_sum.load() == expected_sum);
    }
}

TEST_CASE("SPSC Queue Performance Characteristics", "[spsc_queue][performance]") {
    SPSCQueue<Order, 4096> queue;
    constexpr int ITERATIONS = 100000;
    
    SECTION("Latency measurement") {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < ITERATIONS; ++i) {
            Order order = {"TEST", static_cast<uint64_t>(i), Side::BUY, 
                          OrderType::LIMIT, 100.0, 10};
            queue.push(order);
            queue.pop();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        double avg_latency = static_cast<double>(duration.count()) / (2 * ITERATIONS); // push + pop
        
        // This is just for visibility - adjust thresholds based on your requirements
        INFO("Average operation latency: " << avg_latency << " ns");
        REQUIRE(avg_latency < 1000); // Less than 1 microsecond per operation
    }
}

// Benchmark helper for integration with other tools
void benchmark_spsc_queue() {
    SPSCQueue<Order, 8192> queue;
    constexpr int WARM_UP = 10000;
    constexpr int ITERATIONS = 1000000;
    
    // Warm up
    for (int i = 0; i < WARM_UP; ++i) {
        Order order = {"WARM", static_cast<uint64_t>(i), Side::BUY, 
                      OrderType::LIMIT, 100.0, 10};
        queue.push(order);
        queue.pop();
    }
    
    // Actual benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        Order order = {"BENCH", static_cast<uint64_t>(i), Side::BUY, 
                      OrderType::LIMIT, 100.0, 10};
        queue.push(order);
        queue.pop();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "Benchmark: " << ITERATIONS << " operations in " 
              << duration.count() << " ns\n";
    std::cout << "Average latency: " 
              << static_cast<double>(duration.count()) / (2 * ITERATIONS) << " ns\n";
}