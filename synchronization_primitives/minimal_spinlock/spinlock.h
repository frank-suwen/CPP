// spinlock.h
#pragma once
#include <atomic>
#include <thread>
#include <chrono>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #include <immintrin.h>
    static inline void cpu_relax() noexcept { _mm_pause(); }
#elif defined(__aarch64__) || defined(__arm__)
    #include <arm_acle.h>
    static inline void cpu_relax() noexcept { __yield(); }
#else
    static inline void cpu_relax() noexcept { std::this_thread::yield ;}
#endif

class SpinLock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() noexcept {
        // TTAS: first check (relaxed) to avoid cache-line ping-pong, then TAS.
        size_t spins = 0;
        while (true) {
            // inner read-only spin while someone else holds the lock
            while (flag.test(std::memory_order_relaxed)) {
                cpu_relax();
                if (++spins % 256 == 0) {               // polite backoff under contention
                    std::this_thread::yield();          // let OS run another thread
                }
            }
            // try to acquire
            if (!flag.test_and_set(std::memory_order_acquire)) return;
            // failed: brief backoff to reduce contention
            cpu_relax();
        }
    }

    bool try_lock() noexcept {
        // succeeds only if lag was clear
        return !flag.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept {
        flag.clear(std::memory_order_release);
    }
};

// RAII helper so you can't forget unlock()
class SpinGuard {
    SpinLock& m;
    bool owns;
public:
    explicit SpinGuard(SpinLock& m_) : m(m_), owns(true) { m.lock(); }
    ~SpinGuard() { if (owns) m.unlock(); }
    SpinGuard(const SpinGuard&) = delete;
    SpinGuard& operator=(const SpinGuard&) = delete;
    SpinGuard(SpinGuard&& o) noexcept : m(o.m), owns(o.owns) { o.owns = false; }
    SpinGuard& operator=(SpinGuard&&) = delete;
};