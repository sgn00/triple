#pragma once

#include <mutex>
#include <utility>

namespace triple::lock {

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
            std::lock_guard lk {mtx_};
            std::swap(back_idx_, spare_idx_);
            has_update_ = true;
        }

        // Reader API - Reader checks if there's an update on the spare buffer.
        // If there is, it will swap the front and spare buffer.
        // Then it returns the pointer to front buffer for reader to read,
        // and a bool to let reader know if this is 'new' data.
        std::pair<T*, bool> get_for_reader() {
            std::lock_guard lk {mtx_};
            bool updated = has_update_;
            if (has_update_) {
                std::swap(front_idx_, spare_idx_);
                has_update_ = false;
            }
            return {&buffers_[front_idx_], updated};
        }

    private:
        T buffers_[3];
        int front_idx_ = 0;
        int spare_idx_ = 1;
        int back_idx_ = 2;

        bool has_update_ = false;
        std::mutex mtx_;
    };

}
