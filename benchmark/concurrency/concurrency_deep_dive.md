# Concurrency Deep Dive: Memory Ordering and False Sharing

## 1. Goal

This week focused on two important low-level concurrency concepts:

- C++ atomic memory ordering
- false sharing and cache-line contention

The goal was to connect theory from the C++ memory model and OS synchronization primitives to measurable behavior in small C++ experiments.

---

## 2. False Sharing Experiment

### Setup 

I benchmarked multiple threads incrementing separate atomic counters.

Two layouts were compared:

1. Compact counters
2. Cache-line-aligned padded counters using `alignas(64)`

### Results

| Version          | Size / Alignment              | Time      |
| ---------------- | ----------------------------- | --------- |
| Compact counters | `sizeof = 8`, `alignof = 8`   | 7.37351 s |
| Padded counters  | `sizeof = 64`, `alignof = 64` | 5.29103 s |

The padded version was about: 7.37351 / 5.29103 ≈ 1.39x faster

### Explanation

In the compact version, each counter is only 8 bytes, so multiple counters can fit inside the same 64-byte cache line. 

Even though each thread updates a different counter, the CPU cache coherence protocol works at the cache-line level, not the variable level. Therefore, different cores repeatedly invalidate each other's cache line when updating nearby counters.

This is called false sharing.

In the padded version, `alignas(64)` makes each counter occupy its own cache line. This reduces unnecessary cache-line bouncing and improves performance.

### Key Takeway

False sharing can hurt performance even when threads do not logically share the same variable. 

The important hardware unit is often the cache line, not the C++ object.

## 3. SPSC Ring Buffer Experiment

### Setup

I implemented a single-producer single-consumer ring buffer from scratch.

The ring buffer used:

- power-of-two capacity
- bit-mask wraparound instead of modulo
- separate `head` and `tail` atomics
- cache-line alignment for `head` and `tail`

The producer pushes sequential integers into the buffer, and the consumer pops and verifies them.

### Optimization: Mask Intead of Modulo

The first version used:
```C++
(index + 1) % capacity
```

The optimized version used:
```C++
(index + 1) & mask
```
This works when the capacity is a power of two.

This is common in low-latency systems because bitwise operations are cheaper than modulo in hot loops.

## 4. Memory Ordering Comparison

I compared two versions:

1. Acquire/release version
2. Relaxed-only version

### Results

| Version               |          Throughput |
| --------------------- | ------------------: |
| relaxed-only run 1    | 1.56033e8 items/sec |
| relaxed-only run 2    | 1.56196e8 items/sec |
| relaxed-only run 3    | 1.54008e8 items/sec |
| acquire/release run 1 | 1.56683e8 items/sec |
| acquire/release run 2 | 1.57697e8 items/sec |
| acquire/release run 3 | 1.54476e8 items/sec |

The two versions performed similarly on this VM.

### Interpretation

This shows that weaker memory ordering is not automatically faster in practice. 

The relaxed-only version may appear to work on this machine, especially on x86-like systems with relatively strong memory ordering. However, the acquire/release version better expresses the required synchronization relationship. 

The producer writes data first:
```C++
buffer_[tail] = value;
```
Then publishes the new tail:
```C++
tail_.store(next_tail, std::memory_order_release);
```
The consumer observes the producer's published tail:
```C++
tail_.load(std::memory_order_acquire);
```
Then reads the buffer slot:
```C++
value = buffer_[head];
```
This acquire/release pairing means:

If the consumer sees the updated `tail`, it should also see the data written before that tail update. 

### Key Takeaway

Use `memory_order_relaxed` only when atomicity is enough and no ordering relationship is needed.

Use `memory_order_acquire` / `memory_order_release` when one thread publishes data and another thread consumes it.

## 5. Low-Latency Systems Lessons

### Lesson 1: Cache line matter

False sharing shows that performance can degrade even when variables are logically independent. In low-latency systems, objects placement and alignment matter.

### Lesson 2: Correctness comes before microbenchmark speed

A relaxed-only version may run successfully in a benchmark, but that does not prove it is correct under the C++ memory model.

### Lesson 3: Mechanical sympathy matters

Small implementation details can matter:

- power-of-two ring buffer capacity
- bit-mask wraparound
- cache-line alignment
- avoid unnecessary synchronization
- choosing the weakest correct memory ordering

### Lesson 4: Benchmark results need interpretation

Benchmark numbers depend on CPU, OS scheduling, VM behavior, and compiler optimizations. The goal is not only to collect numbers, but to explain them using the memory model and hardware behavior.

## 6. Final Takeaway

Advanced concurrency performance depends on both software correctness and hdardware behavior.

For quant dev, HFT, and infrastructure engineering, it is not enough to know that atomics exist. A strong engineer should understand:

- what synchronization relationship is required
- which memory ordering expresses that relationship
- how cache-line layout can affect throughput
- why benchmark results must be interpreted carefully