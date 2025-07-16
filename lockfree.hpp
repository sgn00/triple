#pragma once

#include <atomic>
#include <utility>

namespace triple::lockfree {

template <typename T>
struct TripleBuffer {

    // Writer API - returns a pointer to the back buffer for writer
    // to write into.
    T* get_for_writer() {
        return &buffers_[back_idx_];
    }

    // Writer API - Once finished writing, writer calls publish to swap
    // the back and spare buffer.
    void publish() {
        State new_spare {back_idx_, true};
        State prev_spare = spare_.exchange(new_spare, std::memory_order_acq_rel);
        back_idx_ = prev_spare.idx;
    }

    // Reader API - Reader checks if there's an update on spare buffer.
    // If there is, it will swap the front and spare buffer.
    // Then it returns the pointer to front buffer for reader to read,
    // and a bool to let reader know if this is 'new' data.
    std::pair<T*, bool> get_for_reader() {
        State curr_spare = spare_.load(std::memory_order_relaxed);
        bool updated = curr_spare.has_update;
        if (curr_spare.has_update) {
            State new_spare {front_idx_, false};
            State prev_spare = spare_.exchange(new_spare, std::memory_order_acq_rel);
            front_idx_ = prev_spare.idx;
        }
        return {&buffers_[front_idx_], updated};
    }

private:
    // On an x86-64 platform, this struct is 8 bytes (inclusive of padding),
    // so a xchg instruction can swap it atomically.
    struct State {
        int idx;
        bool has_update;
    };

    // Implementation is lock-free as long as this static_assert passes
    static_assert(std::atomic<State>::is_always_lock_free);

    T buffers_[3];
    int front_idx_ = 0;
    std::atomic<State> spare_ {{1, false}};
    int back_idx_ = 2;
};

}

