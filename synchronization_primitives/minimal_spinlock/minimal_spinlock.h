// minimal_spinlock.h
#pragma once
#include <atomic>

struct SpinLock {
    // default-initialized to "clear" (unlocked)
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

    void lock() noexcept {
        // TAS: repeatedly set the flag; if it was already set, keep spinning
        while (flag.test_and_set(std::memory_order_acquire)) {
            // hot spin; consider a pause/yield in real code
        }
    }

    void unlock() noexcept {
        flag.clear(std::memory_order_release);
    }

    void try_lock() noexcept {
        return !flag.test_and_test(std::memory_order_acquire);
    }
};