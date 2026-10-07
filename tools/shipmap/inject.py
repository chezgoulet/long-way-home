#!/usr/bin/env python3
"""Append entities to a compiled map's entity list.

    inject.py MAP.bsp ENTITIES.txt

A compiler flood-fills the map from every entity that has an origin, and reports a leak -- and
writes no visibility data -- if one of them lies in a wall or outside the hull. Entities that are
volumes rather than things (box triggers) are often exactly there. They mean nothing to the
compiler, so they are kept out of the compile and added to the compiled map here: the entity lump
is rewritten at the end of the file and the header pointed at it. Nothing else in the BSP moves.
"""

import struct
import sys


def inject(bsp_path, text):
    with open(bsp_path, "rb") as f:
        data = bytearray(f.read())
    if data[:4] != b"IBSP":
        raise ValueError(f"{bsp_path}: not an IBSP file")
    ofs, length = struct.unpack_from("<ii", data, 8)
    old = bytes(data[ofs:ofs + length]).rstrip(b"\0")
    extra = text.strip().encode("latin-1")
    if not extra:
        return 0
    new = old + b"\n" + extra + b"\n\0"
    while len(data) % 4:
        data.append(0)
    struct.pack_into("<ii", data, 8, len(data), len(new))
    data += new
    with open(bsp_path, "wb") as f:
        f.write(data)
    return extra.count(b'"classname"')


def main(argv):
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    with open(argv[2], encoding="latin-1") as f:
        n = inject(argv[1], f.read())
    print(f"    {n} entities added to {argv[1]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
