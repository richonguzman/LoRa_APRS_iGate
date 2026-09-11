#!/usr/bin/env python3
"""Read-only APRS-IS listener to validate what a station puts on the network
(beacons, telemetry T#, messages) — independent of aprs.fi's display.

    python tools/aprsis_listen.py EA4GLO-11 [seconds]

Logs in unverified (receive-only, pass -1) with a buddy filter on the target.
"""
import socket
import sys
import time

target = sys.argv[1] if len(sys.argv) > 1 else "EA4GLO-11"
dur = int(sys.argv[2]) if len(sys.argv) > 2 else 180

s = socket.create_connection(("rotate.aprs2.net", 14580), timeout=10)
s.sendall(f"user EA4GLO-14 pass -1 vers aprsislisten 1.0 filter b/{target}\r\n".encode())
s.settimeout(2)
print(f"listening for {target} for {dur}s ...", flush=True)

buf = b""
end = time.time() + dur
while time.time() < end:
    try:
        data = s.recv(4096)
    except socket.timeout:
        continue
    if not data:
        break
    buf += data
    while b"\n" in buf:
        line, buf = buf.split(b"\n", 1)
        txt = line.decode("latin-1", "replace").rstrip("\r")
        if txt and not txt.startswith("#"):
            print(time.strftime("%H:%M:%S"), txt, flush=True)
s.close()
print("--- done ---", flush=True)
