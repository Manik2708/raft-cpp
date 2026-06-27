//
// Created by Manik mehta on 10/06/26.
//

#include "progress.h"

namespace trkr {
    void Progress::reset_state(StateType new_state) {
        msg_app_flow_paused = false;
        state = new_state;
        pending_snapshot = 0;
        in_flights->reset();
    }

    void Progress::become_probe() {
        if (state == STATE_SNAPSHOT) {
            reset_state(STATE_PROBE);
            // This means we are recovering from snapshot stage now that means
            // the next entry should be pending_snapshot + 1 but it maybe possible
            // that due to packet losses and other weired things this is a delayed
            // request that means snapshot state is already recovered and match is updated
            // therefore sending pending_snapshot + 1 is waste.
            next = std::ranges::max(match + 1, pending_snapshot + 1);
        } else {
            reset_state(STATE_PROBE);
            next = match + 1;
        }
        sent_commit_ = std::ranges::min(sent_commit_, next - 1);
    }

    void Progress::become_replicate() {
        reset_state(STATE_REPLICATE);
        next = match + 1;
    }

    void Progress::become_snapshot(const uint64_t snapshot_index) {
        reset_state(STATE_SNAPSHOT);
        pending_snapshot = snapshot_index;
        next = snapshot_index + 1;
        sent_commit_ = snapshot_index;
    }

    void Progress::sent_entries(int entries, uint64_t bytes) {
        switch (state) {
            case STATE_REPLICATE:
                if (entries > 0) {
                    next += entries;
                    in_flights->add(next - 1, bytes);
                }
                msg_app_flow_paused = in_flights->full();
                break;
            case STATE_PROBE:
                if (entries > 0) {
                    msg_app_flow_paused = true;
                }
                break;
            default:
                throw std::runtime_error("Unknown state");
        }
    }

    bool Progress::can_bump_commit(const uint64_t index) const {
        return index > sent_commit_ && sent_commit_ < next - 1;
    }

    void Progress::sent_commit(const uint64_t commit) {
        sent_commit_ = commit;
    }

    bool Progress::maybe_update(const uint64_t n) {
        if (n <= match) {
            return false;
        }
        match = n;
        next = std::ranges::max(next, n + 1);
        msg_app_flow_paused = false;
        return true;
    }

    bool Progress::maybe_decr_to(const uint64_t rejected, const uint64_t match_hint) {
        if (state == STATE_REPLICATE) {
            if (rejected <= match) {
                return false;
            }
            next = match + 1;
            sent_commit_ = std::ranges::min(sent_commit_, next - 1);
            return true;
        }
        if (next - 1 != rejected) {
            return false;
        }
        next = std::ranges::max(std::ranges::min(rejected, match_hint + 1), match + 1);
        sent_commit_ = std::ranges::min(sent_commit_, next - 1);
        msg_app_flow_paused = false;
        return true;
    }

    bool Progress::is_paused() const {
        switch (state) {
            case STATE_REPLICATE:
                return msg_app_flow_paused;
            case STATE_PROBE:
                return msg_app_flow_paused;
            case STATE_SNAPSHOT:
                return true;
            default:
                throw std::runtime_error("Unknown state");
        }
    }
}