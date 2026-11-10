#!/usr/bin/env python3
"""Build the standalone copies of the browser game.

The game source (lionel-jungle-fury/index.html) is written for Claude artifacts, which add the
<html>/<head> wrapper at publish time and load three.js and cannon.js from a CDN. This script makes
two self-contained copies with those libraries bundled, so they work with no network:

  docs/      Installable web app for GitHub Pages. In Safari: Share -> Add to Home Screen. It gets
             a home-screen icon, opens full-screen and works offline (service worker).
  ios/www/   The same game for the native iPhone app in ios/, loaded from the app bundle.

Run after changing the game:  python3 tools/build_pages.py
"""
import hashlib
import shutil
from pathlib import Path

root = Path(__file__).resolve().parent.parent
game = (root / "lionel-jungle-fury" / "index.html").read_text(encoding="utf-8")

CDN = {
    "https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js": "vendor/three.min.js",
    "https://cdnjs.cloudflare.com/ajax/libs/cannon.js/0.6.2/cannon.min.js": "vendor/cannon.min.js",
}
for url, local in CDN.items():
    assert url in game, f"expected CDN script not found: {url}"
    game = game.replace(url, local)

VIEWPORT = '<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">'
BASE_STYLE = "<style>:root{padding-top:env(safe-area-inset-top);padding-bottom:env(safe-area-inset-bottom)}body{margin:0}[hidden]{display:none!important}</style>"


def page(extra_head: str, extra_body: str = "") -> str:
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
{VIEWPORT}
<meta name="theme-color" content="#0b1710">
{extra_head}
{BASE_STYLE}
</head>
<body>
{game}
{extra_body}
</body>
</html>
"""


def copy_vendor(dest: Path) -> None:
    (dest / "vendor").mkdir(parents=True, exist_ok=True)
    for name in ("three.min.js", "cannon.min.js", "three.LICENSE", "cannon.LICENSE"):
        shutil.copyfile(root / "web-vendor" / name, dest / "vendor" / name)


# ---- docs/: installable web app ----
docs = root / "docs"
docs.mkdir(exist_ok=True)
copy_vendor(docs)
(docs / "icons").mkdir(exist_ok=True)
for size in (180, 192, 512):
    shutil.copyfile(root / "web-icons" / f"icon-{size}.png", docs / "icons" / f"icon-{size}.png")

web = page(
    """<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="Lionel">
<link rel="manifest" href="manifest.webmanifest">
<link rel="apple-touch-icon" href="icons/icon-180.png">
<link rel="icon" type="image/png" href="icons/icon-192.png">""",
    """<script>if('serviceWorker' in navigator&&(location.protocol==='https:'||location.hostname==='localhost')){navigator.serviceWorker.register('sw.js').catch(function(){})}</script>""",
)
(docs / "index.html").write_text(web, encoding="utf-8")
(docs / ".nojekyll").write_text("", encoding="utf-8")
(docs / "manifest.webmanifest").write_text(
    """{
  "name": "Lionel: Jungle Fury",
  "short_name": "Lionel",
  "description": "First-person jungle shooter. Lionel holds the Temple of the Golden Jaguar.",
  "start_url": "./",
  "scope": "./",
  "display": "fullscreen",
  "orientation": "any",
  "background_color": "#0b1710",
  "theme_color": "#0b1710",
  "icons": [
    { "src": "icons/icon-192.png", "sizes": "192x192", "type": "image/png" },
    { "src": "icons/icon-512.png", "sizes": "512x512", "type": "image/png" },
    { "src": "icons/icon-512.png", "sizes": "512x512", "type": "image/png", "purpose": "maskable" }
  ]
}
""",
    encoding="utf-8",
)
# The cache name changes whenever the game or its libraries change, so players get updates.
digest = hashlib.sha256(web.encode() + (root / "web-vendor" / "three.min.js").read_bytes() + (root / "web-vendor" / "cannon.min.js").read_bytes()).hexdigest()[:12]
(docs / "sw.js").write_text(
    f"""// Offline support: cache the game on first visit, then serve it from the cache.
const CACHE = 'lionel-{digest}';
const FILES = ['./', 'index.html', 'vendor/three.min.js', 'vendor/cannon.min.js', 'manifest.webmanifest', 'icons/icon-180.png', 'icons/icon-192.png', 'icons/icon-512.png'];
self.addEventListener('install', e => {{ e.waitUntil(caches.open(CACHE).then(c => c.addAll(FILES)).then(() => self.skipWaiting())); }});
self.addEventListener('activate', e => {{ e.waitUntil(caches.keys().then(ks => Promise.all(ks.filter(k => k !== CACHE).map(k => caches.delete(k)))).then(() => self.clients.claim())); }});
self.addEventListener('fetch', e => {{
  if (e.request.method !== 'GET') return;
  e.respondWith(caches.match(e.request, {{ ignoreSearch: true }}).then(hit => hit || fetch(e.request).then(res => {{
    // Keep same-site files and the Google web fonts, so the lettering also works offline.
    const host = new URL(e.request.url).hostname;
    const keep = host === location.hostname ? res.ok : (/^fonts\\.(googleapis|gstatic)\\.com$/.test(host) && (res.ok || res.type === 'opaque'));
    if (keep) {{ const copy = res.clone(); caches.open(CACHE).then(c => c.put(e.request, copy)); }}
    return res;
  }}).catch(() => e.request.mode === 'navigate' ? caches.match('index.html') : Response.error())));
}});
""",
    encoding="utf-8",
)

# ---- ios/www/: bundled into the native app ----
www = root / "ios" / "www"
www.mkdir(parents=True, exist_ok=True)
copy_vendor(www)
(www / "index.html").write_text(page(""), encoding="utf-8")

print(f"wrote docs/ (web app, cache lionel-{digest}) and ios/www/ ({len(web):,} bytes each)")
