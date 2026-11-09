#!/usr/bin/env python3
"""Build docs/index.html: a standalone copy of the browser game for GitHub Pages.

The game source (lionel-jungle-fury/index.html) is written for Claude artifacts, which add the
<html>/<head> wrapper at publish time. Opened straight in Safari, a page needs that wrapper itself,
and it must be a normal web page for iOS to allow motion look (the phone's orientation sensors).

Run after changing the game:  python3 tools/build_pages.py
"""
from pathlib import Path

root = Path(__file__).resolve().parent.parent
game = (root / "lionel-jungle-fury" / "index.html").read_text(encoding="utf-8")

head = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="Lionel">
<meta name="theme-color" content="#0b1710">
<style>:root{padding-top:env(safe-area-inset-top);padding-bottom:env(safe-area-inset-bottom)}body{margin:0}[hidden]{display:none!important}</style>
</head>
<body>
"""
out = head + game + "\n</body>\n</html>\n"
(root / "docs" / "index.html").write_text(out, encoding="utf-8")
(root / "docs" / ".nojekyll").write_text("", encoding="utf-8")
print(f"wrote docs/index.html ({len(out):,} bytes)")
