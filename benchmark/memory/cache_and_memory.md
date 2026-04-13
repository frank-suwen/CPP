# Cache and Memory: Data-Oriented Design Notes

## Experiment Goal

The purpose of this experiment was to compare iteration performance across three standard C++ containers:

- `std::vector`
- `std::deque`
- `std::list`

The main focus was to understand how memory layout affects runtime performance, especially through cache locality.

## Experiment Setup

Each container stored the same number of integers. The benchmark:

1. Constructed the container first
2. Performed one warmup iteration
3. Timed multiple full traversal passes using a sum loop

This setup reduced one-time construction noise and made the measurement more representative of iteration cost.

## Execution

### Compilation

`g++ -O3 -std=c++20 -march=native -o container_iter_bench container_iter_bench.cpp`

### Run
```
./container_iter_bench vector
./container_iter_bench deque
./container_iter_bench list
```

## Results

| Container | Time       |
| --------- | ---------- |
| `vector`  | 0.0833709s |
| `deque`   | 0.144385s  |
| `list`    | 0.956439s  |

### Relative Comparison

- `deque` was about 1.7x slower than `vector`
- `list` was about 11.5x slower than `vector`
- `list` was about 6.6x slower than `deque`

## Why Cache Matters

Modern CPUs are much faster than main memory, so performance often depends on whether data can stay in cache.

A cache line is typically 64 bytes. Since an `int` is usually 4 bytes, one cache line can hold about 16 integers.

for sequential traversal, this matters a lot:

- With `vector`, adjacent elements are contiguous, so one cache line fetch brings in many useful future elements.
- With `deque`, elements are stored in blocks, so locality is weaker than `vector` but still much better than a linked structure. 
- with `list`, each node is allocated separately, so iteration requires chasing across memory, causing poor localityand higher memory latency.

## Container Choice and Low-Latency Systems

### `std::vector`
Best choice for traversal-heavy workloads.

Why:
- contiguous storage
- strong spatial locality
- hardware prefetcher works well
- low overhead per element

### `std::deque`
Reasonable middle ground.

Why:
- segmented storage
- better than linked lists for traversal
- weaker locality than `vector`

### `std::list`
Usually a poor choice for low-latency iteration-heavy systems.

Why:
- non-contiguous nodes
- pointer chasing
- poor cache behavior
- significantly slower traversal

## Key Insight

This experiment shows that data layout has a major effect on performance.

Even when all three containers conceptually store the same integers and support iteration, the actual runtime differs dramatically because the CPU interacts very differently with contiguous memory versus pointer-based structures.

In low-latency systems such as HFT or systems infrastructure code, choosing a cache-friendly layout is often more important than replying only on abstract container semantics or asymptotic complexity.

## Final Takeaway

When iteraiton is important, prefer contiguous or mostly contiguous containers.

For performance-critical systems:

- prefer `vector` by default
- use `deque` only when its access/insertion tradeoffs are truly needed
- avoid `list` unless there is a very specific reason that outweighs its poor locality

## Lessons Learned

Before this experiment, it was easy to think about containers mainly in terms of their APIs or asymptotic complexity. After benchmarking them directly, it became much clearer that memory layout and cache locality have a first-order impact on performance. This is especially important in low-latency systems, where iteration cost and memory access patterns can dominate runtime.