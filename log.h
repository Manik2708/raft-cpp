//
// Created by Manik mehta on 13/06/26.
//

#ifndef RAFT_CPP_LOG_H
#define RAFT_CPP_LOG_H
#include "storage.h"

namespace raftlog {
    struct EntryId {
        uint64_t term;
        uint64_t index;
    };

    class Unstable {
        std::unique_ptr<raftpb::Snapshot> snapshot_;
        std::vector<std::unique_ptr<raftpb::Entry>> entries_;
        uint64_t offset_;
        bool snapshot_in_progress_;
        bool offset_in_progress_;
    };

    class raftLog {
        std::unique_ptr<Storage> storage_;
        Unstable unstable_;
        uint64_t committed_;
        uint64_t applying_;
        uint64_t max_applied_ents_size;
        uint64_t applying_ents_size;
        bool applying_ents_paused;
    public:
        void stable_to(EntryId id);
        EntryId last_entry_id();
        bool is_up_to_date(EntryId their);
        std::tuple<uint64_t, uint64_t> find_conflict_by_term(uint64_t index, uint64_t term);
    };
}

#endif //RAFT_CPP_LOG_H
