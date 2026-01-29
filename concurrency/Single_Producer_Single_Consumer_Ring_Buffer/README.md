## SPSC Ring Buffer (Lab)

- **Model:** Single-producer / single-consumer (SPSC), lock-free; power-of-two capacity with mask wrap.
- **Indices:** Monotonic `head/tail` with `head - tail` determining full/empty; slows indexed by `(idx & (Capacity-1))`
- **Memory ordering:** Producer writes slot --> `head.store(..., release)`; consumerreads `head` with `acquire` (and vice-versa for `tail`). Ensures fully-initialized data is overved.
- **Assumptions:** Exactly one producer thread and one consumer thread; trivally copyable `T`; busy-wait when full/emtpy.
- **Throughput (exaple on my VM):** `~XX ns/transfer` (≈ `YY M transfers/s`) at capacity 4096, N=2e6, `-O3 -march=native =pthread -DNDEBUG`.

### Build & Run
```bash
# build
mkdir -p build
g++ -std=c++20 -O3 -march=native -pthread -DNDEBUG -o build/bench_spsc bench_main.cpp ring_buffer.cpp

# tests
g++ -std=c++20 -O2 -pthread -o build/test_smoke tests/test_smoke.cpp
g++ -std=c++20 -O2 -pthread -o build/test_threads tests/test_threads.cpp

# run
./build/test_smoke
./build/test_threads
./build/bench_spsc 2000000
```

### output
- **transfers = 2000000**
- **time_ns = 985572270**
- **ns_per_transfer = 492.786**
- **M_transfers_per_s = 2.02928**