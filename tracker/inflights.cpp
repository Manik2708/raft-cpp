//
// Created by Manik mehta on 10/06/26.
//

#include "inflights.h"
#include <ranges>

namespace trkr {
    InFlights::InFlights(const int size, const uint64_t max_bytes) {
        size_ = size;
        max_bytes_ = max_bytes;
    }

    void InFlights::grow() {
        unsigned long new_size = buffer_.size() * 2;
        if (new_size == 0) {
            new_size = 1;
        } else if (new_size > size_) {
            new_size = size_;
        }
        buffer_.resize(new_size);
    }

    bool InFlights::full() const {
        return buffer_.size() == size_ || (max_bytes_ != 0 && bytes_ >= max_bytes_);
    }

    void InFlights::add(uint64_t index, uint64_t bytes) {
        if (full()) {
            throw std::runtime_error("InFlights::add(): buffer overflow");
        }
        int next = start_ + count_;
        if (next >= size_) {
            next -= size_;
        }
        if (next >= buffer_.size()) {
            grow();
        }
        buffer_[next] = {index, bytes};
        count_++;
        bytes_ += bytes;
    }

    void InFlights::free_le(const uint64_t to) {
        if (count_ == 0 || to < buffer_[start_].index) {
            return;
        }
        int idx = start_;
        int i = 0;
        uint64_t bytes = 0;
        for (i = 0; i < count_; i++) {
            if (to < buffer_[idx].index) {
                break;
            }
            bytes += buffer_[idx].bytes;
            idx++;
            if (idx >= size_) {
                idx -= size_;
            }
        }
        count_ -= i;
        bytes_ -= bytes;
        start_ = idx;
        if (count_ == 0) {
            start_ = 0;
        }
    }

    void InFlights::reset() {
        start_ = 0;
        count_ = 0;
        bytes_ = 0;
    }
}