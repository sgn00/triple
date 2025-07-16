// g++ -Wall -O3 -march=native -std=c++20 test_triplebuffer.cpp

#include "lock.hpp"
#include "lockfree.hpp"
#include "optimized.hpp"
#include <thread>
#include <iostream>
#include <chrono>
#include <pthread.h>

void pin_thread(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        std::cerr << "set affinity error" << std::endl;
        exit(1);
    }
}

template <typename TripleBuffer>
void bench(int cpu1, int cpu2) {
    TripleBuffer tb;
    std::atomic<bool> stop = false;
    std::thread consumer_th([&](){
        // Let the consumer keep spinning to retrieve updates
        pin_thread(cpu1);
        int new_updates = 0;
		int total_reads = 0;
        while (!stop) {
            auto [ptr, updated] = tb.get_for_reader();
			++total_reads;
            if (updated) {
                ++new_updates;
            }
        }
        std::cout << "Consumer new updates: " << new_updates << ", total read attempts: " << total_reads << std::endl;
    });

    // producer th
    pin_thread(cpu2);
    uint64_t iters = 200'000'000;
    auto start = std::chrono::steady_clock::now();
    for (uint64_t i = 0; i < iters; i++) {
        auto* ptr = tb.get_for_writer();
        *ptr = i;
        tb.publish();
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << iters * 1000000000 / std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                       .count()
            << " write ops/s" << std::endl;
    stop = true;
    consumer_th.join();
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Must supply arg: lock|lockfree|optimized" << std::endl;
        exit(1);
    }
	// Add isolcpus=5,6 nohz_full=5,6 to kernel boot cmd to enable CPU isolation for these 2 cores.
    std::string arg = argv[1];
    if (arg == "lock") {
        bench<triple::lock::TripleBuffer<uint32_t>>(5, 6);
    } else if (arg == "lockfree") {
        bench<triple::lockfree::TripleBuffer<uint32_t>>(5, 6);
    } else if (arg == "optimized") {
        bench<triple::optimized::TripleBuffer<uint32_t>>(5, 6);
    } else {
        std::cerr << "Unknown arg, should be: lock|lockfree|optimized" << std::endl;
        exit(1);
    }
}