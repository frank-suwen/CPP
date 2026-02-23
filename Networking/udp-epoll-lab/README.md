# DUP epoll lab (non-blocking echo + RTT percentiles)

A tiny lab to practice low-latency I/O building blocks:
- **UDP echo server** using **non-blocking socket + epoll** (level triggered)
- **UDP client** that measures RTT and reports **p50 / p99** latency
- Client supports **pipelining** via a configurable **in-flight window**

## Build

Release build:
```bash
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j1
```

What the flags mean:
- `-S .`: source directory is current directory
- `-B build`: generate builds files into `./buiild`
- `-DCMAKE_BUILD_TYPE=Release`: enables compiler optimizations (important for latency work)
- `-j1`: build with 1 job (safe for small VM)

## Run

### 1. Start the server (terminal A)
```bash
./build/udp_echo_server --port 9000
```

### 2. run the client (terminal B)
```bash
# Stop-and-wait (window=1)
./build/udp_client --addr 127.0.0.1 --port 9000 --n 100000 --payload 64 --window 1

# Pipelined
./build/udp_client --addr 127.0.0.1 --port 9000 --n 100000 --payload 64 --window 64
```

Notes:
- This lab is indended for **localhost** on a VM (no real network involved).
- `payload >= 8` because the first 8 bytes store the `uint64_t seq`

## Results (localhost VM)
Environment: localhost on Ubuntu VM (small VM; build=Release)

Command: ``--n 100000 --payload 64`

```bash
window                  p50 RTT (us)                    p99 RTT 9(us)
     1                          13.5                           19.601
     8                        60.402                           70.302
    64                       432.714                          528.017
```

Interpretation:
- Larger `window` increases **in-flight packets**, which increases **queueing** in socket buffers and scheduling.
- Queueing inflation shows up strongly in the **tail(p99)**

## Assumptions / limitations
- Designed for **Single Client -> Single Server** on **localhost**
- UDP can **drop / duplicate / reoder** packets (localhost usually won't, but the client is written to tolerate late warmup replies)
- RTT timing uses `std::chrono::steady_clock` (monotonic wall-time, not CPU cycles)
- This is a learning lab, not a production network benchmark