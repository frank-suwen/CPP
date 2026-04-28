#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include <stdexcept>

using Clock = std::chrono::steady_clock;

class SpscRingBuffer {
public:
    explicit SpscRingBuffer(std::size_t capacity) : capacity_(capacity), mask_(capacity - 1), buffer_(capacity) {
        if ((capacity_ & mask_) != 0) {
            throw std::runtime_error("capacity must be power of two");
        }
    }

    bool push(std::int64_t value) {
        std::size_t tail = tail_.load(std::memory_order_relaxed);
        std::size_t next_tail = (tail + 1) & mask_;

        std::size_t head = head_.load(std::memory_order_relaxed);
        if (next_tail == head) {
            return false; // full
        }

        buffer_[tail] = value;

        tail_.store(next_tail, std::memory_order_relaxed);
        return true;
    }

    bool pop(std::int64_t& value) {
        std::size_t head = head_.load(std::memory_order_relaxed);

        std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (head == tail) {
            return false; // empty
        }

        value = buffer_[head];

        std::size_t next_head = (head + 1) & mask_;
        head_.store(next_head, std::memory_order_relaxed);
        return true;
    }

private:
    const std::size_t capacity_;
    const std::size_t mask_;
    std::vector<std::int64_t> buffer_;

    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

int main() {
    constexpr std::size_t capacity = 1 << 20; // 1,048,576 slots
    constexpr std::int64_t N = 50'000'000;

    SpscRingBuffer rb(capacity);

    auto start = Clock::now();

    std::thread producer([&]() {
        for (std::int64_t i = 0; i < N; ++i) {
            while (!rb.push(i)) {
                // busy wait
            }
        }
    });

    std::thread consumer([&]() {
        std::int64_t expected = 0;
        std::int64_t value = 0;

        while (expected < N) {
            if (rb.pop(value)) {
                if (value != expected) {
                    std::cerr << "Data mismatch: expected "
                              << expected << " , got " << value << "\n";
                    std::terminate();
                }
                ++expected;
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = Clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Transferred " << N << " items in "
              << elapsed.count() << " s\n";
    std::cout << "Throughput = "
              << static_cast<double>(N) / elapsed.count()
              << " items/second\n";
    
    return 0;
}