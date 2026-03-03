# KACP — Kcode Artery Connection Protocol

> **Version:** 1.2  
> **Status:** Specification Release  
> **License:** Apache License 2.0

---

## Overview

**KACP (Kcode Artery Connection Protocol)** is a high-speed binary communication protocol designed for the Kcode ecosystem.

Just as an artery carries blood from the heart to the entire body, KACP serves as the core data channel connecting the intelligence layer (Ontology Engine) and the physical layer (DNA Engine).

---

## Purpose

```
High-speed binary communication between:
    Ontology Engine  — Knowledge & Intelligence Layer
    DNA Engine       — 3D Physical Processing Layer
    Language Engine  — Kcode Compiler & Runtime

External languages can also connect via KEXT magic code.
```

---

## Core Technologies

| Feature | Description |
|---------|-------------|
| **32-Byte Alignment** | All blocks aligned to 32 bytes for Zero-Copy GPU memory mapping |
| **Magic Number Routing** | Single port handles all traffic via 4-byte magic identification |
| **KEXT Forced TLS** | External connections always encrypted — cannot be disabled |
| **Delta-Only Sync** | Only changed data is transmitted (Delta=0 → blocked) |
| **Chunked Transfer** | New data split into 32-byte chunks for reliable delivery |
| **Reference Transfer** | Existing data sent by reference + delta only |

---

## Magic Code Table

| Magic | Source | Description |
|-------|--------|-------------|
| `KEXT` | External Languages | Binary + forced TLS + AUTH block |
| `KGET` | External HTTP | HTTP GET (backward compatibility) |
| `KPST` | External HTTP | HTTP POST (backward compatibility) |
| `KSPR` | External SPARQL | SPARQL query |
| `KWSS` | WebSocket | Real-time push notifications |

---

## Packet Structure

```
┌──────────────────────────────┐
│   HEADER Block 1  (32 bytes) │  Required — Basic Info
│   HEADER Block 2  (32 bytes) │  Required — Link ID + Permissions
├──────────────────────────────┤
│   AUTH Block      (32 bytes) │  KEXT only (mandatory)
├──────────────────────────────┤
│   Data Blocks  (32 bytes × N)│  Variable
└──────────────────────────────┘
```

---

## Security Model

```
Internal Engines (engine-to-engine):
    3-step verification
    TLS: configurable (off for development)

External Language (KEXT):
    6-step verification
    TLS: always ON — cannot be disabled
    AUTH block mandatory
    Rate limiting applied
```

### 4-Layer Security (KEXT)

```
Layer 1  TLS          Encryption in transit
Layer 2  checksum     Packet integrity (includes AUTH block)
Layer 3  sequence     Packet loss detection
Layer 4  timestamp    Replay attack prevention (±30s window)
```

---

## TLS Configuration

```c
typedef struct {
    uint8_t internal_tls;   // engine-to-engine — 0=off(dev) / 1=on
    uint8_t external_tls;   // KEXT — always 1, cannot be set to 0
    // ...
} KACPTLSConfig;
```

---

## For External Implementors

If you want to connect your language/framework to the Kcode Ontology Engine:

```
1. Register your engine with the central Kcode Engine Server
2. Use magic code "KEXT"
3. Include AUTH block (32 bytes) after the header
4. Obtain auth_token from the Ontology Engine administrator
5. Connect via TLS socket (port 7070)
6. Follow the chunked/reference transfer rules
7. Select Knowledge Afterlife Policy when registering knowledge
```

See `spec/kacp_spec.md` for full implementation details.  
See `spec/kacp_header.h` for C struct definitions.  
See `examples/` for sample implementations.

---

## Engine Registration & Lifecycle

```
Every Ontology Engine must register with the central Kcode Engine Server
before issuing link_ids.

Registration:
    Engine first boot → central server registration → unique Engine ID issued
    Engine ID is embedded in all link_ids this engine issues

Offline fallback (Provisional ID):
    If central server is unreachable, engine uses provisional ID range
    Provisionally issued link_ids are isolated (Level 0) until re-registered

Engine lifecycle:
    Active        → Normal operation
    Warning       → 2 years inactive (knowledge status report sent)
    Final Warning → 2.5 years inactive
    Frozen        → 3 years inactive (new issuance suspended, recovery possible)
    Terminated    → 5 years inactive (Knowledge Afterlife Policy executed)

Recovery:
    Identity verification during Frozen state → full re-activation
    All existing link_ids remain valid
```

---

## Knowledge Afterlife Policy

```
Select one policy when registering knowledge (required).
This selection is recorded as legal consent.

Option A — Private Forever
    Knowledge frozen on engine termination. Owner/heir recovers anytime.

Option B — Delayed Public [Default]
    Goes public after N years (1~10, default 3). Merge check applied.

Option C — Public on Termination
    Immediately public on engine termination.

Option D — Designated Successor
    Ownership transferred to a designated engine on termination.
```

---

## What to Keep Private

```
✅ Public (this repository):
    KACP packet structure
    Connection procedure
    Magic code table
    AUTH block format

🔒 Private (not disclosed):
    DNA Engine internal algorithms
    Ontology reasoning logic
    Evolution algorithms
    PASSPORT ownership system
    Internal engine source code
```

---

## Repository Structure

```
KACP/
    KACP_README.md        This file
    LICENSE               Apache License 2.0
    spec/
        kacp_header.h     C/C++ struct definitions
        kacp_spec.md      Full specification (Korean)
    examples/
        python/           Python connection example (coming soon)
        go/               Go connection example (coming soon)
```

---

## License

```
Apache License 2.0

You are free to:
    Use, modify, distribute this specification
    Build libraries in any language

Protection:
    Patent retaliation clause included
    Any party using this spec cannot file
    patent claims against the original authors
```

---

*KACP — Kcode Artery Connection Protocol*  
*Connecting Intelligence and Physics at the speed of binary*
