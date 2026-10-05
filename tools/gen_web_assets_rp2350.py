#!/usr/bin/env python3
# Generate src/rp2350/web_assets.h from data_embed/* for the RP2350 iGate build.
# Each SPA asset is gzipped and embedded as a byte array; eth_web.cpp serves them
# with Content-Encoding: gzip. Run after editing any data_embed/* file:
#
#     python tools/gen_web_assets_rp2350.py
#
# (Standalone — unlike tools/compress.py, which is the ESP32 PlatformIO script
# that emits .gz files instead of this C header.)

import base64
import gzip
import os
import datetime
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT  = os.path.join(ROOT, "src", "rp2350", "web_assets.h")

# The page and everything it loads at start-up go out as ONE response: the
# stylesheets, scripts and favicon index.html links are inlined into it. The
# W5100S has 4 sockets and APRS-IS and syslog keep two of them open (NTP takes a
# third while it syncs), so the web server can only take a connection or two at
# a time; the six parallel requests a
# browser fires for separate .css/.js files got reset (ERR_CONNECTION_REFUSED),
# and a lost script.js left the bare page with no settings loaded until a reload.
INLINE_CSS = ["bootstrap.css", "style.css", "leaflet.css"]
INLINE_JS  = ["bootstrap.js", "leaflet.js", "script.js"]
FAVICON    = "favicon.png"

# (url path, source file, C array name, content-type)
ASSETS = [
    ("/",              "index.html",    "asset_index_html",    "text/html"),
    ("/aprs-symbols-24-0.png", "aprs-symbols-24-0.png", "asset_sym24_0", "image/png"),
    ("/aprs-symbols-24-1.png", "aprs-symbols-24-1.png", "asset_sym24_1", "image/png"),
    ("/aprs-symbols-24-2.png", "aprs-symbols-24-2.png", "asset_sym24_2", "image/png"),
]

def build_info():
    when = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S") + " UTC"
    return f"RP2350 LoRa iGate &mdash; Build date: {when}"

def read_embed(fname):
    with open(os.path.join(ROOT, "data_embed", fname), "rb") as f:
        return f.read()

def strip_source_map(content):
    # the .map files are not embedded: drop the reference so devtools don't 404 on it
    return re.sub(rb"\n?/[/*]# sourceMappingURL=[^\n]*", b"", content)

def replace_once(html, old, new):
    if html.count(old) != 1:
        raise SystemExit(f"index.html: expected exactly one {old.decode()!r}")
    return html.replace(old, new)

def inline_page(html):
    for fname in INLINE_CSS:
        css = strip_source_map(read_embed(fname))
        if b"</style" in css.lower():
            raise SystemExit(f"{fname}: contains </style, cannot be inlined")
        html = replace_once(html, f'<link rel="stylesheet" href="/{fname}" />'.encode(),
                            b"<style>\n" + css + b"\n</style>")
    for fname in INLINE_JS:
        js = strip_source_map(read_embed(fname))
        if b"</script" in js.lower() or b"<!--" in js:    # would end / confuse the inline <script>
            raise SystemExit(f"{fname}: contains </script or <!--, cannot be inlined")
        html = replace_once(html, f'<script src="/{fname}"></script>'.encode(),
                            b"<script>\n" + js + b"\n</script>")
    icon = base64.b64encode(read_embed(FAVICON))
    html = replace_once(html, f'<link rel="icon" href="/{FAVICON}" type="image/x-icon">'.encode(),
                        b'<link rel="icon" href="data:image/png;base64,' + icon + b'" type="image/png">')
    return html

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
        content = read_embed(fname)
        if fname == "index.html":
            content = content.replace(b"%BUILD_INFO%", build_info().encode())
            content = inline_page(content)
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
