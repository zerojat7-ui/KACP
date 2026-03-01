/**
 * kacp_header.h
 * Kcode Artery Connection Protocol — Struct Definitions
 * Version: 1.1
 * License: Apache License 2.0
 *
 * This file defines the public packet structures for KACP.
 * Internal engine logic is NOT included.
 */

#ifndef KACP_HEADER_H
#define KACP_HEADER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Version ─────────────────────────────────────────── */
#define KACP_VERSION          1

/* ── Magic Codes (4 bytes) ───────────────────────────── */
#define KACP_MAGIC_EXT        "KEXT"   /* External languages           */
#define KACP_MAGIC_GET        "KGET"   /* HTTP GET  (compatibility)    */
#define KACP_MAGIC_POST       "KPST"   /* HTTP POST (compatibility)    */
#define KACP_MAGIC_SPARQL     "KSPR"   /* SPARQL query                 */
#define KACP_MAGIC_WS         "KWSS"   /* WebSocket                    */

/* ── Flags (1 byte) ──────────────────────────────────── */
#define KACP_FLAG_DIRECTION   0x01     /* bit0: 0=request / 1=response */
#define KACP_FLAG_TRANSFER    0x02     /* bit1: 0=reference / 1=chunk  */
#define KACP_FLAG_IS_LAST     0x04     /* bit2: 1=last chunk           */
#define KACP_PRIORITY_NORMAL  0x00     /* bit3~4: 00=normal            */
#define KACP_PRIORITY_OWNER   0x08     /* bit3~4: 01=owner             */
#define KACP_PRIORITY_URGENT  0x10     /* bit3~4: 10=urgent            */
#define KACP_TYPE_QUERY       0x00     /* bit5~7: 000=query            */
#define KACP_TYPE_RESPONSE    0x20     /* bit5~7: 001=response         */
#define KACP_TYPE_PUSH        0x40     /* bit5~7: 010=push             */
#define KACP_TYPE_ACK         0x60     /* bit5~7: 011=ACK              */

/* ── Access Level ────────────────────────────────────── */
#define KACP_ACCESS_PRIVATE   0x00     /* Private                      */
#define KACP_ACCESS_SHARED    0x01     /* Designated users             */
#define KACP_ACCESS_PUBLIC    0x02     /* Public                       */
#define KACP_ACCESS_NONE      0xFF     /* Not set (use PASSPORT default)*/

/* ── Permission Flags ────────────────────────────────── */
#define KACP_ALLOW_YES        0x01     /* Allowed                      */
#define KACP_ALLOW_NO         0x00     /* Not allowed                  */
#define KACP_ALLOW_NONE       0xFF     /* Not set                      */

/* ── Token Type ──────────────────────────────────────── */
#define KACP_TOKEN_APIKEY     0x00     /* API Key                      */
#define KACP_TOKEN_JWT        0x01     /* JWT Token                    */

/* ── Error Codes ─────────────────────────────────────── */
#define KACP_OK               0x00     /* Success                      */
#define KACP_ERR_AUTH         0x01     /* Authentication failed        */
#define KACP_ERR_PERMISSION   0x02     /* Permission denied            */
#define KACP_ERR_RATELIMIT    0x03     /* Rate limit exceeded          */
#define KACP_ERR_NOT_FOUND    0x04     /* Concept not found            */
#define KACP_ERR_CHECKSUM     0x05     /* Checksum mismatch            */
#define KACP_ERR_BLOCK        0x06     /* Invalid block structure      */
#define KACP_ERR_SERVER       0xFF     /* Server error                 */

/* ── Packet Sizes ────────────────────────────────────── */
#define KACP_BLOCK_SIZE       32       /* All blocks: 32 bytes         */
#define KACP_HEADER_SIZE      64       /* Header: 64 bytes (2 blocks)  */
#define KACP_AUTH_SIZE        32       /* AUTH block: 32 bytes         */
#define KACP_LINK_ID_SIZE     16       /* link_id: 16 bytes            */
#define KACP_HASH_SIZE        32       /* DNA Hash: 32 bytes           */
#define KACP_CHECKSUM_SIZE    8        /* checksum: 8 bytes            */

/* ── Timestamp Window ────────────────────────────────── */
#define KACP_TIMESTAMP_WINDOW 30       /* ±30 seconds (replay attack)  */

