import fs from 'node:fs';
import path from 'node:path';
import type { IncomingMessage } from 'node:http';
import { defineConfig, type Plugin } from 'vite';
import react from '@vitejs/plugin-react';
import { parseTif } from './src/tif/tif.ts';

const repo = path.resolve(import.meta.dirname, '../..');
const gameDir = path.resolve(process.env.GAME_DIR ?? path.join(repo, 'Game'));
const tifPath = path.join(gameDir, 'TClientCmd.tif');
const idsPath = path.join(repo, 'Client', 'History Files', 'TClientID.h');

const readBody = (req: IncomingMessage) => new Promise<Buffer>((resolve, reject) => {
  const parts: Buffer[] = [];
  req.on('data', (c: Buffer) => parts.push(c));
  req.on('end', () => resolve(Buffer.concat(parts)));
  req.on('error', reject);
});

// Dev-server API: the editor reads and saves the game's own files in place.
//   GET /api/tif   Game/TClientCmd.tif          PUT /api/tif   overwrite it (must parse as v2)
//   GET /api/ids   Client/History Files/TClientID.h (names for ids)
function gameFiles(): Plugin {
  return {
    name: 'game-files',
    configureServer(server) {
      server.middlewares.use('/api/tif', async (req, res) => {
        try {
          if (req.method === 'GET') {
            res.setHeader('Content-Type', 'application/octet-stream');
            res.end(fs.readFileSync(tifPath));
          } else if (req.method === 'PUT') {
            const body = await readBody(req);
            parseTif(new Uint8Array(body));          // refuse anything the client loader would choke on
            fs.writeFileSync(tifPath, body);
            res.end(JSON.stringify({ path: tifPath, bytes: body.length }));
          } else {
            res.statusCode = 405;
            res.end();
          }
        } catch (e) {
          res.statusCode = 500;
          res.end((e as Error).message);
        }
      });
      server.middlewares.use('/api/ids', (_req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8');
        res.end(fs.existsSync(idsPath) ? fs.readFileSync(idsPath) : '');
      });
    },
  };
}

export default defineConfig({
  plugins: [react(), gameFiles()],
});
