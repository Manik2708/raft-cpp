//
// Created by Manik mehta on 05/06/26.
//

#include "raft.h"

namespace raft {
    Raft::Raft() :
    term_(0),
    vote_(None) {}

    raftpb::MessageType voteRespMsgType (raftpb::MessageType type) {
        switch (type) {
            case raftpb::MsgPreVote:
                return raftpb::MsgPreVoteResp;
            case raftpb::MsgVote:
                return raftpb::MsgVoteResp;
            default:
                throw std::runtime_error("unkown type");
        }
    }

    void Raft::step(const raftpb::Message& message) {
        const uint64_t type = message.type();
        if (message.term() == 0) {
            // local message
        } else if (message.term() > term_) {
            // Means another server has higher term, so it maybe the leader, therefore
            // this block handles the messages from leader
            if (type == raftpb::MsgVote || type == raftpb::MsgPreVote) {
                const bool force = message.context() == "CAMPAIGNTRANSFER";
                if (const bool inLease =  leader_id_ != None && election_lapsed_ < election_timeout_; !force && inLease) {
                    // This means server has received request to Vote within minimum election timeout
                    // and leader is always checking its quorum, so we can definitely accept it as leader
                    return;
                }
            }
            if (message.type() == raftpb::MsgPreVote) {
                // Never change the term in PreVote, and it is a pre-voting request. therefore
                // we should not step from leadership
            } else if (message.type() == raftpb::MsgPreVoteResp && !message.reject()) {
                // If a node hasn't rejected the pre-vote
            } else {
                if (type == raftpb::MsgApp || type == raftpb::MsgHeartbeat || type == raftpb::MsgSnap) {
                    become_follower(message.term(), message.from());
                } else {
                    // In the case of pre-vote response and rejection we don't know the leader.
                    become_follower(message.term(), None);
                }
            }
        } else if (message.term() < term_) {
            // This means this node is leader
            if (type == raftpb::MsgHeartbeat || type == raftpb::MsgApp) {
                // Now the current raft term is higher than the term we received.
                // This can be possible in two ways, either the message is delayed or
                // a server which is not the actual leader is thinking itself leader.
                // This maybe possible when the leader gets isolated, because the natural
                // behaviour of raft is to conduct a pre-election before an actual election
                // and also the leader always check for the quorum before sending a heartbeat
                // so ideally this is a hypothetical case but in response of this we can send
                // a response message with higher term so it forces that leader to become follower.
                auto msg = raftpb::Message();
                msg.set_to(message.from());
                msg.set_type(raftpb::MsgAppResp);
                send(msg);
            } else if (type == raftpb::MsgPreVote) {
                // We should reject this so that it returns to follower stage
                auto msg = raftpb::Message();
                msg.set_type(raftpb::MsgPreVoteResp);
                msg.set_reject(true);
                msg.set_to(message.from());
                msg.set_term(term_);
                send(msg);
            } else if (type == raftpb::MsgStorageAppendResp) {
                // Don't append messages because message might have been overwritten
                // in the latter term
                if (message.snapshot().has_data()) {
                    applied_snap(message.snapshot());
                }
            }
            return;
        }
        // When m.term = term_
        switch (type) {
            case raftpb::MsgHup:
                hup(CAMPAIGN_PRE_ELECTION);
                break;
            case raftpb::MsgStorageAppendResp:
                if (message.index() != 0) {
                    raft_log_->stable_to({message.logterm(), message.index()});
                }
                if (message.snapshot().has_data()) {
                    applied_snap(message.snapshot());
                }
                break;
            case raftpb::MsgStorageApplyResp:
                if (message.entries().size() > 0) {
                    // TODO: Complete this fxn
                }
                break;
            case raftpb::MsgVote:
            case raftpb::MsgPreVote: {
                const bool can_vote = vote_ == message.from()
                || (vote_ == None && leader_id_ != None)
                || (type == raftpb::MsgPreVote && message.term() > term_);
                if (const auto candidate_last_id = raftlog::EntryId{message.logterm(), message.index()}; can_vote && raft_log_->is_up_to_date(candidate_last_id)) {
                    auto msg = raftpb::Message();
                    msg.set_term(message.term());
                    msg.set_to(message.from());
                    msg.set_type(voteRespMsgType(message.type()));
                    send(msg);
                    if (message.type() == raftpb::MsgVote) {
                        election_lapsed_ = 0;
                        vote_ = message.from();
                    }
                } else {
                    auto msg = raftpb::Message();
                    msg.set_term(term_);
                    msg.set_to(message.from());
                    msg.set_type(voteRespMsgType(message.type()));
                    msg.set_reject(true);
                    send(msg);
                }
                break;
            }
            default:
                step_(*this, message);
        }
    }

    void Raft::become_follower(const uint64_t new_term, const uint64_t leader_id) {
        step_ = step_follower;
        reset(new_term);
        tick_ = tick_election;
        state_ = STATE_FOLLOWER;
        leader_id_ = leader_id;
    }

    void Raft::reset(const uint64_t new_term) {
        if (term_ != new_term) {
            term_ = new_term;
            vote_ = None;
        }
        leader_id_ = None;
        election_lapsed_ = 0;
        heartbeat_lapsed_ = 0;
        reset_randomized_election_timeout();
    }

    void Raft::reset_randomized_election_timeout() {
        randomise_election_timeout_ = election_timeout_ + global_random.Intn(election_timeout_);
    }

