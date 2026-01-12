/*
 * RADS Constellation - Raft Consensus Implementation
 * Phase 3: Resource Orchestration
 *
 * Simplified Raft for leader election only (no log replication yet)
 */

#define _GNU_SOURCE

#include "raft.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

/*
 * ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static RaftConsensus *g_raft = NULL;
static pthread_mutex_t g_raft_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

uint64_t raft_get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t raft_get_random_election_timeout(void) {
    if (!g_raft) return 0;

    int min_ms = g_raft->config.election_timeout_min_ms;
    int max_ms = g_raft->config.election_timeout_max_ms;

    return min_ms + (rand() % (max_ms - min_ms + 1));
}

void raft_reset_election_timer(void) {
    if (!g_raft) return;

    g_raft->last_heartbeat_time = raft_get_current_time_ms();
    g_raft->election_timeout_ms = raft_get_random_election_timeout();
}

/*
 * ============================================================================
 * STATE TRANSITIONS
 * ============================================================================
 */

void raft_become_follower(uint64_t term) {
    if (!g_raft) return;

    pthread_mutex_lock(&g_raft->state_mutex);

    printf("[RAFT] Becoming FOLLOWER (term %lu)\n", term);

    g_raft->state = RAFT_STATE_FOLLOWER;
    g_raft->persistent.current_term = term;
    memset(g_raft->persistent.voted_for, 0, sizeof(g_raft->persistent.voted_for));
    memset(g_raft->current_leader_id, 0, sizeof(g_raft->current_leader_id));

    // Free leader state if exists
    if (g_raft->leader_state) {
        free(g_raft->leader_state->next_index);
        free(g_raft->leader_state->match_index);
        free(g_raft->leader_state);
        g_raft->leader_state = NULL;
    }

    raft_reset_election_timer();

    pthread_mutex_unlock(&g_raft->state_mutex);
}

void raft_become_candidate(void) {
    if (!g_raft) return;

    pthread_mutex_lock(&g_raft->state_mutex);

    printf("[RAFT] Becoming CANDIDATE (term %lu -> %lu)\n",
           g_raft->persistent.current_term,
           g_raft->persistent.current_term + 1);

    g_raft->state = RAFT_STATE_CANDIDATE;
    g_raft->persistent.current_term++;
    strncpy(g_raft->persistent.voted_for, g_raft->node_id,
            sizeof(g_raft->persistent.voted_for) - 1);

    raft_reset_election_timer();

    pthread_mutex_unlock(&g_raft->state_mutex);
}

void raft_become_leader(void) {
    if (!g_raft) return;

    pthread_mutex_lock(&g_raft->state_mutex);

    printf("[RAFT] Becoming LEADER (term %lu)\n", g_raft->persistent.current_term);

    g_raft->state = RAFT_STATE_LEADER;
    strncpy(g_raft->current_leader_id, g_raft->node_id,
            sizeof(g_raft->current_leader_id) - 1);

    // Initialize leader state
    if (!g_raft->leader_state) {
        g_raft->leader_state = calloc(1, sizeof(RaftLeaderState));
        g_raft->leader_state->peer_count = g_raft->peer_count;

        g_raft->leader_state->next_index = calloc(g_raft->peer_count, sizeof(uint64_t));
        g_raft->leader_state->match_index = calloc(g_raft->peer_count, sizeof(uint64_t));

        // Initialize to leader's last log index + 1
        for (int i = 0; i < g_raft->peer_count; i++) {
            g_raft->leader_state->next_index[i] = g_raft->persistent.log_index + 1;
            g_raft->leader_state->match_index[i] = 0;
        }
    }

    pthread_mutex_unlock(&g_raft->state_mutex);

    // Send immediate heartbeat to assert leadership
    raft_send_heartbeats();
}

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int raft_init(const char *node_id, RaftConfig *config) {
    pthread_mutex_lock(&g_raft_mutex);

    if (g_raft != NULL) {
        pthread_mutex_unlock(&g_raft_mutex);
        return 1; // Already initialized
    }

    g_raft = calloc(1, sizeof(RaftConsensus));
    if (!g_raft) {
        pthread_mutex_unlock(&g_raft_mutex);
        return -1;
    }

    strncpy(g_raft->node_id, node_id, sizeof(g_raft->node_id) - 1);
    g_raft->state = RAFT_STATE_FOLLOWER;

    // Initialize persistent state
    g_raft->persistent.current_term = 0;
    memset(g_raft->persistent.voted_for, 0, sizeof(g_raft->persistent.voted_for));
    g_raft->persistent.log_index = 0;
    g_raft->persistent.log_term = 0;

    // Initialize volatile state
    g_raft->volatile_state.commit_index = 0;
    g_raft->volatile_state.last_applied = 0;

    // No leader initially
    memset(g_raft->current_leader_id, 0, sizeof(g_raft->current_leader_id));

    // Set configuration
    if (config) {
        g_raft->config = *config;
    } else {
        // Default configuration
        g_raft->config.election_timeout_min_ms = 150;
        g_raft->config.election_timeout_max_ms = 300;
        g_raft->config.heartbeat_interval_ms = 50;
        g_raft->config.max_retries = 3;
    }

    // Initialize peers
    g_raft->peers = NULL;
    g_raft->peer_count = 0;

    // Initialize leader state
    g_raft->leader_state = NULL;

    // Initialize timing
    raft_reset_election_timer();
    g_raft->last_election_time = 0;

    // Initialize mutex
    pthread_mutex_init(&g_raft->state_mutex, NULL);

    g_raft->is_running = true;

    // Start background threads
    pthread_create(&g_raft->election_thread, NULL, raft_election_timer_thread, NULL);

    pthread_mutex_unlock(&g_raft_mutex);

    printf("[RAFT] Initialized node %s as FOLLOWER\n", node_id);
    return 0;
}

