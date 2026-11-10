// Offline support: cache the game on first visit, then serve it from the cache.
const CACHE = 'lionel-564b4a6d40e1';
const FILES = ['./', 'index.html', 'vendor/three.min.js', 'vendor/cannon.min.js', 'manifest.webmanifest', 'icons/icon-180.png', 'icons/icon-192.png', 'icons/icon-512.png'];
self.addEventListener('install', e => { e.waitUntil(caches.open(CACHE).then(c => c.addAll(FILES)).then(() => self.skipWaiting())); });
self.addEventListener('activate', e => { e.waitUntil(caches.keys().then(ks => Promise.all(ks.filter(k => k !== CACHE).map(k => caches.delete(k)))).then(() => self.clients.claim())); });
self.addEventListener('fetch', e => {
  if (e.request.method !== 'GET') return;
  e.respondWith(caches.match(e.request, { ignoreSearch: true }).then(hit => hit || fetch(e.request).then(res => {
    // Keep same-site files and the Google web fonts, so the lettering also works offline.
    const host = new URL(e.request.url).hostname;
    const keep = host === location.hostname ? res.ok : (/^fonts\.(googleapis|gstatic)\.com$/.test(host) && (res.ok || res.type === 'opaque'));
    if (keep) { const copy = res.clone(); caches.open(CACHE).then(c => c.put(e.request, copy)); }
    return res;
  }).catch(() => e.request.mode === 'navigate' ? caches.match('index.html') : Response.error())));
});
