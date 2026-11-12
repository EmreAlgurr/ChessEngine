#pragma once

#include "Platform.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>

// Cache-line aligned, virtual-memory-backed custom arena pool.
// Eliminates global heap fragmentation and per-node malloc/new overhead during search.
class MemoryPool {
public:
    MemoryPool() = default;

    explicit MemoryPool(size_t capacityBytes, bool preferHugePages = true) {
        init(capacityBytes, preferHugePages);
    }

    ~MemoryPool() {
        release();
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    MemoryPool(MemoryPool&& other) noexcept
        : allocation(other.allocation), offset(other.offset) {
        other.allocation = Platform::VirtualAllocation{};
        other.offset = 0;
    }

    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            release();
            allocation = other.allocation;
            offset = other.offset;
            other.allocation = Platform::VirtualAllocation{};
            other.offset = 0;
        }
        return *this;
    }

    bool init(size_t capacityBytes, bool preferHugePages = true) {
        release();
        allocation = Platform::allocVirtualPages(capacityBytes, preferHugePages);
        offset = 0;
        return allocation.ptr != nullptr;
    }

    void release() {
        Platform::freeVirtualPages(allocation);
        offset = 0;
    }

    // O(1) bump-pointer aligned allocation inside the contiguous pool
    void* allocateAligned(size_t bytes, size_t alignment = 64) {
        if (!allocation.ptr || bytes == 0) return nullptr;
        const size_t align = (alignment < 8) ? 8 : alignment;
        uintptr_t baseAddr = reinterpret_cast<uintptr_t>(allocation.ptr);
        uintptr_t currAddr = baseAddr + offset;
        uintptr_t alignedAddr = (currAddr + (align - 1)) & ~(static_cast<uintptr_t>(align - 1));
        size_t newOffset = static_cast<size_t>(alignedAddr - baseAddr) + bytes;

        if (newOffset > allocation.allocatedBytes) {
            return nullptr;
        }

        offset = newOffset;
        return reinterpret_cast<void*>(alignedAddr);
    }

    template <typename T>
    T* allocateArray(size_t count, size_t alignment = 64, bool zeroMemory = false) {
        if (count == 0) return nullptr;
        const size_t align = std::max<size_t>(alignment, alignof(T));
        void* mem = allocateAligned(sizeof(T) * count, align);
        if (!mem) return nullptr;

        if (zeroMemory) {
            std::memset(mem, 0, sizeof(T) * count);
        }
        if constexpr (!std::is_trivially_default_constructible_v<T>) {
            T* arr = static_cast<T*>(mem);
            for (size_t i = 0; i < count; ++i) {
                new (&arr[i]) T();
            }
            return arr;
        } else {
            return static_cast<T*>(mem);
        }
    }

    void reset() {
        offset = 0;
    }

    void zeroUsed() {
        if (allocation.ptr && offset > 0) {
            std::memset(allocation.ptr, 0, offset);
        }
    }

    size_t capacity() const { return allocation.allocatedBytes; }
    size_t used() const { return offset; }
    size_t remaining() const { return allocation.allocatedBytes - offset; }
    bool isHugePageHinted() const { return allocation.hugePageHinted; }
    bool isVirtualBacked() const { return allocation.usedVirtualPages; }

private:
    Platform::VirtualAllocation allocation;
    size_t offset = 0;
};

