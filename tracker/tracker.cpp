//
// Created by Manik mehta on 13/06/26.
//

#include "tracker.h"

namespace trkr {
    ProgressTracker::MatchAckIndexer::MatchAckIndexer(const std::unordered_map<uint64_t, std::unique_ptr<Progress>>& indexer) {
        for (const auto& [id, pr] : indexer) {
            indexer_[id] = pr->match;
        }
    }

    std::tuple<uint64_t, bool> ProgressTracker::MatchAckIndexer::acked_index(const uint64_t id) const {
        if (!indexer_.contains(id)) {
            return {0, false};
        }
        return {indexer_.at(id), true};
    }

    bool ProgressTracker::is_singleton() const {
        return voters.size() == 1;
    }

    uint64_t ProgressTracker::committed() const {
        return voters.commited_index(MatchAckIndexer(progress));
    }

    bool ProgressTracker::quorum_active() {
        const std::unordered_map<uint64_t, bool> vts;
        visit([&](const uint64_t id, const Progress* pr) -> void {
            if (pr->is_learner) {
                return;
            }
            votes[id] = pr->recent_active;
        });
        return voters.vote_result(vts) == quorum::VOTE_WON;
    }

    std::vector<uint64_t> ProgressTracker::voter_nodes() const {
        return voters.slice();
    }

    void ProgressTracker::reset_votes() {
        votes.clear();
    }

    // Only records vote when there is no other vote for this id
    void ProgressTracker::record_vote(const uint64_t id, const bool vote) {
        if (!votes.contains(id)) {
            votes[id] = vote;
        }
    }

    std::tuple<int, int, quorum::VoteResult> ProgressTracker::tally_votes() const {
        int granted = 0;
        int rejected = 0;
        for (const auto& id : progress | std::views::keys) {
            if (!votes.contains(id)) {
                continue;
            }
            if (votes.at(id)) {
                granted++;
            } else {
                rejected++;
            }
        }
        quorum::VoteResult result = voters.vote_result(votes);
        return {granted, rejected, result};
    }
}