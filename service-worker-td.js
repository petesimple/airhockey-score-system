const CACHE_NAME = 'td-static-v4';
const ASSETS = [
  './',
  './TD.html',
  './manifestTD.json',
  './iconTD-192.png',
  './TDlogo.png',
  'https://cdn.jsdelivr.net/npm/qrcode/build/qrcode.min.js',
];

self.addEventListener('install', (event) => {
  event.waitUntil(caches.open(CACHE_NAME).then(c => c.addAll(ASSETS)).catch(() => {}));
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then(keys => Promise.all(keys.map(k => (k !== CACHE_NAME ? caches.delete(k) : null))))
  );
  self.clients.claim();
});

// Only handle same-origin GET. Never touch cross-origin or non-GET.
self.addEventListener('fetch', (event) => {
  try {
    const req = event.request;
    const url = new URL(req.url);
    if (req.method !== 'GET' || url.origin !== self.location.origin) return;
    event.respondWith(caches.match(req).then(cached => cached || fetch(req)));
  } catch (_) {}
});