int raft_shutdown(void) {
    pthread_mutex_lock(&g_raft_mutex);

    if (!g_raft) {
        pthread_mutex_unlock(&g_raft_mutex);
        return -1;
    }

    g_raft->is_running = false;

    // Wait for threads to finish
    pthread_join(g_raft->election_thread, NULL);
    if (g_raft->state == RAFT_STATE_LEADER) {
        pthread_join(g_raft->heartbeat_thread, NULL);
    }

    // Free leader state
    if (g_raft->leader_state) {
        free(g_raft->leader_state->next_index);
        free(g_raft->leader_state->match_index);
        free(g_raft->leader_state);
    }

    // Free peers
    free(g_raft->peers);

    pthread_mutex_destroy(&g_raft->state_mutex);

    free(g_raft);
    g_raft = NULL;

    pthread_mutex_unlock(&g_raft_mutex);

    printf("[RAFT] Shutdown complete\n");
    return 0;
}

bool raft_is_running(void) {
    pthread_mutex_lock(&g_raft_mutex);
    bool running = (g_raft != NULL && g_raft->is_running);
    pthread_mutex_unlock(&g_raft_mutex);
    return running;
}

/*
 * ============================================================================
 * STATE QUERIES
 * ============================================================================
 */

RaftState raft_get_state(void) {
    if (!g_raft) return RAFT_STATE_FOLLOWER;

    pthread_mutex_lock(&g_raft->state_mutex);
    RaftState state = g_raft->state;
    pthread_mutex_unlock(&g_raft->state_mutex);

    return state;
}

bool raft_is_leader(void) {
    return raft_get_state() == RAFT_STATE_LEADER;
}

const char *raft_get_leader_id(void) {
    if (!g_raft) return NULL;

    pthread_mutex_lock(&g_raft->state_mutex);
    static char leader_id[64];
    strncpy(leader_id, g_raft->current_leader_id, sizeof(leader_id) - 1);
    pthread_mutex_unlock(&g_raft->state_mutex);

    return leader_id[0] != '\0' ? leader_id : NULL;
}

uint64_t raft_get_current_term(void) {
    if (!g_raft) return 0;

    pthread_mutex_lock(&g_raft->state_mutex);
    uint64_t term = g_raft->persistent.current_term;
    pthread_mutex_unlock(&g_raft->state_mutex);

    return term;
}

/*
 * ============================================================================
 * CLUSTER MEMBERSHIP
 * ============================================================================
 */

