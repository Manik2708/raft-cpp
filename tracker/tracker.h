//
// Created by Manik mehta on 13/06/26.
//

#ifndef RAFT_CPP_TRACKER_H
#define RAFT_CPP_TRACKER_H
#include <ranges>
#include "progress.h"
#include "quorum/majority.h"

namespace trkr {
    template <typename F>
    concept ProgressVisitor = std::invocable<F, const uint64_t, Progress*>;

    class ProgressTracker {
        class MatchAckIndexer : public quorum::AckedIndexer {
            std::unordered_map<uint64_t, uint64_t> indexer_;
        public:
            explicit MatchAckIndexer(const std::unordered_map<uint64_t, std::unique_ptr<Progress>>& indexer);
            std::tuple<uint64_t, bool> acked_index(uint64_t id) const override;
        };
    public:
        std::unordered_map<uint64_t, std::unique_ptr<Progress>> progress;
        quorum::MajorityConfig voters;
        std::unordered_map<uint64_t, bool> votes;
        int max_inflight;
        uint64_t max_inflight_bytes;

        bool is_singleton() const;
        uint64_t committed() const;
        template <ProgressVisitor F>
        void visit(F&& fn) {
            auto n = progress.size();
            std::vector<uint64_t> ids(n);
            for (const auto &id: progress | std::views::keys) {
                n--;
                ids[n] = id;
            }
            std::ranges::sort(ids.begin(), ids.end());
            for (uint64_t id : ids) {
                fn(id, progress[id].get());
            }
        }
        bool quorum_active();
        std::vector<uint64_t> voter_nodes() const;
        void reset_votes();
        void record_vote(uint64_t id, bool vote);
        std::tuple<int, int, quorum::VoteResult> tally_votes() const;
    };
}

#endif //RAFT_CPP_TRACKER_H
