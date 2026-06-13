#!/usr/bin/env python3
# Generate src/rp2350/web_assets.h from data_embed/* for the RP2350 iGate build.
# Each SPA asset is gzipped and embedded as a byte array; eth_web.cpp serves them
# with Content-Encoding: gzip. Run after editing any data_embed/* file:
#
#     python tools/gen_web_assets_rp2350.py
#
# (Standalone — unlike tools/compress.py, which is the ESP32 PlatformIO script
# that emits .gz files instead of this C header.)

import gzip
import os
import datetime

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT  = os.path.join(ROOT, "src", "rp2350", "web_assets.h")

# (url path, source file, C array name, content-type)
ASSETS = [
    ("/",              "index.html",    "asset_index_html",    "text/html"),
    ("/style.css",     "style.css",     "asset_style_css",     "text/css"),
    ("/script.js",     "script.js",     "asset_script_js",     "application/javascript"),
    ("/bootstrap.css", "bootstrap.css", "asset_bootstrap_css", "text/css"),
    ("/bootstrap.js",  "bootstrap.js",  "asset_bootstrap_js",  "application/javascript"),
    ("/favicon.png",   "favicon.png",   "asset_favicon_png",   "image/png"),
]

def build_info():
    when = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S") + " UTC"
    return f"RP2350 LoRa iGate &mdash; Build date: {when}"

def main():
    lines = [
        "#pragma once",
        "// AUTO-GENERATED from data_embed/* by tools/gen_web_assets_rp2350.py — do not edit.",
        "// Gzipped SPA assets embedded in flash; served with Content-Encoding: gzip.",
        "#include <Arduino.h>",
        "#include <stddef.h>",
        "",
    ]
    for _, fname, arr, _ctype in ASSETS:
        with open(os.path.join(ROOT, "data_embed", fname), "rb") as f:
            content = f.read()
        if fname == "index.html":
            content = content.replace(b"%BUILD_INFO%", build_info().encode())
        gz = gzip.compress(content, compresslevel=9, mtime=0)
        body = ",".join(str(b) for b in gz)
        lines.append(f"static const uint8_t {arr}[] = {{{body}}};")
        print(f"  {fname:16s} {len(content):7d} -> {len(gz):7d} B gz")
    lines.append("")
    lines.append("struct WebAsset { const char *path; const uint8_t *data; size_t len; const char *ctype; };")
    lines.append("static const WebAsset WEB_ASSETS[] = {")
    for path, _f, arr, ctype in ASSETS:
        lines.append(f'  {{ "{path}", {arr}, sizeof({arr}), "{ctype}" }},')
    lines.append("};")
    lines.append("static const size_t WEB_ASSETS_N = sizeof(WEB_ASSETS)/sizeof(WEB_ASSETS[0]);")
    lines.append("")

    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {OUT} ({os.path.getsize(OUT)} bytes)")

if __name__ == "__main__":
    main()
