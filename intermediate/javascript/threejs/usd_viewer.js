/**
 * @fileoverview Simple HTTP server to serve the Three.js USD demo
 */
const http = require('http');
const fs = require('fs');
const path = require('path');

const MIME_TYPES = {
    '.html': 'text/html',
    '.js': 'application/javascript',
    '.mjs': 'application/javascript',
    '.css': 'text/css',
    '.json': 'application/json',
    '.usdz': 'model/vnd.usdz+zip',
    '.usda': 'model/vnd.usda',
    '.usdc': 'application/octet-stream',
    '.usd': 'application/octet-stream',
};

const PORT = 8080;
const ROOT = __dirname;

const server = http.createServer((req, res) => {
    let filePath = path.join(ROOT, req.url === '/' ? 'index.html' : req.url);
    filePath = path.normalize(filePath);

    // Prevent directory traversal
    if (!filePath.startsWith(ROOT)) {
        res.writeHead(403);
        res.end('Forbidden');
        return;
    }

    const ext = path.extname(filePath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';

    fs.readFile(filePath, (err, data) => {
        if (err) {
            res.writeHead(404);
            res.end('Not found');
            return;
        }
        res.writeHead(200, { 'Content-Type': contentType });
        res.end(data);
    });
});

server.listen(PORT, () => {
    console.log(`Three.js USD demo server running at http://localhost:${PORT}`);
    console.log('Place a .usdz file in this directory and load it via the UI.');
});
