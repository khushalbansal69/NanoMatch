#pragma once
#include <cstdlib>
#include <stdexcept>
#include <vector>

template<typename T, size_t N>
class MemoryPool {
public:
    MemoryPool() {
        storage_ = static_cast<T*>(aligned_alloc(alignof(T), sizeof(T) * N));
        if (!storage_) throw std::bad_alloc();
        free_list_.reserve(N);
        for (size_t i = 0; i < N; ++i)
            free_list_.push_back(&storage_[i]);
    }

    ~MemoryPool() { free(storage_); }

    MemoryPool(const MemoryPool&)            = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    T* Acquire() {
        if (free_list_.empty()) throw std::bad_alloc();
        T* slot = free_list_.back();
        free_list_.pop_back();
        return new(slot) T();
    }

    void Return(T* obj) {
        obj->~T();
        free_list_.push_back(obj);
    }

    size_t Available() const { return free_list_.size(); }
    size_t Capacity()  const { return N; }

private:
    T*               storage_;
    std::vector<T*>  free_list_;
};
