#pragma once

#include <atomic>
#include <utility>

namespace triple::optimized {

template <typename T>
struct TripleBuffer {

    T* get_for_writer() {
        return &buffers_[back_idx_].buf;
    }

    void publish() {
        State new_spare {back_idx_, true};
        State prev_spare = spare_.exchange(new_spare, std::memory_order_acq_rel);
        back_idx_ = prev_spare.idx;
    }

    std::pair<T*, bool> get_for_reader() {
        State curr_spare = spare_.load(std::memory_order_relaxed);
        bool updated = curr_spare.has_update;
        if (curr_spare.has_update) {
            State new_spare {front_idx_, false};
            State prev_spare = spare_.exchange(new_spare, std::memory_order_acq_rel);
            front_idx_ = prev_spare.idx;
        }
        return {&buffers_[front_idx_].buf, updated};
    }

private:
    static constexpr size_t keep_apart_sz = std::hardware_destructive_interference_size;

    struct State {
        int idx;
        bool has_update;
    };

    struct alignas(keep_apart_sz) Buffer {
        T buf;
    };

    Buffer buffers_[3];
    alignas(keep_apart_sz) int front_idx_ = 0;
    alignas(keep_apart_sz) std::atomic<State> spare_ {{1, false}};
    alignas(keep_apart_sz) int back_idx_ = 2;
};

}