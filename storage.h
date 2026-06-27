#ifndef RAFT_CPP_STORAGE_H
#define RAFT_CPP_STORAGE_H

#include <vector>
#include <memory>
#include <string>
#include "raftpb/raft.pb.h"
#include <shared_mutex>

struct StateInfo {
    std::unique_ptr<raftpb::HardState> hard_state;
    std::unique_ptr<raftpb::ConfState> conf_state;
};

class Storage {
public:
    virtual ~Storage() = default;
    virtual StateInfo InitialState() = 0;
    virtual std::vector<std::unique_ptr<raftpb::Entry>> Entries(uint64_t low, uint64_t high) = 0;
    virtual uint64_t Term() = 0;
    virtual uint64_t LastIndex() = 0;
    virtual uint64_t FirstIndex() = 0;
    virtual std::unique_ptr<raftpb::Snapshot> Snapshot() = 0;
};

struct inMemStorageCallStats {
    int initial_state = 0;
    int first_index = 0;
    int last_index = 0;
    int entries = 0;
    int term = 0;
    int snapshot = 0;
};

class MemoryStorage : public Storage {
    std::shared_mutex mu_;
    std::unique_ptr<raftpb::HardState> hard_state_;
    std::unique_ptr<raftpb::Snapshot> snapshot_;
    std::vector<std::unique_ptr<raftpb::Entry>> entries_;
    inMemStorageCallStats call_stats_;
public:
    MemoryStorage();
    StateInfo InitialState() override;
    std::vector<std::unique_ptr<raftpb::Entry>> Entries(uint64_t low, uint64_t high) override;
    uint64_t Term() override;
    uint64_t LastIndex() override;
    uint64_t FirstIndex() override;
    std::unique_ptr<raftpb::Snapshot> Snapshot() override;
    void SetHardState(std::unique_ptr<raftpb::HardState> hard_state);
    void SetSnapshot(std::unique_ptr<raftpb::Snapshot> snapshot);
    std::unique_ptr<raftpb::Snapshot> CreateSnapshot(uint64_t i, std::unique_ptr<raftpb::ConfState> conf_state, std::unique_ptr<std::string> data);
    void Compact(uint64_t index);
    void Append(std::vector<std::unique_ptr<raftpb::Entry>>& entries);
};


#endif //RAFT_CPP_STORAGE_H
