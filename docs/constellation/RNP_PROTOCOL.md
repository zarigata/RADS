# 🌐 RNP - RADS Native Protocol Specification

**Version:** 1.0.0-draft
**Status:** Specification
**Last Updated:** January 12, 2026

---

## 📋 Table of Contents

1. [Overview](#overview)
2. [Design Goals](#design-goals)
3. [Protocol Stack](#protocol-stack)
4. [Message Format](#message-format)
5. [Message Types](#message-types)
6. [Connection Management](#connection-management)
7. [Security](#security)
8. [Error Handling](#error-handling)
9. [Performance](#performance)
10. [Implementation Notes](#implementation-notes)

---

## Overview

**RNP (RADS Native Protocol)** is the communication protocol used by RADS Constellation nodes to:

- Discover and communicate with other nodes
- Synchronize cluster state
- Transfer instance data
- Share resource information
- Coordinate distributed operations
- Stream logs and metrics

### Key Characteristics

- **Binary Protocol:** Efficient MessagePack encoding
- **Bi-directional:** Full-duplex communication
- **Encrypted:** TLS 1.3 by default
- **Low Latency:** Optimized for < 1ms within datacenter
- **Versioned:** Forward/backward compatibility
- **Extensible:** Plugin system for custom messages

---

## Design Goals

### Primary Goals

1. **Performance**
   - Sub-millisecond latency for small messages
   - High throughput for bulk data transfer
   - Minimal CPU overhead (< 5%)
   - Efficient memory usage

2. **Reliability**
   - Message delivery guarantees (at-most-once, at-least-once, exactly-once)
   - Automatic retries with exponential backoff
   - Connection recovery
   - Message ordering preservation

3. **Security**
   - Encrypted by default (TLS 1.3)
   - Node authentication (mutual TLS)
   - Message integrity (HMAC)
   - Protection against replay attacks

4. **Simplicity**
   - Clean, understandable protocol
   - Easy to implement
   - Easy to debug
   - Comprehensive error messages

### Non-Goals

- Not HTTP-compatible (custom binary protocol)
- Not human-readable (optimized for machines)
- Not connectionless (TCP-based, persistent connections)

---

## Protocol Stack

```
┌─────────────────────────────────────┐
│     Application Layer               │  RADS Constellation
│  (RNP Messages: Gossip, Sync, etc.) │
├─────────────────────────────────────┤
│        RNP Protocol Layer           │  Message framing,
│   (Framing, Compression, Encoding)  │  serialization
├─────────────────────────────────────┤
│       Security Layer (TLS 1.3)      │  Encryption,
│                                     │  authentication
├─────────────────────────────────────┤
│      Transport Layer (TCP)          │  Reliable delivery
├─────────────────────────────────────┤
│      Network Layer (IP)             │  Routing
└─────────────────────────────────────┘
```

### Layer Responsibilities

**Application Layer:**
- Constellation-specific logic
- Message types (gossip, sync, heartbeat, etc.)
- Business logic

**RNP Protocol Layer:**
- Message framing (length prefix)
- Serialization (MessagePack)
- Compression (optional LZ4)
- Versioning

**Security Layer:**
- TLS 1.3 encryption
- Mutual authentication
- Certificate validation

**Transport Layer:**
- TCP connection management
- Flow control
- Congestion control

---

## Message Format

### Frame Structure

Every RNP message is transmitted as a frame:

```
┌──────────────────────────────────────────────────────────┐
│                        RNP FRAME                          │
├──────────┬────────────┬──────────┬──────────┬────────────┤
│  Magic   │  Version   │  Length  │  Flags   │  Payload   │
│ (4 bytes)│  (2 bytes) │(4 bytes) │(2 bytes) │  (N bytes) │
└──────────┴────────────┴──────────┴──────────┴────────────┘
```

**Field Descriptions:**

1. **Magic (4 bytes):** Protocol identifier
   - Value: `0x52 0x4E 0x50 0x01` ("RNP\x01")
   - Validates frame start
   - Detects protocol mismatches

2. **Version (2 bytes):** Protocol version
   - Format: Major.Minor (1 byte each)
   - Current: `0x01 0x00` (v1.0)
   - Enables version negotiation

3. **Length (4 bytes):** Payload size
   - Unsigned 32-bit integer (big-endian)
   - Maximum: 16MB (16,777,216 bytes)
   - Does not include header size

4. **Flags (2 bytes):** Message flags
   - Bit 0: Compressed (0=no, 1=yes)
   - Bit 1: Encrypted (0=no, 1=yes)
   - Bit 2: Requires ACK (0=no, 1=yes)
   - Bit 3: Is ACK (0=no, 1=yes)
   - Bit 4-15: Reserved (must be 0)

5. **Payload (N bytes):** MessagePack-encoded data
   - Contains actual message
   - May be compressed (if flag set)
   - Always MessagePack format

### Payload Structure

Payload is MessagePack map with these fields:

```
{
    "type": "string",        // Message type (e.g., "gossip.announce")
    "id": "string",          // Unique message ID (UUID)
    "timestamp": integer,    // Unix timestamp (milliseconds)
    "sender": "string",      // Sender node ID
    "data": {...}            // Type-specific data
}
```

**Example (Heartbeat):**

```json
{
    "type": "heartbeat",
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "timestamp": 1705075200000,
    "sender": "node-1",
    "data": {
        "cpu_percent": 45.2,
        "ram_used_mb": 2048,
        "instance_count": 12
    }
}
```

---

## Message Types

### 1. Cluster Management Messages

#### GOSSIP.ANNOUNCE
Node announces its presence to the cluster.

**Direction:** Broadcast → All nodes
**Frequency:** On join, then every 30 seconds
**Payload:**
```json
{
    "type": "gossip.announce",
    "sender": "node-1",
    "data": {
        "address": "192.168.1.10:7946",
        "version": "0.0.4",
        "capabilities": ["instance", "filesystem", "mesh"],
        "metadata": {
            "datacenter": "us-east-1",
            "rack": "rack-3"
        }
    }
}
```

#### GOSSIP.HEARTBEAT
Regular heartbeat to indicate node is alive.

**Direction:** Broadcast → All nodes
**Frequency:** Every 10 seconds
**Payload:**
```json
{
    "type": "gossip.heartbeat",
    "sender": "node-1",
    "data": {
        "sequence": 12345,
        "cpu_percent": 45.2,
        "ram_used_mb": 2048,
        "instances": 12
    }
}
```

#### GOSSIP.LEAVE
Node is gracefully leaving the cluster.

**Direction:** Broadcast → All nodes
**Frequency:** On shutdown
**Payload:**
```json
{
    "type": "gossip.leave",
    "sender": "node-1",
    "data": {
        "reason": "shutdown",
        "transfer_instances_to": "node-2"
    }
}
```

#### GOSSIP.SUSPECT
Report a node as suspected dead.

**Direction:** Broadcast → All nodes
**Frequency:** When suspicion detected
**Payload:**
```json
{
    "type": "gossip.suspect",
    "sender": "node-2",
    "data": {
        "suspect_node": "node-3",
        "last_seen": 1705075100000,
        "failed_pings": 3
    }
}
```

---

### 2. Resource Management Messages

#### RESOURCE.STATUS
Report current resource availability.

**Direction:** Node → Controller
**Frequency:** Every 30 seconds or on change
**Payload:**
```json
{
    "type": "resource.status",
    "sender": "node-1",
    "data": {
        "cpu": {
            "total_cores": 16,
            "available_cores": 8.5,
            "percent_used": 46.875
        },
        "ram": {
            "total_mb": 32768,
            "available_mb": 16384,
            "percent_used": 50.0
        },
        "disk": {
            "total_gb": 1000,
            "available_gb": 750,
            "percent_used": 25.0
        },
        "network": {
            "in_mbps": 125.5,
            "out_mbps": 89.3
        }
    }
}
```

#### RESOURCE.REQUEST
Request resources for an instance.

**Direction:** Any node → Controller
**Frequency:** On instance creation
**Payload:**
```json
{
    "type": "resource.request",
    "sender": "node-1",
    "data": {
        "instance_id": "inst-12345",
        "cpu_cores": 2,
        "ram_mb": 4096,
        "disk_mb": 10240,
        "placement": {
            "strategy": "spread",
            "constraints": ["region=us-east"]
        }
    }
}
```

#### RESOURCE.ALLOCATED
Confirm resource allocation.

**Direction:** Controller → Requesting node
**Frequency:** In response to request
**Payload:**
```json
{
    "type": "resource.allocated",
    "sender": "controller",
    "data": {
        "instance_id": "inst-12345",
        "node": "node-2",
        "cpu_cores": 2,
        "ram_mb": 4096,
        "disk_mb": 10240,
        "expires_at": 1705078800000
    }
}
```

---

### 3. Instance Management Messages

#### INSTANCE.CREATE
Create a new instance.

**Direction:** Controller → Target node
**Frequency:** When instance requested
**Payload:**
```json
{
    "type": "instance.create",
    "sender": "controller",
    "data": {
        "instance_id": "inst-12345",
        "name": "web-worker-1",
        "script": "worker.rads",
        "env": {
            "PORT": "8080",
            "DB_URL": "postgresql://..."
        },
        "resources": {
            "cpu_percent": 25,
            "ram_mb": 512,
            "disk_mb": 1024
        }
    }
}
```

#### INSTANCE.STARTED
Instance has started successfully.

**Direction:** Node → Controller
**Frequency:** When instance starts
**Payload:**
```json
{
    "type": "instance.started",
    "sender": "node-2",
    "data": {
        "instance_id": "inst-12345",
        "pid": 54321,
        "startup_time_ms": 87,
        "listening_on": "0.0.0.0:8080"
    }
}
```

#### INSTANCE.STOPPED
Instance has stopped.

**Direction:** Node → Controller
**Frequency:** When instance stops
**Payload:**
```json
{
    "type": "instance.stopped",
    "sender": "node-2",
    "data": {
        "instance_id": "inst-12345",
        "exit_code": 0,
        "reason": "terminated",
        "runtime_seconds": 3600
    }
}
```

---

### 4. Filesystem Messages

#### FS.WRITE
Write file to distributed filesystem.

**Direction:** Any node → Storage nodes
**Frequency:** On file write
**Payload:**
```json
{
    "type": "fs.write",
    "sender": "node-1",
    "data": {
        "path": "/cluster/config.json",
        "size": 1024,
        "checksum": "sha256:abc123...",
        "chunks": 1,
        "replication_factor": 3
    }
}
```

#### FS.REPLICATE
Replicate file chunk to another node.

**Direction:** Storage node → Replica node
**Frequency:** On file write/rebalance
**Payload:**
```json
{
    "type": "fs.replicate",
    "sender": "node-1",
    "data": {
        "file_id": "file-789",
        "chunk_id": 0,
        "chunk_data": "<base64 encoded>",
        "checksum": "sha256:def456..."
    }
}
```

---

### 5. Service Mesh Messages

#### MESH.REGISTER
Register a service.

**Direction:** Node → Controller
**Frequency:** On service start
**Payload:**
```json
{
    "type": "mesh.register",
    "sender": "node-2",
    "data": {
        "service_name": "user-service",
        "version": "1.0.0",
        "instance_id": "inst-12345",
        "protocol": "http",
        "port": 8080,
        "health_check": "/health",
        "tags": ["api", "auth"]
    }
}
```

#### MESH.DEREGISTER
Deregister a service.

**Direction:** Node → Controller
**Frequency:** On service stop
**Payload:**
```json
{
    "type": "mesh.deregister",
    "sender": "node-2",
    "data": {
        "service_name": "user-service",
        "instance_id": "inst-12345"
    }
}
```

#### MESH.QUERY
Query for service instances.

**Direction:** Any node → Controller
**Frequency:** On service discovery
**Payload:**
```json
{
    "type": "mesh.query",
    "sender": "node-1",
    "data": {
        "service_name": "user-service",
        "filters": {
            "healthy": true,
            "version": "1.0.0"
        }
    }
}
```

#### MESH.RESPONSE
Response with service instances.

**Direction:** Controller → Requester
**Frequency:** In response to query
**Payload:**
```json
{
    "type": "mesh.response",
    "sender": "controller",
    "data": {
        "service_name": "user-service",
        "instances": [
            {
                "instance_id": "inst-12345",
                "node": "node-2",
                "address": "192.168.1.11:8080",
                "healthy": true,
                "latency_ms": 2.5
            },
            {
                "instance_id": "inst-67890",
                "node": "node-3",
                "address": "192.168.1.12:8080",
                "healthy": true,
                "latency_ms": 1.8
            }
        ]
    }
}
```

---

### 6. Control Messages

#### PING
Check if node is responsive.

**Direction:** Any node → Any node
**Frequency:** As needed
**Payload:**
```json
{
    "type": "ping",
    "sender": "node-1",
    "data": {
        "timestamp": 1705075200000
    }
}
```

#### PONG
Response to ping.

**Direction:** Response to PING
**Frequency:** In response to PING
**Payload:**
```json
{
    "type": "pong",
    "sender": "node-2",
    "data": {
        "request_timestamp": 1705075200000,
        "response_timestamp": 1705075200002
    }
}
```

#### ERROR
Report an error.

**Direction:** Any direction
**Frequency:** On error
**Payload:**
```json
{
    "type": "error",
    "sender": "node-2",
    "data": {
        "code": "RESOURCE_EXHAUSTED",
        "message": "Insufficient RAM to create instance",
        "details": {
            "requested_mb": 4096,
            "available_mb": 2048
        }
    }
}
```

---

## Connection Management

### Connection Lifecycle

```
CLIENT                           SERVER
  │                                │
  │──── TCP SYN ──────────────────→│
  │←─── TCP SYN-ACK ──────────────│
  │──── TCP ACK ──────────────────→│
  │                                │
  │──── TLS Client Hello ─────────→│
  │←─── TLS Server Hello ─────────│
  │       ... TLS Handshake ...   │
  │                                │
  │──── RNP HELLO ────────────────→│
  │        (version, node_id)      │
  │←─── RNP HELLO_ACK ────────────│
  │        (version, capabilities) │
  │                                │
  │ ──── RNP Messages ────────────→│
  │ ←─── RNP Messages ────────────│
  │                                │
  │──── RNP GOODBYE ──────────────→│
  │←─── RNP GOODBYE_ACK ──────────│
  │                                │
  │──── TCP FIN ──────────────────→│
  │←─── TCP FIN-ACK ──────────────│
  └────────────────────────────────┘
```

### Connection Pooling

Each node maintains connection pools:

- **Control Pool:** Connections to controller (default: 4 connections)
- **Peer Pool:** Connections to peer nodes (default: 2 per peer)
- **Data Pool:** Connections for bulk data transfer (default: 8 connections)

**Configuration:**
```rads
constellation.network.configure({
    control_pool_size: 4,
    peer_pool_size: 2,
    data_pool_size: 8,
    max_connections: 1000,
    idle_timeout_seconds: 300,
    keepalive_interval_seconds: 60
});
```

### Heartbeat Protocol

Nodes send heartbeats to detect failures:

```
┌──────────────────────────────────────┐
│         Heartbeat Timeline           │
├──────────────────────────────────────┤
│  T+0s   Send heartbeat               │
│  T+10s  Send heartbeat               │
│  T+20s  Send heartbeat               │
│  T+30s  No heartbeat received!       │
│  T+31s  Mark node as SUSPECT         │
│  T+32s  Send indirect ping (via peer)│
│  T+35s  No response                  │
│  T+36s  Mark node as DEAD            │
│  T+37s  Broadcast GOSSIP.SUSPECT     │
└──────────────────────────────────────┘
```

**States:**
- **ALIVE:** Node is healthy
- **SUSPECT:** Node might be dead (missed heartbeats)
- **DEAD:** Node is confirmed dead

---

## Security

### TLS Configuration

**Required:**
- TLS version: 1.3 minimum
- Certificate validation: Always enabled
- Mutual authentication: Required

**Cipher Suites (preference order):**
1. `TLS_AES_256_GCM_SHA384`
2. `TLS_AES_128_GCM_SHA256`
3. `TLS_CHACHA20_POLY1305_SHA256`

**Certificates:**
- Each node has unique certificate
- Signed by cluster CA
- Subject Alternative Name (SAN) includes node ID
- Valid for 365 days (auto-rotation at 300 days)

### Authentication

**Node Authentication:**
```
1. Client presents certificate during TLS handshake
2. Server validates certificate against CA
3. Server extracts node ID from certificate SAN
4. Server checks node ID against allowed list
5. If valid, connection accepted
```

**Message Authentication:**
- Every message includes sender node ID
- Sender node ID must match certificate
- Controller maintains node registry
- Unauthorized nodes are rejected

### Message Integrity

**HMAC Protection:**
- Optional HMAC-SHA256 for message integrity
- Shared secret per cluster
- HMAC included in payload
- Prevents message tampering

```json
{
    "type": "resource.status",
    "sender": "node-1",
    "data": {...},
    "hmac": "sha256:abc123..."
}
```

---

## Error Handling

### Error Codes

| Code | Name | Description |
|------|------|-------------|
| 1000 | PROTOCOL_ERROR | Invalid protocol usage |
| 1001 | VERSION_MISMATCH | Incompatible protocol version |
| 2000 | AUTH_FAILED | Authentication failed |
| 2001 | UNAUTHORIZED | Not authorized for operation |
| 3000 | RESOURCE_EXHAUSTED | Insufficient resources |
| 3001 | QUOTA_EXCEEDED | Quota limit reached |
| 4000 | NOT_FOUND | Requested resource not found |
| 4001 | ALREADY_EXISTS | Resource already exists |
| 5000 | INTERNAL_ERROR | Internal server error |
| 5001 | TIMEOUT | Operation timed out |

### Retry Policy

**Default Retry Strategy:**
```
Attempt 1: Immediate
Attempt 2: After 100ms
Attempt 3: After 300ms
Attempt 4: After 900ms
Attempt 5: After 2700ms
Max attempts: 5
```

**Exponential Backoff:**
```
delay = base_delay * (multiplier ^ (attempt - 1))
where:
  base_delay = 100ms
  multiplier = 3
  max_delay = 30s
```

---

## Performance

### Latency Targets

| Message Type | Target Latency (p99) |
|--------------|---------------------|
| PING/PONG | < 1ms |
| HEARTBEAT | < 5ms |
| GOSSIP | < 10ms |
| RESOURCE.STATUS | < 20ms |
| INSTANCE.CREATE | < 50ms |
| FS.WRITE (1MB) | < 100ms |

### Throughput Targets

| Operation | Target Throughput |
|-----------|------------------|
| Messages/sec | 100,000+ |
| Data transfer | 1 GB/sec+ |
| Concurrent connections | 10,000+ |

### Optimization Techniques

1. **Connection Pooling:** Reuse TCP connections
2. **MessagePack:** Efficient binary serialization
3. **LZ4 Compression:** Optional compression for large payloads
4. **Zero-Copy:** Direct buffer transfers where possible
5. **Batching:** Group small messages together

---

## Implementation Notes

### Message Serialization

**MessagePack Advantages:**
- Binary format (smaller than JSON)
- Fast encoding/decoding
- Wide language support
- Type-safe

**Example (C):**
```c
#include <msgpack.h>

// Serialize message
msgpack_sbuffer sbuf;
msgpack_packer pk;
msgpack_sbuffer_init(&sbuf);
msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

msgpack_pack_map(&pk, 4);
msgpack_pack_str(&pk, 4); msgpack_pack_str_body(&pk, "type", 4);
msgpack_pack_str(&pk, 9); msgpack_pack_str_body(&pk, "heartbeat", 9);
// ... pack other fields ...

// Send buffer: sbuf.data, sbuf.size
```

### Compression

**LZ4 Compression:**
- Enable for payloads > 1KB
- Typical compression: 2-3x
- Fast: < 1ms for 100KB

**When to compress:**
```c
if (payload_size > 1024 && compression_enabled) {
    compressed = lz4_compress(payload, payload_size);
    if (compressed.size < payload_size * 0.9) {
        // Use compressed version (10%+ savings)
        set_flag(flags, FLAG_COMPRESSED);
        payload = compressed.data;
    }
}
```

### Flow Control

**Window Size:**
- Default: 64KB
- Max: 16MB
- Adjustable based on network conditions

**Backpressure:**
```
if (send_queue_size > threshold) {
    // Slow down sender
    apply_backpressure();
}
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0-draft | 2026-01-12 | Initial specification |

---

## References

- MessagePack: https://msgpack.org/
- TLS 1.3: RFC 8446
- SWIM: https://www.cs.cornell.edu/projects/Quicksilver/public_pdfs/SWIM.pdf
- Raft: https://raft.github.io/

---

**RNP - RADS Native Protocol**
**Fast. Secure. Reliable.**

*Part of RADS v0.0.4 "Constellation"*
