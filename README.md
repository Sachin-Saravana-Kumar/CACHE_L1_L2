# L1/L2 Cache Simulator with Prefetcher

## Overview
This project implements a **L1/L2 cache simulator** with a configurable **prefetcher** to analyze and improve memory access efficiency. The simulator models a two-level cache hierarchy, handles memory accesses, and evaluates cache performance metrics. Additionally, it incorporates the **Write-Back Write-Allocate (WBWA)** technique to optimize memory write operations.

## Features
- Simulates **L1 and L2 caches** with customizable configurations (size, associativity, block size, etc.).
- Implements **various cache replacement policies** (e.g., LRU).
- Supports **configurable prefetching strategies** (e.g., next-line, stride, custom prefetchers).
- Implements **Write-Back Write-Allocate (WBWA)** for efficient memory writes, reducing main memory accesses.
- Provides detailed **cache performance statistics** (hit/miss rates, access times, etc.).
- Reads memory access traces for realistic workload evaluation.

## Installation
### Prerequisites
Ensure you have the following dependencies installed:
- C/C++ compiler (e.g., GCC, Clang)
- Python (if used for analysis or trace generation)
- Make (optional, for build automation)

### Build Instructions
```sh
# Clone the repository
git clone https://github.com/Sachin-Saravana-Kumar/CACHE_L1_L2
cd l1-l2-cache-simulator

# Compile the simulator
make  # If using a Makefile
# OR manually compile using g++
g++ -o cache_simulator main.cpp cache.cpp prefetcher.cpp -O2
```

## Usage
### Running the Simulator
```sh
./cache_simulator <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>
```
Example:
```sh
./cache_simulator 64 32768 8 262144 16 1 2 traces/memory_access.trace
```

### Command-Line Arguments
- `<BLOCKSIZE>`: Cache block size in bytes
- `<L1_SIZE>`: L1 cache size in bytes
- `<L1_ASSOC>`: L1 cache associativity
- `<L2_SIZE>`: L2 cache size in bytes
- `<L2_ASSOC>`: L2 cache associativity
- `<PREF_N>`: Prefetcher N parameter
- `<PREF_M>`: Prefetcher M parameter
- `<trace_file>`: Path to the memory access trace file

### Sample Output
```
L1 Cache Hits:  105432
L1 Cache Misses: 20432
L2 Cache Hits:  15032
L2 Cache Misses: 5412
Total Accesses: 125000
Hit Rate: 89.6%
```

## Write-Back Write-Allocate (WBWA) Technique
The simulator implements the **Write-Back Write-Allocate (WBWA)** policy for handling write operations efficiently:
- **Write-Back**: Instead of writing data immediately to memory, it is stored in the cache and written to memory only when evicted, reducing memory traffic.
- **Write-Allocate**: On a write miss, the block is loaded into the cache first before modifying it, improving performance for future accesses.
- This approach minimizes expensive memory writes and improves overall performance compared to Write-Through policies.

## Prefetching Strategies
- **No Prefetching**: Standard demand-fetch mechanism.
- **Next-Line Prefetching**: Prefetches the next cache line on every miss.
- **Stride Prefetching**: Detects access patterns and prefetches accordingly.
- **Custom Prefetchers**: Users can implement their own prefetching logic.

## Performance Evaluation
You can analyze the simulator's performance using different traces and configurations. The project includes:
- **Trace files** for testing different memory access patterns.
- **Python scripts** for performance visualization.
