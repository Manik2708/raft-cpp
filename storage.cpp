//
// Created by Manik mehta on 31/05/26.
//

#include "storage.h"

MemoryStorage::MemoryStorage() {
    hard_state_ = std::make_unique<raftpb::HardState>();
    snapshot_ = std::make_unique<raftpb::Snapshot>();
    entries_.push_back(std::make_unique<raftpb::Entry>());
}

StateInfo MemoryStorage::InitialState() {
    call_stats_.initial_state++;
    raftpb::ConfState conf = snapshot_->metadata().conf_state();
    return {
        .hard_state = std::make_unique<raftpb::HardState>(*hard_state_),
        .conf_state = std::make_unique<raftpb::ConfState>(conf),
    };
}

void MemoryStorage::SetHardState(std::unique_ptr<raftpb::HardState> hard_state) {
    std::unique_lock lock(mu_);
    hard_state_ = std::move(hard_state);
}

std::vector<std::unique_ptr<raftpb::Entry>> MemoryStorage::Entries(uint64_t low, uint64_t high) {
    std::shared_lock lock(mu_);
    call_stats_.entries++;
    uint64_t offset = entries_[0]->index();
    if (low <= offset) {
        throw std::runtime_error("cannot process request due to compaction");
    }
    if (high > offset) {
        throw std::runtime_error("entries out of bound last index");
    }
    if (entries_.size() == 1) {
        throw std::runtime_error("unavailable");
    }
}




