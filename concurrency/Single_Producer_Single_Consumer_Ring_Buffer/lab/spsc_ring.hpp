#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <new>
#include <memory>
#include <cassert>

namespace lab {

// Small cacheline pad to reduce false sharing between head/tail
struct alignas(64) CachePad { char pad[64]; };

template <typename T, std::size_t CapacityPow2>
class SpscRing {
    static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0,
                  "Capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>,
                  "Lab version requires trivially copyable T");
    
    static constexpr std::size_t kCapacity = CapacityPow2;
    static constexpr std::size_t kMask     = CapacityPow2 - 1;

public:
    SpscRing()
        : storage_(new T[kCapacity]) // lab:default-constructed; OK for trivially copyable
    {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    // non-copyable, movable if you want (omitted for brevity)
    SpscRing(const SpscRing&) = delete;
    SpscRing& operator=(const SpscRing&) = delete;

    // Attempt to enqueue one element.
    // Returns false if the ring is full.
    bool try_push(const T&v) noexcept {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail_snapshot = tail_.load(std::memory_order_acquire); // read other index

        // ring is full if size == kCapacity
        if ((head - tail_snapshot) >= kCapacity) {
            return false;
        }

        storage_[head & kMask] = v; // write data before publishing
        head_.store(head + 1, std::memory_order_release); // publish
        return true;
    }

    // Attempt to dequeue one element into out.
    // Returns false if the ring is empty.
    bool try_pop(T& out) noexcept {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head_snapshot = head_.load(std::memory_order_acquire); // read other index

        if (head_snapshot == tail) {
            return false; // empty
        }

        out = storage_[tail & kMask]; // consume
        tail_.store(tail + 1, std::memory_order_release); // publish
        return true;
    }

    // size() is approximate (non-synchronized); for tests/metrics only
    std::size_t approx_size() const noexcept {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_acquire);
        return h - t;
    }

    static constexpr std::size_t capacity() noexcept { return kCapacity; }

private:
    // Layout to avoid false sharing: put indices on separate cachelines
    alignas(64) std::atomic<std::size_t> head_{0};
    CachePad pad1_;
    alignas(64) std::atomic<std::size_t> tail_{0};
    CachePad pad2_;

    std::unique_ptr<T[]> storage_;
};

} // namespace lab