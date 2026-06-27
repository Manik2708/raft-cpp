//
// Created by Manik mehta on 05/06/26.
//

#ifndef RAFT_CPP_RAFT_H
#define RAFT_CPP_RAFT_H
#include "storage.h"
#include "log.h"
#include "utils.h"
#include "tracker/tracker.h"

namespace raft {
    class Raft;
    using step_methods = void(*)(Raft& r, const raftpb::Message& msg);
    using tick_methods = void(*)(Raft& r);

    inline ThreadSafeRandom global_random;
    static constexpr uint64_t None = 0;

    enum State {
        STATE_FOLLOWER,
        STATE_LEADER,
        STATE_PRECANDIDATE,
        STATE_CANDIDATE
    };

    enum CampaignType {
        CAMPAIGN_PRE_ELECTION,
        CAMPAIGN_ELECTION,
        CAMPAIGN_TRANSFER
    };

    struct ReadIndex {
        uint64_t index;
        std::string data;
    };

    struct PollResult {
        int granted;
        int rejected;
        quorum::VoteResult result;
    };

    class Raft {
        uint64_t leader_id_;
        uint64_t id_;
        // election_lapsed_ is number of ticks since it reached last electionTimeout
        // election is called if it doesn't listen within election_timeout. The edge case
        // when it receives request to vote within election timeout, it neither increases
        // the term and nor grants the vote.
        int election_lapsed_;
        int heartbeat_lapsed_;
        int election_timeout_;
        int randomise_election_timeout_;
        std::unique_ptr<raftlog::raftLog> raft_log_;
        std::unique_ptr<trkr::ProgressTracker> trkr_;
        State state_;
        step_methods step_;
        tick_methods tick_;
        std::vector<std::unique_ptr<ReadIndex>> read_states_;
        uint64_t term_;
        uint64_t vote_;

        void become_follower(uint64_t new_term, uint64_t leader_id);
        void reset(uint64_t new_term);
        void reset_randomized_election_timeout();
        void send(const raftpb::Message& msg);
        void applied_snap(const raftpb::Snapshot& snapshot);
        void hup(CampaignType type);
        void applied_to(uint64_t index, uint64_t size);
        void reduce_uncommitted_size(uint64_t size);
        void handle_append_entries(const raftpb::Message& msg);
        void handle_heart_beat(const raftpb::Message& msg);
        void handle_snapshot(const raftpb::Message& msg);
        PollResult poll(uint64_t id, raftpb::MessageType type, bool vote);
        void become_leader();
        void bcast_append();
        void bcast_heartbeat();
        bool append_entries(const raftpb::Entry& entry);
        void send_append(uint64_t to);

        friend void step_follower(Raft& r, const raftpb::Message& msg);
        friend void step_candidate(Raft& r, const raftpb::Message& msg);
        friend void step_leader(Raft& r, const raftpb::Message& msg);
        friend void tick_election(Raft& r);
    public:
        Raft();
        void step(const raftpb::Message& message);
    };

    void step_follower(Raft& r, const raftpb::Message& msg);
    void step_candidate(Raft& r, const raftpb::Message& msg);
    void step_leader(Raft& r, const raftpb::Message& msg);

    void tick_election(Raft& r);
}
#endif //RAFT_CPP_RAFT_H
