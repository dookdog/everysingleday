#!/usr/bin/env python3
"""Build LilyLeap.html: one self-contained file you can send to anyone.

Takes the game source (index.html), wraps it in a full HTML document, and
embeds the two fonts as base64 so it works offline with no other files.

    python3 build_standalone.py
"""
import base64
import pathlib
import re

HERE = pathlib.Path(__file__).resolve().parent
SRC = HERE / "index.html"
OUT = HERE / "LilyLeap.html"


def font_face(family, path, weight):
    data = base64.b64encode((HERE / "fonts" / path).read_bytes()).decode()
    return (
        f"@font-face{{font-family:'{family}';font-style:normal;font-weight:{weight};"
        f"font-display:swap;src:url(data:font/woff2;base64,{data}) format('woff2')}}"
    )


src = SRC.read_text(encoding="utf-8")

# Drop the Google Fonts links; the fonts get embedded below instead.
src = re.sub(r'<link rel="(preconnect|stylesheet)" href="https://fonts\.[^"]*"[^>]*>\n?', "", src)

title_match = re.match(r"\s*(<title>.*?</title>)\n?", src)
title = title_match.group(1)
body = src[title_match.end():]

# Split the leading <style> into <head>; everything else goes in <body>.
style_end = body.index("</style>") + len("</style>")
style, rest = body[:style_end], body[style_end:].lstrip("\n")

fonts = "<style>\n" + "\n".join([
    font_face("Chewy", "Chewy-latin.woff2", "400"),
    font_face("Nunito", "Nunito-latin.woff2", "700 900"),
]) + "\n</style>"

doc = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="Lily Leap">
<meta name="theme-color" content="#1f5d73">
<meta name="description" content="Hop across the blooms and snap up every fly in the pond.">
{title}
{fonts}
{style}
</head>
<body>
{rest}</body>
</html>
"""

OUT.write_text(doc, encoding="utf-8")
print(f"Wrote {OUT.name} ({OUT.stat().st_size / 1024:.0f} KB)")