/* ─────────────────────────────────────────────────────── */
/* HEADER Block 1 (32 bytes) — Basic Info                  */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  magic[4];               /* Magic code (e.g. "KEXT")      */
    uint8_t  version;                /* Protocol version (1)          */
    uint8_t  flags;                  /* Bit flags (see above)         */
    uint8_t  chunk_index;            /* Chunk sequence (0-based)      */
    uint8_t  chunk_total;            /* Total chunk count             */
    uint16_t data_blocks;            /* Number of data blocks         */
    uint16_t ont_blocks;             /* Number of ONT response blocks */
    uint32_t scene_id;               /* Scene ID (0 if unused)        */
    uint32_t sequence;               /* Packet sequence number        */
    uint32_t timestamp;              /* Unix timestamp (seconds)      */
    uint8_t  checksum[8];            /* SHA-256 first 8 bytes         */
                                     /* Covers: block1(0~23)          */
                                     /*       + AUTH block            */
                                     /*       + all data blocks       */
} KACPHeaderBlock1;                  /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* HEADER Block 2 (32 bytes) — Link ID + Permissions       */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  link_id[16];            /* ONT-issued link ID            */
                                     /* 0x00 = new (not yet issued)   */
    uint8_t  access_level;           /* Access level (KACP_ACCESS_*)  */
    uint8_t  render_allow;           /* Render permission             */
    uint8_t  copy_allow;             /* Copy permission               */
    uint8_t  learn_allow;            /* ML learn permission           */
    uint8_t  reserved[12];           /* Reserved — set to 0x00        */
} KACPHeaderBlock2;                  /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* Full Header (64 bytes)                                  */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    KACPHeaderBlock1 block1;         /* 32 bytes                      */
    KACPHeaderBlock2 block2;         /* 32 bytes                      */
} KACPHeader;                        /* 64 bytes total                */

/* ─────────────────────────────────────────────────────── */
/* AUTH Block (32 bytes) — KEXT mandatory                  */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  auth_token[24];         /* Authentication token          */
    uint8_t  token_type;             /* KACP_TOKEN_APIKEY or _JWT     */
    uint8_t  rate_limit;             /* 0=normal / 1=premium          */
    uint8_t  reserved[6];            /* Reserved — set to 0x00        */
} KACPExtAuthBlock;                  /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* Data Block: Concept Identifier (32 bytes)               */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    char     concept_name[32];       /* Concept name (UTF-8, 0-padded)*/
} KACPConceptBlock;                  /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* Data Block: Attribute Data (32 bytes)                   */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  attr_type;              /* Attribute type                */
    uint8_t  attr_count;             /* Number of attributes          */
    uint8_t  reserved[2];            /* Padding                       */
    float    attr_values[7];         /* Attribute values (7 × 4 bytes)*/
} KACPAttrBlock;                     /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* Data Block: Spatial Coordinates (32 bytes)              */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    float    xyz[3];                 /* Spatial coordinates           */
    uint32_t zone_id;                /* Zone ID                       */
    uint8_t  obj_type;               /* 0=static / 1=dynamic         */
    uint8_t  channel_flag;           /* Changed channel bitmask       */
    uint8_t  reserved[14];           /* Padding                       */
} KACPSpatialBlock;                  /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* Data Block: Delta Values (32 bytes)                     */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    float    delta[8];               /* Delta values (8 × 4 bytes)    */
                                     /* Do NOT send if all delta == 0 */
} KACPDeltaBlock;                    /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* ONT Response Block 1: Concept Node (32 bytes)           */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    char     concept_node[32];       /* Mapped ontology concept       */
} KACPONTConceptBlock;               /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* ONT Response Block 2: Constraint + Suggestion (32 bytes)*/
/* ─────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  constraint_flag;        /* 0=Pass / 1=Fail               */
    uint8_t  suggestion_type;        /* Action suggestion type        */
    uint8_t  ont_status;             /* Ontology match status         */
    uint8_t  error_code;             /* KACP_OK or KACP_ERR_*        */
    float    suggestion[7];          /* Suggestion values (advisory)  */
} KACPONTResultBlock;                /* 32 bytes                      */

/* ─────────────────────────────────────────────────────── */
/* TLS Configuration                                       */
/* ─────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  internal_tls;           /* KDNA/KLNG/KONT TLS            */
                                     /* 0=off(dev) / 1=on(production) */
    uint8_t  external_tls;           /* KEXT TLS — ALWAYS 1           */
                                     /* Setting to 0 causes abort     */
    char     cert_path[256];         /* Certificate path              */
    char     key_path[256];          /* Private key path              */
    char     ca_path[256];           /* CA certificate path           */
    uint32_t tls_timeout_ms;         /* Handshake timeout (ms)        */
    uint8_t  verify_peer;            /* 0=skip / 1=verify             */
} KACPTLSConfig;

#ifdef __cplusplus
}
#endif

#endif /* KACP_HEADER_H */
