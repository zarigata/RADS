/*
 * RADS Constellation - Raft Consensus Header
 * Phase 3: Resource Orchestration
 *
 * Simplified Raft consensus for leader election
 * Based on "In Search of an Understandable Consensus Algorithm" (Ongaro & Ousterhout, 2014)
 */

#ifndef CONSTELLATION_RAFT_H
#define CONSTELLATION_RAFT_H

#include "../constellation.h"
#include "../cluster/cluster.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/*
 * ============================================================================
 * RAFT TYPES
 * ============================================================================
 */

// Raft node states
typedef enum {
    RAFT_STATE_FOLLOWER,         // Following leader
    RAFT_STATE_CANDIDATE,        // Campaigning for leadership
    RAFT_STATE_LEADER            // Elected leader
} RaftState;

// Raft RPC types
typedef enum {
    RAFT_RPC_REQUEST_VOTE,       // Request vote from peers
    RAFT_RPC_REQUEST_VOTE_REPLY, // Vote response
    RAFT_RPC_HEARTBEAT,          // Leader heartbeat
    RAFT_RPC_HEARTBEAT_REPLY     // Heartbeat response
} RaftRPCType;

// RequestVote RPC
typedef struct {
    uint64_t term;               // Candidate's term
    char candidate_id[64];       // Candidate requesting vote
    uint64_t last_log_index;     // Index of candidate's last log entry
    uint64_t last_log_term;      // Term of candidate's last log entry
} RequestVoteRequest;

typedef struct {
    uint64_t term;               // Current term, for candidate to update itself
    bool vote_granted;           // True means candidate received vote
} RequestVoteReply;

// Heartbeat RPC (simplified AppendEntries)
typedef struct {
    uint64_t term;               // Leader's term
    char leader_id[64];          // So follower can redirect clients
    uint64_t prev_log_index;     // Index of log entry immediately preceding new ones
    uint64_t prev_log_term;      // Term of prevLogIndex entry
    uint64_t leader_commit;      // Leader's commitIndex
} HeartbeatRequest;

typedef struct {
    uint64_t term;               // Current term, for leader to update itself
    bool success;                // True if follower contained entry matching prevLogIndex and prevLogTerm
} HeartbeatReply;

// Raft configuration
typedef struct {
    int election_timeout_min_ms; // Min election timeout (default: 150ms)
    int election_timeout_max_ms; // Max election timeout (default: 300ms)
    int heartbeat_interval_ms;   // Leader heartbeat interval (default: 50ms)
    int max_retries;             // Max RPC retries (default: 3)
} RaftConfig;

// Raft persistent state (on all servers)
typedef struct {
    uint64_t current_term;       // Latest term server has seen
    char voted_for[64];          // CandidateId that received vote in current term (or NULL)
    uint64_t log_index;          // Current log index (simplified - no log entries in this impl)
    uint64_t log_term;           // Term of last log entry
} RaftPersistentState;

// Raft volatile state (on all servers)
typedef struct {
    uint64_t commit_index;       // Index of highest log entry known to be committed
    uint64_t last_applied;       // Index of highest log entry applied to state machine
} RaftVolatileState;

// Raft leader state (only on leaders)
typedef struct {
    uint64_t *next_index;        // For each server, index of next log entry to send
    uint64_t *match_index;       // For each server, index of highest log entry known to be replicated
    int peer_count;              // Number of peers
} RaftLeaderState;

// Main Raft state
typedef struct {
    char node_id[64];            // This node's ID
    RaftState state;             // Current state

    RaftPersistentState persistent; // Persistent state
    RaftVolatileState volatile_state; // Volatile state
    RaftLeaderState *leader_state;   // Leader state (NULL if not leader)

    char current_leader_id[64];  // Current leader ID (empty if unknown)

    // Timing
    uint64_t last_heartbeat_time; // Last time we received heartbeat from leader
    uint64_t election_timeout_ms; // Current election timeout (randomized)
    uint64_t last_election_time;  // Last time we started an election

    // Configuration
    RaftConfig config;

    // Cluster nodes
    Node **peers;                // Peer nodes
    int peer_count;              // Number of peers

    // Running state
    bool is_running;
    pthread_t election_thread;   // Election timer thread
    pthread_t heartbeat_thread;  // Leader heartbeat thread
    pthread_mutex_t state_mutex; // Protects raft state
} RaftConsensus;

/*
 * ============================================================================
 * RAFT API
 * ============================================================================
 */

// Lifecycle
int raft_init(const char *node_id, RaftConfig *config);
int raft_shutdown(void);
bool raft_is_running(void);

// State queries
RaftState raft_get_state(void);
bool raft_is_leader(void);
const char *raft_get_leader_id(void);
uint64_t raft_get_current_term(void);

// Cluster membership
int raft_add_peer(Node *peer);
int raft_remove_peer(const char *peer_id);

// RPC handlers (called when receiving messages)
RequestVoteReply raft_handle_request_vote(RequestVoteRequest *request);
HeartbeatReply raft_handle_heartbeat(HeartbeatRequest *request);

// Campaign for leadership (called internally or on demand)
int raft_start_election(void);

// Leader operations
int raft_send_heartbeats(void);

/*
 * ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

// State transitions
void raft_become_follower(uint64_t term);
void raft_become_candidate(void);
void raft_become_leader(void);

// Voting
bool raft_should_grant_vote(RequestVoteRequest *request);
int raft_request_votes_from_peers(void);

// Timing
void raft_reset_election_timer(void);
uint64_t raft_get_random_election_timeout(void);
uint64_t raft_get_current_time_ms(void);

// Background threads
void *raft_election_timer_thread(void *arg);
void *raft_heartbeat_thread(void *arg);

// Persistence (simplified - in-memory only for now)
int raft_save_state(void);
int raft_load_state(void);

#endif /* CONSTELLATION_RAFT_H */
