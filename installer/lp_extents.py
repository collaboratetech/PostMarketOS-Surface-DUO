#!/usr/bin/env python3
"""Print the device-mapper linear table of a logical partition inside Android's `super` partition.

Usage: lp_extents.py /dev/<super> <partition_name> [slot_index]
Output: lines "<start_sector>:<length_sectors>" relative to the super device, usable with duo-dmlin.
Read-only. Parses LP metadata (liblp format): geometry at 4096, metadata slots after it.
"""
import struct
import sys

dev, name = sys.argv[1], sys.argv[2]
slot = int(sys.argv[3]) if len(sys.argv) > 3 else 0
with open(dev, 'rb') as f:
    head = f.read(1 << 20)

geo = head[4096:8192]
if struct.unpack('<I', geo[:4])[0] != 0x616C4467:
    sys.exit('no LP geometry')
metadata_max, slot_count = struct.unpack('<II', geo[40:48])
base = 4096 + 8192 + slot * metadata_max
hdr = head[base:base + 256]
if struct.unpack('<I', hdr[:4])[0] != 0x414C5030:
    sys.exit('no LP metadata in slot %d' % slot)
hsize = struct.unpack('<I', hdr[8:12])[0]
tsize = struct.unpack('<I', hdr[44:48])[0]
desc = lambda o: struct.unpack('<III', hdr[o:o + 12])
parts, exts = desc(80), desc(92)
t = head[base + hsize:base + hsize + tsize]
extents = [struct.unpack('<QIQI', t[exts[0] + i * exts[2]:exts[0] + i * exts[2] + 24]) for i in range(exts[1])]
for i in range(parts[1]):
    o = parts[0] + i * parts[2]
    if t[o:o + 36].split(b'\0')[0].decode() != name:
        continue
    _, first, num, _ = struct.unpack('<IIII', t[o + 36:o + 52])
    for k in range(num):
        n_sectors, target_type, target_data, _ = extents[first + k]
        if target_type != 0:          # 0 = linear
            sys.exit('unsupported extent type')
        print('%d:%d' % (target_data, n_sectors))
    sys.exit(0)
sys.exit('partition %s not found' % name)
