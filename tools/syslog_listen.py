#!/usr/bin/env python3
"""Tiny UDP syslog listener to validate the RP2350 iGate's remote syslog.

    python tools/syslog_listen.py [port]

Default port 5514 (avoids needing admin for the privileged 514). Set the same
port in the iGate's Syslog config, with server = this PC's LAN IP.
"""
import socket
import sys
import time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 5514
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("0.0.0.0", port))
print(f"syslog listening on udp/{port} …", flush=True)
while True:
    data, addr = s.recvfrom(2048)
    print(time.strftime("%H:%M:%S"), addr[0], data.decode("latin-1", "replace"), flush=True)
