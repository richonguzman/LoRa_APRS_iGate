#!/usr/bin/env python3
"""Minimal KISS-over-TCP client to test the RP2350 iGate TNC server (:8001).

Monitor received frames:
    python tools/kiss_test.py 192.168.1.234

Send one APRS frame over RF through the iGate, then keep monitoring:
    python tools/kiss_test.py 192.168.1.234 --send "EA4GLO-9>APRS,WIDE1-1:>KISS test"

Mirrors the AX.25/KISS codec in src/kiss_utils.cpp.
"""
import argparse
import socket
import sys
import threading

FEND, FESC, TFEND, TFESC = 0xC0, 0xDB, 0xDC, 0xDD
CMD_DATA = 0x00
LAST = 0x01
DIGIPITED = 0x80


def encode_addr(addr):
    digi = "*" in addr
    if "-" not in addr:
        if digi:
            addr = addr[:-1]
        addr += "-0"
    base, ssid = addr.split("-")
    base = base.replace("*", "")
    out = bytearray()
    for i in range(6):
        c = base[i] if i < len(base) else " "
        out.append((ord(c) << 1) & 0xFF)
    out.append(((int(ssid) << 1) | 0x60 | (DIGIPITED if digi else 0)) & 0xFF)
    return out


def tnc2_to_ax25(frame):
    head, info = frame.split(":", 1)
    parts = head.replace(">", ",").split(",")  # [src, dst, digi...]
    src, dst = parts[0], parts[1]
    digis = parts[2:]
    ax = bytearray()
    ax += encode_addr(dst)
    ax += encode_addr(src)
    for d in digis:
        ax += encode_addr(d)
    ax[-1] |= LAST
    ax += bytes([0x03, 0xF0])
    ax += info.encode("latin-1", "replace")
    return ax


def kiss_wrap(ax25):
    out = bytearray([FEND, CMD_DATA])
    for b in ax25:
        if b == FEND:
            out += bytes([FESC, TFEND])
        elif b == FESC:
            out += bytes([FESC, TFESC])
        else:
            out.append(b)
    out.append(FEND)
    return bytes(out)


def decode_addr(a):
    base = "".join(chr(b >> 1) for b in a[:6]).strip()
    ssid_b = a[6]
    last = ssid_b & LAST
    digi = ssid_b & DIGIPITED
    ssid = (ssid_b >> 1) & 0x0F
    s = base + (f"-{ssid}" if ssid else "")
    if digi:
        s += "*"
    return s, last


def ax25_to_tnc2(ax):
    if len(ax) < 16:
        return None
    dst, _ = decode_addr(ax[0:7])
    src, last = decode_addr(ax[7:14])
    frame = f"{src}>{dst}"
    i = 14
    while not last and i + 7 <= len(ax):
        d, last = decode_addr(ax[i:i + 7])
        frame += "," + d
        i += 7
    # skip control(0x03)+pid(0xF0)
    info = ax[i + 2:].decode("latin-1", "replace")
    return frame + ":" + info


def reader(sock):
    buf = bytearray()
    in_frame = False
    esc = False
    while True:
        data = sock.recv(1024)
        if not data:
            print("[disconnected]")
            return
        for b in data:
            if b == FEND:
                if in_frame and len(buf) > 1:
                    ax = bytes(buf[1:])  # drop the KISS command byte
                    frame = ax25_to_tnc2(ax)
                    print("RX:", frame if frame else ax.hex())
                buf = bytearray()
                in_frame = True
            elif in_frame:
                if esc:
                    buf.append(FEND if b == TFEND else FESC)
                    esc = False
                elif b == FESC:
                    esc = True
                else:
                    buf.append(b)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("host")
    ap.add_argument("--port", type=int, default=8001)
    ap.add_argument("--send", help="TNC2 frame to transmit, e.g. 'CALL>APRS,WIDE1-1:>hi'")
    args = ap.parse_args()

    s = socket.create_connection((args.host, args.port), timeout=5)
    s.settimeout(None)                       # block on recv (monitor indefinitely)
    print(f"connected to {args.host}:{args.port}")

    if args.send:
        s.sendall(kiss_wrap(tnc2_to_ax25(args.send)))
        print("TX:", args.send)

    t = threading.Thread(target=reader, args=(s,), daemon=True)
    t.start()
    try:
        t.join()
    except KeyboardInterrupt:
        print("\nbye")
        s.close()


if __name__ == "__main__":
    main()
