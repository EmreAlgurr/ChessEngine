#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <sys/mman.h>
#  include <unistd.h>
#  include <time.h>
#  ifdef __linux__
#    include <sched.h>
#  endif
#endif

namespace Platform {

struct VirtualAllocation {
    void*  ptr = nullptr;
    size_t allocatedBytes = 0;
    bool   usedVirtualPages = false;
    bool   hugePageHinted = false;
};

// Configure userland execution boundaries:
// - Decouple C++ streams from C stdio locks to reduce userland synchronization overhead.
// - Enforce line-buffered stdout for deterministic single-syscall UCI output lines.
// - On Linux, pin the search process/thread to its first allowed physical core to avoid
//   cross-core scheduler migrations and L1/L2 cache invalidation across userland boundaries.
inline void optimizeUserlandBoundaries() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::setvbuf(stdout, nullptr, _IOLBF, 4096);

#ifdef __linux__
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(mask), &mask) == 0) {
        for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
            if (CPU_ISSET(cpu, &mask)) {
                cpu_set_t pin;
                CPU_ZERO(&pin);
                CPU_SET(cpu, &pin);
                sched_setaffinity(0, sizeof(pin), &pin);
                break;
            }
        }
    }
#endif
}

// Allocate page-aligned, zero-initialized virtual memory directly from the OS kernel.
// On Linux, requests Transparent Huge Pages (MADV_HUGEPAGE) to minimize TLB misses
// and userland/kernel page-fault transitions.
inline VirtualAllocation allocVirtualPages(size_t requestedBytes, bool preferHugePages = true) {
    VirtualAllocation alloc;
    if (requestedBytes == 0) return alloc;

#ifdef _WIN32
    (void)preferHugePages;
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    const size_t pageSize = sysInfo.dwPageSize ? static_cast<size_t>(sysInfo.dwPageSize) : 4096;
    const size_t alignedSize = (requestedBytes + pageSize - 1) & ~(pageSize - 1);

    void* p = VirtualAlloc(nullptr, alignedSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (p) {
        alloc.ptr = p;
        alloc.allocatedBytes = alignedSize;
        alloc.usedVirtualPages = true;
        return alloc;
    }
#else
    long ps = sysconf(_SC_PAGESIZE);
    const size_t pageSize = (ps > 0) ? static_cast<size_t>(ps) : 4096;
    const size_t alignedSize = (requestedBytes + pageSize - 1) & ~(pageSize - 1);

    void* p = mmap(nullptr, alignedSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p != MAP_FAILED) {
        alloc.ptr = p;
        alloc.allocatedBytes = alignedSize;
        alloc.usedVirtualPages = true;
#  ifdef MADV_HUGEPAGE
        if (preferHugePages && alignedSize >= (2ULL * 1024ULL * 1024ULL)) {
            if (madvise(p, alignedSize, MADV_HUGEPAGE) == 0) {
                alloc.hugePageHinted = true;
            }
        }
#  else
        (void)preferHugePages;
#  endif
        return alloc;
    }
#endif

    // Fallback: 64-byte aligned heap allocation, explicitly zeroed
    const size_t fallbackSize = (requestedBytes + 63) & ~size_t(63);
#ifdef _WIN32
    void* hp = _aligned_malloc(fallbackSize, 64);
#else
    void* hp = std::aligned_alloc(64, fallbackSize);
#endif
    if (hp) {
        std::memset(hp, 0, fallbackSize);
        alloc.ptr = hp;
        alloc.allocatedBytes = fallbackSize;
        alloc.usedVirtualPages = false;
    }
    return alloc;
}

inline void freeVirtualPages(VirtualAllocation& alloc) {
    if (!alloc.ptr) return;

    if (alloc.usedVirtualPages) {
#ifdef _WIN32
        VirtualFree(alloc.ptr, 0, MEM_RELEASE);
#else
        munmap(alloc.ptr, alloc.allocatedBytes);
#endif
    } else {
#ifdef _WIN32
        _aligned_free(alloc.ptr);
#else
        std::free(alloc.ptr);
#endif
    }

    alloc.ptr = nullptr;
    alloc.allocatedBytes = 0;
    alloc.usedVirtualPages = false;
    alloc.hugePageHinted = false;
}

// High-resolution monotonic clock in nanoseconds.
// On Linux, clock_gettime(CLOCK_MONOTONIC) executes in userland via vDSO without a kernel syscall.
inline int64_t fastNowNs() {
#if !defined(_WIN32) && defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + static_cast<int64_t>(ts.tv_nsec);
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
#endif
}

} // namespace Platform
