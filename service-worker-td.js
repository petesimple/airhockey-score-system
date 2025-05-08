const tdCacheName = 'td-cache-v1';
const tdAssets = [
  './TD.html',
  './manifestTD.json',
  './iconTD-192.png',
  './iconTD-512.png',
  './TDlogo.png',
  'https://cdn.jsdelivr.net/npm/qrcode/build/qrcode.min.js'
];

self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(tdCacheName).then(cache => cache.addAll(tdAssets))
  );
});

self.addEventListener('fetch', event => {
  event.respondWith(
    caches.match(event.request).then(res => res || fetch(event.request))
  );
});
