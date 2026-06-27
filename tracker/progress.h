//
// Created by Manik mehta on 10/06/26.
//

#ifndef RAFT_CPP_PROGRESS_H
#define RAFT_CPP_PROGRESS_H

#include <memory>
#include "inflights.h"

namespace trkr {
    enum StateType {
        STATE_PROBE,
        STATE_REPLICATE,
        STATE_SNAPSHOT
    };

    class Progress {
        uint64_t sent_commit_ = 0;
    public:
        uint64_t match = 0;
        uint64_t next = 0;
        StateType state;
        uint64_t pending_snapshot = 0;
        bool recent_active = false;
        bool msg_app_flow_paused = false;
        bool is_learner = false;
        std::unique_ptr<InFlights> in_flights;

        void reset_state(StateType new_state);
        void become_probe();
        void become_replicate();
        void become_snapshot(uint64_t snapshot_index);
        void sent_entries(int entries, uint64_t bytes);
        bool can_bump_commit(uint64_t index) const;
        void sent_commit(uint64_t commit);
        bool maybe_update(uint64_t n);
        bool maybe_decr_to(uint64_t rejected, uint64_t match_hint);
        bool is_paused() const;
    };
}

#endif //RAFT_CPP_PROGRESS_H
