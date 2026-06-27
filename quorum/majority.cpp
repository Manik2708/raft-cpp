//
// Created by Manik mehta on 12/06/26.
//

#include "majority.h"
namespace quorum {
    std::vector<uint64_t> MajorityConfig::slice() const {
        std::vector<uint64_t> result;
        for (const uint64_t& id : cfg_) {
            result.push_back(id);
        }
        std::ranges::sort(result.begin(), result.end());
        return result;
    }

    uint64_t MajorityConfig::commited_index(const AckedIndexer& indexer) const {
        const unsigned long int n = cfg_.size();
        if (n == 0) {
            return UINT64_MAX;
        }
        std::vector<uint64_t> committed_indices(n);
        int iterator = n - 1;
        for (const uint64_t& id : cfg_) {
            if (const auto [index, found] = indexer.acked_index(id); found) {
                committed_indices[iterator] = index;
                iterator--;
            }
        }
        std::ranges::sort(committed_indices.begin(), committed_indices.end());
        return committed_indices[n - (n/2 + 1)];
    }

    VoteResult MajorityConfig::vote_result(const std::unordered_map<uint64_t, bool>& votes) const {
        if (votes.size() == 0) {
            return VOTE_WON;
        }
        int voted_count = 0;
        int missing = 0;
        for (const uint64_t& id : cfg_) {
            if (!votes.contains(id)) {
                missing++;
                continue;
            }
            if (votes.at(id)) {
                voted_count++;
            }
        }
        unsigned long const int q = cfg_.size();
        if (voted_count >= q) {
            return VOTE_WON;
        }
        if (voted_count + missing >= q) {
            return VOTE_PENDING;
        }
        return VOTE_LOST;
    }

    unsigned long int MajorityConfig::size() const {
        return cfg_.size();
    }
}
