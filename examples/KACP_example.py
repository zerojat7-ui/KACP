“””
KACP — Kcode Artery Connection Protocol
Python Client Example v1.0

License: Apache License 2.0
“””

import socket
import struct
import time
import hashlib
import ssl

class KACPClient:
def **init**(self, host=‘127.0.0.1’, port=7070, token=“YOUR_AUTH_TOKEN_HERE”):
self.host = host
self.port = port
self.token = token.encode(‘utf-8’)[:24].ljust(24, b’\x00’)
self.link_id = b’\x00’ * 16  # 0x00 = not yet issued
self.sequence = 0

```
def _calculate_checksum(self, header_block1, data_blocks):
    """
    SHA-256 over:
        header_block1 bytes 0~23 (checksum field set to 0x00)
        + AUTH block
        + all data blocks
    Returns first 8 bytes.
    """
    content = header_block1[:24]
    for block in data_blocks:
        content += block
    return hashlib.sha256(content).digest()[:8]

def create_concept_packet(self, concept_name):
    """
    Build a KACP query packet for a concept name.
    Packet structure: Header(64) + AUTH(32) + Data(32) = 128 bytes
    """
    self.sequence += 1
    timestamp = int(time.time())

    # ── Data Block 1: Concept Identifier (32 bytes) ──────────
    data_block = concept_name.encode('utf-8')[:32].ljust(32, b'\x00')
    data_blocks = [data_block]

    # ── Header Block 1 (32 bytes) ────────────────────────────
    # '<4sBBBBHHIII' = 4+1+1+1+1+2+2+4+4+4 = 24 bytes
    # checksum (8 bytes) appended after = 32 bytes total
    header1_no_check = struct.pack('<4sBBBBHHIII',
        b"KEXT",              # magic
        1,                    # version
        0b00000000,           # flags: request / reference / normal / query
        0,                    # chunk_index
        1,                    # chunk_total
        len(data_blocks),     # data_blocks count
        0,                    # ont_blocks count
        0,                    # scene_id
        self.sequence,        # sequence
        timestamp             # timestamp
    )
    checksum = self._calculate_checksum(header1_no_check, data_blocks)
    header1 = header1_no_check + checksum  # 24 + 8 = 32 bytes

    # ── Header Block 2 (32 bytes) ────────────────────────────
    header2 = struct.pack('<16sBBBB12s',
        self.link_id,         # link_id (0x00 = new)
        0xFF,                 # access_level  (unset)
        0xFF,                 # render_allow  (unset)
        0xFF,                 # copy_allow    (unset)
        0xFF,                 # learn_allow   (unset)
        b'\x00' * 12         # reserved
    )

    # ── AUTH Block (32 bytes) — KEXT mandatory ───────────────
    auth_block = struct.pack('<24sBB6s',
        self.token,           # auth_token
        0,                    # token_type: 0=API Key / 1=JWT
        0,                    # rate_limit: 0=normal / 1=premium
        b'\x00' * 6          # reserved
    )

    return header1 + header2 + auth_block + b''.join(data_blocks)

def _parse_response(self, data):
    """Parse ONT response header and print result."""
    if len(data) < 64:
        print("[-] Response too short")
        return

    magic = data[:4]
    flags = data[5]
    direction = flags & 0x01  # bit0: 1=response
    ont_blocks = struct.unpack('<H', data[10:12])[0]
    link_id = data[32:48]

    print(f"[+] Magic:      {magic.decode()}")
    print(f"[+] Direction:  {'response' if direction else 'request'}")
    print(f"[+] ONT blocks: {ont_blocks}")
    print(f"[+] link_id:    {link_id.hex()}")

    # Store issued link_id for future packets
    if link_id != b'\x00' * 16:
        self.link_id = link_id
        print(f"[+] link_id stored for future use")

    # Parse ONT response blocks
    offset = 96  # header(64) + auth(32)
    for i in range(ont_blocks):
        if offset + 32 > len(data):
            break
        block = data[offset:offset + 32]
        concept = block.rstrip(b'\x00').decode('utf-8', errors='ignore')
        print(f"[+] ONT Block {i+1}: {concept}")
        offset += 32

def send_query(self, concept_name):
    """Send a KACP concept query over TLS."""
    packet = self.create_concept_packet(concept_name)

    # ── TLS setup (mandatory for KEXT) ───────────────────────
    context = ssl.create_default_context()
    # NOTE: In production, enable certificate verification:
    #   context.load_verify_locations('/path/to/ca.crt')
    #   context.verify_mode = ssl.CERT_REQUIRED
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE

    try:
        with socket.create_connection((self.host, self.port)) as sock:
            with context.wrap_socket(sock, server_hostname=self.host) as ssock:
                print(f"[*] Connected to {self.host}:{self.port} (TLS)")
                print(f"[*] Sending KACP packet ({len(packet)} bytes)...")
                ssock.sendall(packet)

                # Receive response header (minimum 64 bytes)
                response = ssock.read(4096)
                if response:
                    print(f"[*] Received {len(response)} bytes")
                    self._parse_response(response)
                else:
                    print("[-] No response received")

    except ConnectionRefusedError:
        print(f"[-] Connection refused: {self.host}:{self.port}")
    except ssl.SSLError as e:
        print(f"[-] TLS error: {e}")
    except Exception as e:
        print(f"[-] Error: {e}")
```

if **name** == “**main**”:
client = KACPClient(
host=‘127.0.0.1’,
port=7070,
token=“KCODE-MASTER-TOKEN-2026”
)
client.send_query(“TemperatureSensor”)