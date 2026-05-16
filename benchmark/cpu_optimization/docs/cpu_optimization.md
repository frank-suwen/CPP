# CPU OPtimization Notes: Branching and Profiling

## 1. goal

This week focused on CPU-level performance concepts:

- branch prediction
- branchless programming
- compiler optimization effects
- profiling with `perf`
- hotspot analysis with flamegraphs

The goal was to connect CPU theory to actual benchmark and profiling results.

## 2. Branch vs Brachless Benchmark

### Setup

I benchmarked two implementations over a large vector of integers:

1. Branch version:

```cpp
if (x >= 128) {
    sum += x;
}
```

2. Branchless version:

```cpp
sum += (x >= 128) * x;
```

I tested both random and sorted data.

3. Results with Normal Optimized Build

Compiled with:

```bash
g++ -O3 -std=c++20 -march=native -o branch_benchmark branch_benchmark.cpp
```

Case                   | Branch     | Branchless |
---------------------- | ---------- | ---------- |
random / unpredictable | 0.0380357s | 0.0381878s |
sorted / predictable   | 0.0390091s | 0.038552s  |

**Interpretation**

With the normal optimization build, the branch and branchless versions performed almost the same. 

This suggests that the compiler likely optimized the loop aggressively, possibly through vectorization or conditional-move style transformations. It also suggests that the benchmark may be limited more by memory streaming or compiler-generated code than by branch misprediction alone.

4. Results with Vectorization Disabled

Compiled with:

```bash
g++ -O3 -std=c++20 -march=native -fno-tree-vectorize -o branch_benchmark_no_vec branch_benchmark.cpp
```

Case                   | Branch     | Branchless |
---------------------- | ---------- | ---------- |
random / unpredictable | 0.0875831s | 0.0817157s |
sorted / predictable   | 0.0883281s | 0.0820071s |

**Interpretation**

After disabling vectorization, the branchless version became about 6-8% faster.

This shows that branchless code can help when scaler control-flow overhead matters. However, the benefit is not automatic and depends on compiler optimization, CPU behavior, and workload characteristics.

The key lession is:

Branchless code should be justified by measurement, not assumed to be faster. 

5. **perf stat** Limitation

On this Azure VM, direct branch counters were not available because `perf_event_paranoid` was set to `4` and the VM did not expose the required PMU events.

As a result, I could not directly collect reliable branches or branch-misses counters for this lab.

Instead, I used timing-based evidence and compared builds with and without vectorization.

6. Black-Scholes / Monte Carlo Profiling Setup

I created a standalone profiling target separate from the main project to avoid changing the production project structure.

The profiling target included:
- closed-form Black-Scholes call pricing
- normal CDF using `std::erfc`
- Monte Carlo call pricing
- `__attribute__((noinline))` to preserve function boundaries
- `-g -fno-omit-frame-pointer` for profiling support

Compiled with:

```bash
g++ -O3 -std=c++20 -g -fno-omit-frame-pointer -march=native -o bs_profile bs_profile.cpp
```

Normal runtime:

```text
mc_price = 10.4522
elapsed = 2.9966s
```

7. **peerf record** and Hotspot Analysis

I profiled the target with:

```bash
sudo perf record -g ./bs_profile
sudo perf report
```

I also saved the report:

```bash
sudo perf report --stdio > perf_report.txt
```

Main observed hotspots:

Symbol                   | Observation                 |
------------------------ | --------------------------- |
`bs_call_price`          | around 48-50% children time |
`erfcf32x`               | around 31.5% self time      |
`monte_carlo_call_price` | around 16.9% self time      |
`normal_cdf`             | visible in profile          |
`exp / log`              | visible math-library costs  |

**Interpretaation**

The profile showed that the closed-form Black-Scholes pricing path dominated this workload.

The largest cost came from the normal CDF implementation:

```cpp
double normal_cdf(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}
```

The math library symbol `erfcf32x` alone accounted for about 31.5% self time.

Monte Carlo pricing was visible but smaller in this workload, accounting for about 16.9% self time.

This suggests that for this benchmark, optimizing or approximating the normal CDF could have more impact than focusing only on the Monte Carlo loop.

8. Flamegraph

I generated a flamegraph with:

```bash
sudo perf script > out.perf
~/FlameGraph/stackcollapse-perf.pl out.perf > out.folded
~/FlameGraph/flamegraph.pl out.folded > bs_profile_flamegraph.svg
```

Generated profiling artifacts:

```text
perf.data
perf_report.txt
out.perf
out.foldeed
bs_profile_flamegraph.svg
```

The flamegraph visually confirmed the same hotspot pattern: the widest section was in the Black-Scholes pricing path, especially the `erfc` / normal CDF computation.

9. Lession Learned 

**Branching**

Branchless code can reduce branch-related overhead, but it is not universally faster. Compiler optimizations such as vectorization can eliminate or reduce the expected difference. 

**Profiling**

Performance work should start with measurement. In this profile, the expensive math library calls were more important than guessing about where time was spent. 

**CPU Optimization**

Micro-optimization should be guided by actual hotposts. For this workload, the best next optimization target would likely be the normal CDF implementation rather than broad rewrites of the entire pricing code. 

10. Final Takeaway

CPU optimization requires both low-level knowledge and disciplined measurement.

Important habits:
- form a hypothesis
- benchmark carefully
- profile before optimizing
- understand compiler effects
- focus on real hotspots
- avoid assuming that lower-level or branchless code is always faster