    void step_follower(Raft& r, const raftpb::Message &msg) {
        switch (msg.type()) {
            case raftpb::MsgProp: {
                if (r.leader_id_ == None) {
                    throw std::logic_error("no leader, dropping proposal");
                }
                auto message = raftpb::Message();
                message.set_to(r.leader_id_);
                r.send(msg);
                break;
            }
            case raftpb::MsgApp: {
                r.election_lapsed_ = 0;
                r.leader_id_ = msg.from();
                r.handle_append_entries(msg);
                break;
            }
            case raftpb::MsgHeartbeat: {
                r.election_lapsed_ = 0;
                r.leader_id_ = msg.from();
                r.handle_heart_beat(msg);
                break;
            }
            case raftpb::MsgSnap: {
                r.election_lapsed_ = 0;
                r.leader_id_ = msg.from();
                r.handle_snapshot(msg);
                break;
            }
            case raftpb::MsgTransferLeader: {
                if (r.leader_id_ == None) {
                    return;
                }
                raftpb::Message message = msg;
                message.set_to(r.leader_id_);
                r.send(message);
                break;
            }
            case raftpb::MsgForgetLeader: {
                if (r.leader_id_ != None) {
                    r.leader_id_ = None;
                }
                break;
            }
            case raftpb::MsgTimeoutNow: {
                r.hup(CAMPAIGN_TRANSFER);
                break;
            }
            case raftpb::MsgReadIndex: {
                if (r.leader_id_ == None) {
                    return;
                }
                raftpb::Message message = msg;
                message.set_to(r.leader_id_);
                r.send(message);
                break;
            }
            case raftpb::MsgReadIndexResp: {
                if (msg.entries().size() != 1) {
                    return;
                }
                r.read_states_.push_back(std::make_unique<ReadIndex>(ReadIndex{msg.index(), msg.entries()[0].data()}));
            }
            default: break;
        }
    }

    void step_candidate(Raft& r, const raftpb::Message& msg) {
        raftpb::MessageType my_vote_resp;
        if (r.state_ == STATE_PRECANDIDATE) {
            my_vote_resp = raftpb::MsgPreVoteResp;
        } else {
            my_vote_resp = raftpb::MsgVoteResp;
        }
        switch (msg.type()) {
            case raftpb::MsgProp: {
                throw std::logic_error("not implemented");
            }
            case raftpb::MsgApp: {
                r.become_follower(msg.term(), msg.from());
                r.handle_append_entries(msg);
                break;
            }
            case raftpb::MsgHeartbeat: {
                r.become_follower(msg.term(), msg.from());
                r.handle_heart_beat(msg);
                break;
            }
            case raftpb::MsgSnap: {
                r.become_follower(msg.term(), msg.from());
                r.handle_snapshot(msg);
                break;
            }
            case raftpb::MsgPreVoteResp:
            case raftpb::MsgVoteResp: {
                switch (const auto res = r.poll(msg.from(), msg.type(), !msg.reject()); res.result) {
                    case quorum::VOTE_WON:
                        if (r.state_ == STATE_PRECANDIDATE) {
                            r.hup(CAMPAIGN_ELECTION);
                        } else {
                            r.become_leader();
                            r.bcast_append();
                        }
                    case quorum::VOTE_LOST:
                        r.become_follower(msg.term(), None);
                    default: break;
                }
            }
            default: break;
        }
    }

    void step_leader(Raft& r, const raftpb::Message& msg) {
        switch (msg.type()) {
            case raftpb::MsgBeat: {
                r.bcast_heartbeat();
                return;
            }
            case raftpb::MsgCheckQuorum: {
                if (!r.trkr_->quorum_active()) {
                    r.become_follower(r.term_, None);
                }
                r.trkr_->visit([&] (const uint64_t id, trkr::Progress* pr) -> void {
                    if (id != r.id_) {
                        pr->recent_active = false;
                    }
                });
                break;
            }
            case raftpb::MsgProp: {
                if (msg.entries().empty()) {
                    throw std::runtime_error("proposal entries are empty");
                }
                for (const auto& entry : msg.entries()) {
                    if (!r.append_entries(entry)) {
                        throw std::runtime_error("proposal dropped");
                    }
                }
                r.bcast_append();
                break;
            }
            case raftpb::MsgReadIndex: {
                // TODO: Implement, currently skipping
                break;
            }
            case raftpb::MsgForgetLeader: {
                break;
            }
            default: {
                if (!r.trkr_->progress.contains(msg.from())) {
                    return;
                }
                trkr::Progress* pr = r.trkr_->progress.at(msg.from()).get();
                switch (msg.type()) {
                    case raftpb::MsgAppResp: {
                        if (msg.reject()) {
                            uint64_t next_probe_index = msg.rejecthint();
                            if (msg.logterm() > 0) {
                                const auto tp = r.raft_log_->find_conflict_by_term(msg.rejecthint(), msg.logterm());
                                next_probe_index = std::get<0>(tp);
                            }
                            if (pr->maybe_decr_to(msg.index(), next_probe_index)) {
                                if (pr->state == trkr::STATE_REPLICATE) {
                                    pr->become_probe();
                                }
                            }
                            r.send_append(msg.from());
                        } else {

                        }
                    }
                    default: break;
                }
            }
        }
    }
}