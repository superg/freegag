#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <unordered_map>

namespace freegag
{
inline constexpr uint32_t runtime_infinite_wait = UINT32_MAX;

class RuntimeManualResetEvent;

class RuntimeMutex
{
public:
    RuntimeMutex()
        : mutex_(std::make_unique<std::recursive_mutex>())
    {
    }
    RuntimeMutex(RuntimeMutex &&) noexcept = default;
    RuntimeMutex &operator=(RuntimeMutex &&) noexcept = default;
    RuntimeMutex(const RuntimeMutex &) = delete;
    RuntimeMutex &operator=(const RuntimeMutex &) = delete;

    void lock()
    {
        mutex_->lock();
    }

    void unlock()
    {
        mutex_->unlock();
    }

private:
    std::unique_ptr<std::recursive_mutex> mutex_;
};
using RuntimeThreadId = std::thread::id;

inline RuntimeThreadId runtime_thread_id()
{
    return std::this_thread::get_id();
}

inline void runtime_sleep(uint32_t milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline void lock_runtime_mutex(RuntimeMutex *mutex)
{
    mutex->lock();
}

inline void lock_runtime_mutex_forever(RuntimeMutex *mutex, uint32_t)
{
    mutex->lock();
}

inline void unlock_runtime_mutex(RuntimeMutex *mutex)
{
    mutex->unlock();
}

inline void wait_runtime_event(RuntimeManualResetEvent *event);

class RuntimeManualResetEvent
{
public:
    explicit RuntimeManualResetEvent(bool signaled = false)
        : signaled_(signaled)
    {
    }

    void set()
    {
        {
            std::lock_guard lock(mutex_);
            signaled_ = true;
        }
        changed_.notify_all();
    }

    void reset()
    {
        std::lock_guard lock(mutex_);
        signaled_ = false;
    }

    void wait()
    {
        std::unique_lock lock(mutex_);
        changed_.wait(lock, [&] { return signaled_; });
    }

    bool wait_for(std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(mutex_);
        return changed_.wait_for(lock, timeout, [&] { return signaled_; });
    }

private:
    std::mutex mutex_;
    std::condition_variable changed_;
    bool signaled_{};
};

inline void wait_runtime_event(RuntimeManualResetEvent *event)
{
    event->wait();
}

inline void set_runtime_event(RuntimeManualResetEvent *event)
{
    event->set();
}

class RuntimeHeap
{
public:
    ~RuntimeHeap()
    {
        release_all();
    }

    void *allocate(size_t size, bool zeroed = false)
    {
        if(size == 0)
            size = 1;
        std::unique_ptr<uint8_t[]> storage(zeroed ? new (std::nothrow) uint8_t[size]{} : new (std::nothrow) uint8_t[size]);
        if(storage == nullptr)
            return nullptr;
        void *memory = storage.get();
        std::lock_guard lock(mutex_);
        allocations_.emplace(memory, Allocation{ std::move(storage), size });
        return memory;
    }

    void *reallocate(void *memory, size_t size, bool zeroed = false)
    {
        if(memory == nullptr)
            return allocate(size, zeroed);
        std::lock_guard lock(mutex_);
        const auto found = allocations_.find(memory);
        if(found == allocations_.end())
            return nullptr;
        if(size == 0)
            size = 1;
        std::unique_ptr<uint8_t[]> replacement(zeroed ? new (std::nothrow) uint8_t[size]{} : new (std::nothrow) uint8_t[size]);
        if(replacement == nullptr)
            return nullptr;
        std::memcpy(replacement.get(), found->second.storage.get(), std::min(size, found->second.size));
        allocations_.erase(found);
        void *result = replacement.get();
        allocations_.emplace(result, Allocation{ std::move(replacement), size });
        return result;
    }

    bool release(void *memory)
    {
        if(memory == nullptr)
            return true;
        std::lock_guard lock(mutex_);
        return allocations_.erase(memory) != 0;
    }

    void release_all()
    {
        std::lock_guard lock(mutex_);
        allocations_.clear();
    }

private:
    struct Allocation
    {
        std::unique_ptr<uint8_t[]> storage;
        size_t size{};
    };

    std::recursive_mutex mutex_;
    std::unordered_map<void *, Allocation> allocations_;
};

inline constexpr uint32_t runtime_heap_zero_memory = 0x8;
inline RuntimeHeap runtime_process_heap_storage;

inline RuntimeHeap *runtime_process_heap()
{
    return &runtime_process_heap_storage;
}

inline RuntimeHeap *create_runtime_heap(uint32_t, size_t, size_t)
{
    return new (std::nothrow) RuntimeHeap;
}

inline bool destroy_runtime_heap(RuntimeHeap *heap)
{
    delete heap;
    return true;
}

inline void *allocate_runtime_heap(RuntimeHeap *heap, uint32_t flags, size_t bytes)
{
    return heap == nullptr ? nullptr : heap->allocate(bytes, (flags & runtime_heap_zero_memory) != 0);
}

inline void *reallocate_runtime_heap(RuntimeHeap *heap, uint32_t flags, void *memory, size_t bytes)
{
    return heap == nullptr ? nullptr : heap->reallocate(memory, bytes, (flags & runtime_heap_zero_memory) != 0);
}

inline bool free_runtime_heap(RuntimeHeap *heap, uint32_t, void *memory)
{
    return heap != nullptr && heap->release(memory);
}
} // namespace freegag
