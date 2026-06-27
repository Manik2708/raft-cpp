//
// Created by Manik mehta on 10/06/26.
//

#ifndef RAFT_CPP_INFLIGHTS_H
#define RAFT_CPP_INFLIGHTS_H

#include <memory>
#include <vector>

namespace trkr {
    struct inflight {
        uint64_t index;
        uint64_t bytes;
    };

    class InFlights {
        int start_ = 0;
        int count_ = 0;
        uint64_t bytes_ = 0;
        int size_;
        uint64_t max_bytes_;
        std::vector<inflight> buffer_;

        void grow();
    public:
        InFlights(int size, uint64_t max_bytes);
        void add(uint64_t index, uint64_t bytes);
        bool full() const;
        void free_le(uint64_t to);
        void reset();
    };
}

#endif //RAFT_CPP_INFLIGHTS_H
