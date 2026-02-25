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
window         p50 RTT (us)          p99 RTT (us)
     1                 13.5                19.601
     8               60.402                70.302
    64              432.714               528.017
```

Interpretation:
- Larger `window` increases **in-flight packets**, which increases **queueing** in socket buffers and scheduling.
- Queueing inflation shows up strongly in the **tail(p99)**

## Assumptions / limitations
- Designed for **Single Client -> Single Server** on **localhost**
- UDP can **drop / duplicate / reoder** packets (localhost usually won't, but the client is written to tolerate late warmup replies)
- RTT timing uses `std::chrono::steady_clock` (monotonic wall-time, not CPU cycles)
- This is a learning lab, not a production network benchmark

## How It Works

### 1. Server: Non-blocking UDP + epoll
The echo server uses:
- `SOCK_DGRAM` --> UDP socket
- `O_NONBLOCK` --> non-blocking I/O
- `epoll` (level-triggered) --> event-driven readiness notification

#### Why epoll?
Instead of blocking on `recvfrom`, we:
1. Register the socket with `epoll`
2. Wait for `EPOLLIN` (readable event)
3. Drain all available packets
4. Echo payload back immediately

This pevents:
- Blocking the entire thrad
- Artifical queue buildup
- Inefficient polling loops

Even though this la uses a single socket, `epoll` scales to thousands of fds.

### 2. Client: Pipelined RTT Measurement

The client:
- Uses `connect()` on UDP
    - Sets default peer
    - Filters incoming packets to that peer
- Sets `O_NONBLOCK`
- Maintains a configurable in-flight **window**

#### Pipelining logic
```txt
while received < n:
    send until (sent - received) == window
    drain all available replies
    poll() briefly if needed
```
Where:
- `sent - received` = number of outstanding packets
- `window` controls artifical load

### 3. Sequence Number Tracking

Each packet stores:
```C++
uint64_t seq
```
in the first 8 bytes of the payload.

Client stores:
```C++
unordered_map<uint64_t, uint64_t> sent_us;
```

Mapping:
```
seq --> send_timestamp
```

ON receive:
```
RTT = now() - send_timestamp
```

This supports:
- Out-of-order replies
- Multiple in-flight packets
- Late warmup replies (ignored safely)

### 4. Why Non-Blocking Drain Matters

If the client reads only one packet at a time:
- Replies sit in the socket buffer
- Measured RTT includes **queue waiting time**
- Tail latency inflates artifically

Switching to:
```C++
while recv(...) until EAGAIN
```
drains bursts quickly and gives more accurate tail behavior.

### 5. Why p99 Increases With Larger Window

When `window` increases:
- More packets are in-flight
- Socket buffers fill
- Server loop processes bursts
- Kernel scheduling delays accumulate

So RTT becomes:
```
RTT = network + server processing + queueing delay
```
Queueing delay dominates p99.

This demonstrates:
- Backpressure
- Head-of-line effects
- Tail amplification under load

### 6. Why UDP + connect()?

Although UDP is connectionless:
```C++
connect(fd, ...)
```
does two useful things:
1. Sets default peer (so we can use `send()` instead of `sendto()`)
2. Filters incoming packets to only that peer

This simplifies the client code.

### 7. What This Lab Demonstrates (Interview Angle)

This lab exercises core low-latency primitives:
- Non-blocking sockets
- epoll event loops
- Windowed pipelining
- RTT percentile measurement
- Tail latency behavior
- Kernel socket buffer dynamics
- Queueing effects under load

There are foundational in:
- HFT market data handlers
- Order gateways
- RPC engines
- Distributed systems infra
- Matching engines
- Microservices under high load

## Where This Could Be Extended
- Add `SO_REUSEPORT`
- Pin threads via `sched_setaffinity`
- Tune `SO_RCVBUF` / `SO_SNDBUF`
- Switch to `recvmmsg()` batching
- Add histogram (HdrHistogram)
- Measure CPU cycles via `perf`
- Compare epoll vs busy-spin polling