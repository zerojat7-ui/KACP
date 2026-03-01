/**

- KACP — Kcode Artery Connection Protocol
- JavaScript (Node.js) Client Example v1.0
- 
- License: Apache License 2.0
- 
- Requirements: Node.js 18+
- Usage: node kacp_example.js
  */

‘use strict’;

const net   = require(‘net’);
const tls   = require(‘tls’);
const crypto = require(‘crypto’);

class KACPClient {
constructor({ host = ‘127.0.0.1’, port = 7070, token = ‘YOUR_AUTH_TOKEN_HERE’ } = {}) {
this.host     = host;
this.port     = port;
this.token    = Buffer.alloc(24, 0);
this.linkId   = Buffer.alloc(16, 0);  // 0x00 = not yet issued
this.sequence = 0;

```
    // Token: UTF-8, max 24 bytes, zero-padded
    const tokenBuf = Buffer.from(token, 'utf8').slice(0, 24);
    tokenBuf.copy(this.token);
}

/**
 * SHA-256 over:
 *   header_block1 bytes 0~23 (checksum field = 0x00)
 *   + AUTH block
 *   + all data blocks
 * Returns first 8 bytes.
 */
_calculateChecksum(header1NoCheck, dataBlocks) {
    const hash = crypto.createHash('sha256');
    hash.update(header1NoCheck.slice(0, 24));
    for (const block of dataBlocks) {
        hash.update(block);
    }
    return hash.digest().slice(0, 8);
}

/**
 * Build a KACP query packet for a concept name.
 * Packet: Header(64) + AUTH(32) + Data(32) = 128 bytes
 */
createConceptPacket(conceptName) {
    this.sequence += 1;
    const timestamp = Math.floor(Date.now() / 1000);

    // ── Data Block 1: Concept Identifier (32 bytes) ──────────
    const dataBlock = Buffer.alloc(32, 0);
    Buffer.from(conceptName, 'utf8').slice(0, 32).copy(dataBlock);
    const dataBlocks = [dataBlock];

    // ── Header Block 1 (32 bytes) ────────────────────────────
    // 24 bytes fields + 8 bytes checksum = 32 bytes
    const header1NoCheck = Buffer.alloc(24, 0);
    header1NoCheck.write('KEXT', 0, 4, 'ascii');        // magic       [0~3]
    header1NoCheck.writeUInt8(1, 4);                     // version     [4]
    header1NoCheck.writeUInt8(0b00000000, 5);            // flags       [5]
    header1NoCheck.writeUInt8(0, 6);                     // chunk_index [6]
    header1NoCheck.writeUInt8(1, 7);                     // chunk_total [7]
    header1NoCheck.writeUInt16LE(dataBlocks.length, 8);  // data_blocks [8~9]
    header1NoCheck.writeUInt16LE(0, 10);                 // ont_blocks  [10~11]
    header1NoCheck.writeUInt32LE(0, 12);                 // scene_id    [12~15]
    header1NoCheck.writeUInt32LE(this.sequence, 16);     // sequence    [16~19]
    header1NoCheck.writeUInt32LE(timestamp, 20);         // timestamp   [20~23]

    const checksum = this._calculateChecksum(header1NoCheck, dataBlocks);
    const header1 = Buffer.concat([header1NoCheck, checksum]); // 24+8=32

    // ── Header Block 2 (32 bytes) ────────────────────────────
    const header2 = Buffer.alloc(32, 0);
    this.linkId.copy(header2, 0);    // link_id      [0~15]
    header2.writeUInt8(0xFF, 16);    // access_level [16]
    header2.writeUInt8(0xFF, 17);    // render_allow [17]
    header2.writeUInt8(0xFF, 18);    // copy_allow   [18]
    header2.writeUInt8(0xFF, 19);    // learn_allow  [19]
    // reserved [20~31] = 0x00

    // ── AUTH Block (32 bytes) — KEXT mandatory ───────────────
    const authBlock = Buffer.alloc(32, 0);
    this.token.copy(authBlock, 0);   // auth_token   [0~23]
    authBlock.writeUInt8(0, 24);     // token_type   [24] 0=API Key
    authBlock.writeUInt8(0, 25);     // rate_limit   [25] 0=normal
    // reserved [26~31] = 0x00

    return Buffer.concat([header1, header2, authBlock, ...dataBlocks]);
}

/**
 * Parse ONT response header and blocks.
 */
_parseResponse(data) {
    if (data.length < 64) {
        console.log('[-] Response too short');
        return;
    }

    const magic     = data.slice(0, 4).toString('ascii');
    const flags     = data.readUInt8(5);
    const direction = flags & 0x01;   // bit0: 1=response
    const ontBlocks = data.readUInt16LE(10);
    const linkId    = data.slice(32, 48);

    console.log(`[+] Magic:      ${magic}`);
    console.log(`[+] Direction:  ${direction ? 'response' : 'request'}`);
    console.log(`[+] ONT blocks: ${ontBlocks}`);
    console.log(`[+] link_id:    ${linkId.toString('hex')}`);

    // Store issued link_id for future packets
    if (!linkId.equals(Buffer.alloc(16, 0))) {
        this.linkId = linkId;
        console.log(`[+] link_id stored for future use`);
    }

    // Parse ONT response blocks
    let offset = 96; // header(64) + auth(32)
    for (let i = 0; i < ontBlocks; i++) {
        if (offset + 32 > data.length) break;
        const block   = data.slice(offset, offset + 32);
        const concept = block.toString('utf8').replace(/\x00/g, '').trim();
        console.log(`[+] ONT Block ${i + 1}: ${concept}`);
        offset += 32;
    }
}

/**
 * Send a KACP concept query over TLS.
 */
sendQuery(conceptName) {
    return new Promise((resolve, reject) => {
        const packet = this.createConceptPacket(conceptName);

        // ── TLS setup (mandatory for KEXT) ───────────────────
        const options = {
            host: this.host,
            port: this.port,
            // NOTE: In production, enable certificate verification:
            //   ca: fs.readFileSync('/path/to/ca.crt'),
            //   checkServerIdentity: () => undefined,
            rejectUnauthorized: false,
        };

        const socket = tls.connect(options, () => {
            console.log(`[*] Connected to ${this.host}:${this.port} (TLS)`);
            console.log(`[*] Sending KACP packet (${packet.length} bytes)...`);
            socket.write(packet);
        });

        const chunks = [];
        socket.on('data', (chunk) => chunks.push(chunk));

        socket.on('end', () => {
            const response = Buffer.concat(chunks);
            if (response.length > 0) {
                console.log(`[*] Received ${response.length} bytes`);
                this._parseResponse(response);
            } else {
                console.log('[-] No response received');
            }
            resolve();
        });

        socket.on('error', (err) => {
            console.error(`[-] Error: ${err.message}`);
            reject(err);
        });
    });
}
```

}

// ── Packet structure validation (offline) ────────────────
function validatePacket() {
console.log(’=== KACP Packet Structure Validation ===’);

```
const client = new KACPClient({ token: 'KCODE-MASTER-TOKEN-2026' });
const packet = client.createConceptPacket('TemperatureSensor');

const check = (label, actual, expected) => {
    const ok = actual === expected;
    console.log(`${label}: ${actual} ${ok ? '✅' : `❌ (expected ${expected})`}`);
    return ok;
};

check('Total size (bytes)',   packet.length, 128);

const h1   = packet.slice(0, 32);
const h2   = packet.slice(32, 64);
const auth = packet.slice(64, 96);
const data = packet.slice(96, 128);

check('Header Block 1',  h1.length,   32);
check('Header Block 2',  h2.length,   32);
check('AUTH Block',      auth.length, 32);
check('Data Block',      data.length, 32);

console.log('\n--- Header Block 1 ---');
const magic   = h1.slice(0, 4).toString('ascii');
const version = h1.readUInt8(4);
const flags   = h1.readUInt8(5);
const dblocks = h1.readUInt16LE(8);
const seq     = h1.readUInt32LE(16);
const chksum  = h1.slice(24, 32).toString('hex');
check('magic',       magic,   'KEXT');
check('version',     version, 1);
check('data_blocks', dblocks, 1);
console.log(`flags:       0b${flags.toString(2).padStart(8,'0')}`);
console.log(`sequence:    ${seq}`);
console.log(`checksum:    ${chksum}`);

console.log('\n--- Header Block 2 ---');
const access = h2.readUInt8(16);
check('access_level (0xFF)', access, 0xFF);

console.log('\n--- AUTH Block ---');
const ttype = auth.readUInt8(24);
check('token_type (0=API Key)', ttype, 0);

console.log('\n--- Data Block ---');
const concept = data.toString('utf8').replace(/\x00/g, '');
check('concept', concept, 'TemperatureSensor');

console.log('\n--- Checksum Re-verification ---');
const hash = crypto.createHash('sha256');
hash.update(h1.slice(0, 24));
hash.update(data);
const recalc = hash.digest().slice(0, 8).toString('hex');
const original = h1.slice(24, 32).toString('hex');
console.log(`original:  ${original}`);
console.log(`recalc:    ${recalc}`);
check('checksum match', original, recalc);
```

}

// ── Entry point ──────────────────────────────────────────
const args = process.argv.slice(2);

if (args.includes(’–validate’)) {
validatePacket();
} else {
const client = new KACPClient({
host:  ‘127.0.0.1’,
port:  7070,
token: ‘KCODE-MASTER-TOKEN-2026’,
});
client.sendQuery(‘TemperatureSensor’).catch(console.error);
}