// Parse a v2 .tif with the editor's codec and write it back; the bytes must be identical.
//
//   node scripts/roundtrip.ts [file.tif]        (default: ../../Game/TClientCmd.tif)
import fs from 'node:fs';
import path from 'node:path';
import { parseTif, writeTif, type TifNode } from '../src/tif/tif.ts';

const file = path.resolve(process.argv[2] ?? path.join(import.meta.dirname, '../../../Game/TClientCmd.tif'));
const src = new Uint8Array(fs.readFileSync(file));

let t = performance.now();
const doc = parseTif(src);
const parseMs = performance.now() - t;
t = performance.now();
const out = writeTif(doc);
const writeMs = performance.now() - t;

let nodes = 0;
const count = (list: TifNode[]) => list.forEach(n => { nodes++; if (n.shared === undefined) count(n.children); });
count(doc.frames);
doc.shared.forEach(count);

const same = out.length === src.length && out.every((b, i) => b === src[i]);
console.log(`${path.basename(file)}: ${doc.frames.length} frames, ${doc.shared.length} shared lists, ${doc.fonts.length} fonts, ${nodes} stored nodes`);
console.log(`parse ${parseMs.toFixed(0)} ms, write ${writeMs.toFixed(0)} ms -> ${same ? 'byte-identical' : 'MISMATCH'}`);
process.exit(same ? 0 : 1);