int raft_add_peer(Node *peer) {
    if (!g_raft || !peer) return -1;

    pthread_mutex_lock(&g_raft->state_mutex);

    g_raft->peers = realloc(g_raft->peers, sizeof(Node*) * (g_raft->peer_count + 1));
    g_raft->peers[g_raft->peer_count] = peer;
    g_raft->peer_count++;

    pthread_mutex_unlock(&g_raft->state_mutex);

    printf("[RAFT] Added peer %s (total: %d)\n", peer->name, g_raft->peer_count);
    return 0;
}

int raft_remove_peer(const char *peer_id) {
    if (!g_raft || !peer_id) return -1;

    pthread_mutex_lock(&g_raft->state_mutex);

    for (int i = 0; i < g_raft->peer_count; i++) {
        if (strcmp(g_raft->peers[i]->id, peer_id) == 0) {
            // Shift array
            for (int j = i; j < g_raft->peer_count - 1; j++) {
                g_raft->peers[j] = g_raft->peers[j + 1];
            }
            g_raft->peer_count--;

            pthread_mutex_unlock(&g_raft->state_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_raft->state_mutex);
    return -1;
}

/*
 * ============================================================================
 * VOTING LOGIC
 * ============================================================================
 */

bool raft_should_grant_vote(RequestVoteRequest *request) {
    if (!g_raft || !request) return false;

    pthread_mutex_lock(&g_raft->state_mutex);

    // If candidate's term is less than ours, reject
    if (request->term < g_raft->persistent.current_term) {
        pthread_mutex_unlock(&g_raft->state_mutex);
        return false;
    }

    // If we already voted for someone else in this term, reject
    if (g_raft->persistent.voted_for[0] != '\0' &&
        strcmp(g_raft->persistent.voted_for, request->candidate_id) != 0) {
        pthread_mutex_unlock(&g_raft->state_mutex);
        return false;
    }

    // Check if candidate's log is at least as up-to-date as ours
    bool log_ok = (request->last_log_term > g_raft->persistent.log_term) ||
                  (request->last_log_term == g_raft->persistent.log_term &&
                   request->last_log_index >= g_raft->persistent.log_index);

    pthread_mutex_unlock(&g_raft->state_mutex);

    return log_ok;
}

RequestVoteReply raft_handle_request_vote(RequestVoteRequest *request) {
    RequestVoteReply reply;
    reply.vote_granted = false;
    reply.term = 0;

    if (!g_raft || !request) return reply;

    pthread_mutex_lock(&g_raft->state_mutex);

    // If candidate term is greater, step down to follower
    if (request->term > g_raft->persistent.current_term) {
        pthread_mutex_unlock(&g_raft->state_mutex);
        raft_become_follower(request->term);
        pthread_mutex_lock(&g_raft->state_mutex);
    }

    reply.term = g_raft->persistent.current_term;

    // Grant vote if appropriate
    if (raft_should_grant_vote(request)) {
        reply.vote_granted = true;
        strncpy(g_raft->persistent.voted_for, request->candidate_id,
                sizeof(g_raft->persistent.voted_for) - 1);
        raft_reset_election_timer();

        printf("[RAFT] Granted vote to %s (term %lu)\n",
               request->candidate_id, request->term);
    }

    pthread_mutex_unlock(&g_raft->state_mutex);

    return reply;
}

int raft_request_votes_from_peers(void) {
    if (!g_raft) return -1;

    pthread_mutex_lock(&g_raft->state_mutex);

    int votes_received = 1; // Vote for ourselves

    RequestVoteRequest request;
    request.term = g_raft->persistent.current_term;
    strncpy(request.candidate_id, g_raft->node_id, sizeof(request.candidate_id) - 1);
    request.last_log_index = g_raft->persistent.log_index;
    request.last_log_term = g_raft->persistent.log_term;

    // Request votes from all peers (simplified - would be async in real impl)
    for (int i = 0; i < g_raft->peer_count; i++) {
        // TODO: Send actual RPC to peer
        // For now, simulate random votes for testing
        if (rand() % 2 == 0) {
            votes_received++;
        }
    }

    pthread_mutex_unlock(&g_raft->state_mutex);

    return votes_received;
}

/*
 * ============================================================================
 * ELECTION
 * ============================================================================
 */

int raft_start_election(void) {
    if (!g_raft) return -1;

    printf("[RAFT] Starting election\n");

    // Transition to candidate
    raft_become_candidate();

    g_raft->last_election_time = raft_get_current_time_ms();

    // Request votes from peers
    int votes_received = raft_request_votes_from_peers();
    int majority = (g_raft->peer_count + 1) / 2 + 1;

    printf("[RAFT] Received %d votes, need %d for majority\n", votes_received, majority);

    // If we got majority, become leader
    if (votes_received >= majority) {
        raft_become_leader();
        return 1; // Won election
    }

    return 0; // Didn't win
}

/*
 * ============================================================================
 * HEARTBEATS
 * ============================================================================
 */

HeartbeatReply raft_handle_heartbeat(HeartbeatRequest *request) {
    HeartbeatReply reply;
    reply.success = false;
    reply.term = 0;

    if (!g_raft || !request) return reply;

    pthread_mutex_lock(&g_raft->state_mutex);

    // If leader's term is greater, step down to follower
    if (request->term > g_raft->persistent.current_term) {
        pthread_mutex_unlock(&g_raft->state_mutex);
        raft_become_follower(request->term);
        pthread_mutex_lock(&g_raft->state_mutex);
    }

    reply.term = g_raft->persistent.current_term;

    // If term matches, accept heartbeat
    if (request->term == g_raft->persistent.current_term) {
        reply.success = true;

        // Update current leader
        strncpy(g_raft->current_leader_id, request->leader_id,
                sizeof(g_raft->current_leader_id) - 1);

        // Reset election timer
        raft_reset_election_timer();

        // Step down to follower if we were candidate
        if (g_raft->state == RAFT_STATE_CANDIDATE) {
            pthread_mutex_unlock(&g_raft->state_mutex);
            raft_become_follower(request->term);
            return reply;
        }
    }

    pthread_mutex_unlock(&g_raft->state_mutex);

    return reply;
}

int raft_send_heartbeats(void) {
    if (!g_raft || g_raft->state != RAFT_STATE_LEADER) return -1;

    pthread_mutex_lock(&g_raft->state_mutex);

    HeartbeatRequest request;
    request.term = g_raft->persistent.current_term;
    strncpy(request.leader_id, g_raft->node_id, sizeof(request.leader_id) - 1);
    request.prev_log_index = g_raft->persistent.log_index;
    request.prev_log_term = g_raft->persistent.log_term;
    request.leader_commit = g_raft->volatile_state.commit_index;

    // Send to all peers (simplified - would be async in real impl)
    for (int i = 0; i < g_raft->peer_count; i++) {
        // TODO: Send actual RPC to peer
    }

    pthread_mutex_unlock(&g_raft->state_mutex);

    return 0;
}

/*
 * ============================================================================
 * BACKGROUND THREADS
 * ============================================================================
 */

void *raft_election_timer_thread(void *arg) {
    (void)arg;

    printf("[RAFT] Election timer thread started\n");

    while (g_raft && g_raft->is_running) {
        usleep(50000); // Check every 50ms

        if (!g_raft || !g_raft->is_running) break;

        // Only check election timeout if we're follower or candidate
        if (g_raft->state == RAFT_STATE_LEADER) {
            continue;
        }

        uint64_t now = raft_get_current_time_ms();
        uint64_t elapsed = now - g_raft->last_heartbeat_time;

        // If election timeout elapsed, start election
        if (elapsed >= g_raft->election_timeout_ms) {
            printf("[RAFT] Election timeout! Starting election...\n");
            raft_start_election();
        }
    }

    printf("[RAFT] Election timer thread stopped\n");
    return NULL;
}

void *raft_heartbeat_thread(void *arg) {
    (void)arg;

    printf("[RAFT] Heartbeat thread started\n");

    while (g_raft && g_raft->is_running && g_raft->state == RAFT_STATE_LEADER) {
        // Send heartbeats
        raft_send_heartbeats();

        // Sleep for heartbeat interval
        usleep(g_raft->config.heartbeat_interval_ms * 1000);
    }

    printf("[RAFT] Heartbeat thread stopped\n");
    return NULL;
}

/*
 * ============================================================================
 * PERSISTENCE (Simplified - in-memory only)
 * ============================================================================
 */

int raft_save_state(void) {
    // TODO: Persist to disk
    return 0;
}

int raft_load_state(void) {
    // TODO: Load from disk
    return 0;
}
