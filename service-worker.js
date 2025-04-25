
self.addEventListener('install', (e) => {
  e.waitUntil(
    caches.open('airhockey-cache').then((cache) => {
      return cache.addAll([
        './',
        './index.html',
        './index_v1_3.html',
        './manifest.json',
        './service-worker.js',
        './icon-192.png',
        './icon-512.png'
      ]);
    })
  );
});

self.addEventListener('fetch', (e) => {
  e.respondWith(
    caches.match(e.request).then((response) => {
      return response || fetch(e.request);
    })
  );
});
