# Low-Latency Market Data System

market data publishing system that demonstrates low-latency inter-process communication using Shared Memory and TCP.

## Overview

The system consists of three components:

1.  **Publisher (Process A)**: Simulates market data updates and publishes them via:
    - **TCP Loopback**: JSON messages over a non-blocking TCP socket.
    - **Shared Memory**: Binary `Message` structs pushed to a lock-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
2.  **SHM Consumer (Process B)**: Reads messages from Shared Memory with minimal latency.
3.  **TCP Consumer (Process C)**: Connects to the Publisher via TCP and reads JSON messages.

## Prerequisites

- **C++ Compiler**: C++17 compatible (clang++ or g++)
- **Git**: For version control and fetching dependencies
- **Make**: Build system
- **Docker**: For containerized deployment

## 🚀 Run Locally

### Build and Run using CMake

```bash
mkdir build && cd build
cmake ..
make -j4

# Run executables (in separate terminals)
./publisher
./consumer_shm
./consumer_tcp
```

## 🐳 Run with Docker

You can use Docker Compose to spin up the entire system with a single command. The containers use `network_mode: host` and `ipc: host` to enable loopback networking and shared memory access.

```bash
# Build and Start all services
docker-compose up --build
```

You will see output from all three containers in the console, prefixed by their container names.

To stop the system:

```bash
docker-compose down
```

## System Architecture

### Shared Memory (Low Latency)

- **Mechanism**: `mmap` / `shm_open` (POSIX Shared Memory)
- **Data Structure**: Lock-free SPSC Ring Buffer with `std::atomic` indices and cache-line padding (64 bytes) to prevent false sharing.
- **Performance**: Zero system calls in the hot path.

### TCP Networking

- **Mechanism**: POSIX Sockets
- **Optimization**:
  - `TCP_NODELAY` (Nagle's algorithm disabled)
  - Non-blocking I/O
  - Reuse Address enabled

## Dependencies

- [fmt](https://github.com/fmtlib/fmt): Fast and safe string formatting.
- [nlohmann/json](https://github.com/nlohmann/json): JSON manipulation.
