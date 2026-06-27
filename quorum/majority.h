//
// Created by Manik mehta on 12/06/26.
//

#ifndef RAFT_CPP_MAJORITY_H
#define RAFT_CPP_MAJORITY_H
#include <unordered_set>

namespace quorum {
    enum VoteResult {
        VOTE_WON,
        VOTE_PENDING,
        VOTE_LOST
    };

    class AckedIndexer {
    public:
        virtual ~AckedIndexer() = default;
        virtual std::tuple<uint64_t, bool> acked_index(uint64_t id) const = 0;
    };

    class MajorityConfig {
        std::unordered_set<uint64_t> cfg_;
    public:
        std::vector<uint64_t> slice() const;
        uint64_t commited_index(const AckedIndexer& indexer) const;
        VoteResult vote_result(const std::unordered_map<uint64_t, bool>& votes) const;
        unsigned long int size() const;
    };
}


#endif //RAFT_CPP_MAJORITY_H
