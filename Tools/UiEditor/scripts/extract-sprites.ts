// Bake the UI sprite pool (Game/Index/TClientI.IDX + Data/Img/*Img.TIM + *Src.TIS) into browser-friendly
// files: one PNG per texture page and a JSON sprite table.
//
//   node scripts/extract-sprites.ts [gameDir]        (default: ../../Game)
//
// Output (public/sprites/):
//   pages/<pageId>.png
//   sprites.json   { pages: { [pageId]: [w, h] }, sets: { [spriteId]: { frames: imageId[], color } },
//                    images: { [imageId]: { w, h, parts: [{ p, x, y, w, h, sx, sy, sw, sh }] } } }
//
// Mirrors CTachyonRes::LoadIMGBUF / LoadIMG / ComplateIMG and CD3DImage::Load (Engine Lib):
// a TIM chunk is an IMAGESET (frames = ids of other TIM chunks that carry a CD3DImage) and a CD3DImage is
// parts of TIS texture pages placed by 4 TVERTEX (PosX, PosY, PosZ, RHW, U, V) each.
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';

const gameDir = path.resolve(process.argv[2] ?? path.join(import.meta.dirname, '../../../Game'));
const outDir = path.join(import.meta.dirname, '../public/sprites');

class Reader {
  pos = 0;
  readonly buf: Buffer;
  constructor(buf: Buffer) { this.buf = buf; }
  get eof() { return this.pos >= this.buf.length; }
  u8() { return this.buf[this.pos++]; }
  i32() { const v = this.buf.readInt32LE(this.pos); this.pos += 4; return v; }
  u32() { const v = this.buf.readUInt32LE(this.pos); this.pos += 4; return v; }
  f32() { const v = this.buf.readFloatLE(this.pos); this.pos += 4; return v; }
  bytes(n: number) { const v = this.buf.subarray(this.pos, this.pos + n); this.pos += n; return v; }
  str() { return this.bytes(this.i32()).toString('latin1'); }
  // CTachyonUncompressor::FromFile: [u32 origSize][u32 compSize][zlib]
  block() { const orig = this.u32(); const len = this.u32(); return zlib.inflateSync(this.bytes(len), { finishFlush: zlib.constants.Z_SYNC_FLUSH }).subarray(0, orig); }
}

// ---------- DDS -> RGBA ----------
function rgb565(c: number): [number, number, number] {
  return [((c >> 11) & 31) * 255 / 31 | 0, ((c >> 5) & 63) * 255 / 63 | 0, (c & 31) * 255 / 31 | 0];
}

function decodeColorBlock(src: Buffer, o: number, out: Uint8Array, w: number, bx: number, by: number, h: number, dxt1: boolean, alpha: (i: number) => number) {
  const c0 = src.readUInt16LE(o), c1 = src.readUInt16LE(o + 2);
  const p0 = rgb565(c0), p1 = rgb565(c1);
  const pal: number[][] = [[...p0, 255], [...p1, 255]];
  if (!dxt1 || c0 > c1) {
    pal.push([0, 1, 2].map(k => (2 * p0[k] + p1[k]) / 3 | 0).concat(255));
    pal.push([0, 1, 2].map(k => (p0[k] + 2 * p1[k]) / 3 | 0).concat(255));
  } else {
    pal.push([0, 1, 2].map(k => (p0[k] + p1[k]) / 2 | 0).concat(255));
    pal.push([0, 0, 0, 0]);
  }
  const bits = src.readUInt32LE(o + 4);
  for (let i = 0; i < 16; i++) {
    const x = bx + (i & 3), y = by + (i >> 2);
    if (x >= w || y >= h) continue;
    const c = pal[(bits >>> (i * 2)) & 3];
    const d = (y * w + x) * 4;
    out[d] = c[0]; out[d + 1] = c[1]; out[d + 2] = c[2];
    out[d + 3] = dxt1 ? c[3] : alpha(i);
  }
}

function decodeDDS(buf: Buffer): { w: number; h: number; rgba: Uint8Array } {
  if (buf.toString('latin1', 0, 4) !== 'DDS ') throw new Error('not a DDS');
  const h = buf.readUInt32LE(12), w = buf.readUInt32LE(16);
  const pfFlags = buf.readUInt32LE(80), fourCC = buf.toString('latin1', 84, 88);
  const rgba = new Uint8Array(w * h * 4);
  let o = 128;
  if (pfFlags & 4) {
    const dxt = fourCC;
    for (let by = 0; by < h; by += 4)
      for (let bx = 0; bx < w; bx += 4) {
        if (dxt === 'DXT1') { decodeColorBlock(buf, o, rgba, w, bx, by, h, true, () => 255); o += 8; }
        else if (dxt === 'DXT2' || dxt === 'DXT3') {
          const a = o;
          decodeColorBlock(buf, o + 8, rgba, w, bx, by, h, false, i => ((buf[a + (i >> 1)] >> ((i & 1) * 4)) & 15) * 17);
          o += 16;
        } else if (dxt === 'DXT4' || dxt === 'DXT5') {
          const a0 = buf[o], a1 = buf[o + 1];
          const ap = [a0, a1];
          if (a0 > a1) for (let k = 1; k < 7; k++) ap.push(((7 - k) * a0 + k * a1) / 7 | 0);
          else { for (let k = 1; k < 5; k++) ap.push(((5 - k) * a0 + k * a1) / 5 | 0); ap.push(0, 255); }
          const lo = buf.readUInt32LE(o + 2), hi = buf.readUInt16LE(o + 6);
          const idx = (i: number) => { const bit = i * 3; return bit < 30 ? (lo >>> bit) & 7 : bit === 30 ? ((lo >>> 30) | (hi << 2)) & 7 : (hi >>> (bit - 32)) & 7; };
          decodeColorBlock(buf, o + 8, rgba, w, bx, by, h, false, i => ap[idx(i)]);
          o += 16;
        } else throw new Error('unsupported fourCC ' + dxt);
      }
    return { w, h, rgba };
  }
  // uncompressed: generic bitmask decode
  const bpp = buf.readUInt32LE(88) / 8;
  const masks = [92, 96, 100, 104].map(k => buf.readUInt32LE(k)); // R G B A
  const hasAlpha = (pfFlags & 1) !== 0;
  const chan = (v: number, m: number) => {
    if (!m) return 255;
    let s = 0; while (((m >>> s) & 1) === 0) s++;      // unsigned shifts: 0xFF000000 is negative as int32
    const max = m >>> s;
    return Math.round(((v & m) >>> s) * 255 / max);
  };
  for (let i = 0; i < w * h; i++, o += bpp) {
    const v = bpp === 4 ? buf.readUInt32LE(o) : bpp === 3 ? buf.readUIntLE(o, 3) : buf.readUInt16LE(o);
    rgba[i * 4] = chan(v, masks[0]);
    rgba[i * 4 + 1] = chan(v, masks[1]);
    rgba[i * 4 + 2] = chan(v, masks[2]);
    rgba[i * 4 + 3] = hasAlpha ? chan(v, masks[3]) : 255;
  }
  return { w, h, rgba };
}

// ---------- RGBA -> PNG ----------
const crcTable = new Int32Array(256).map((_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; return c; });
function crc32(b: Buffer) { let c = -1; for (const x of b) c = crcTable[(c ^ x) & 255] ^ (c >>> 8); return (c ^ -1) >>> 0; }
function chunk(type: string, data: Buffer) {
  const len = Buffer.alloc(4); len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type, 'latin1'), data]);
  const crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td));
  return Buffer.concat([len, td, crc]);
}
function encodePNG(w: number, h: number, rgba: Uint8Array) {
  const raw = Buffer.alloc((w * 4 + 1) * h);
  for (let y = 0; y < h; y++) Buffer.from(rgba.buffer, rgba.byteOffset + y * w * 4, w * 4).copy(raw, y * (w * 4 + 1) + 1);
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 6;
  return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw, { level: 6 })), chunk('IEND', Buffer.alloc(0))]);
}

// ---------- main ----------
const readIndex = (file: string) => {
  const r = new Reader(fs.readFileSync(path.join(gameDir, 'Index', file)));
  const count = r.i32(), total = r.i32();
  const files: string[] = [];
  for (let i = 0; i < count; i++) files.push(r.str());
  return { r, files, total };
};

fs.rmSync(outDir, { recursive: true, force: true });
fs.mkdirSync(path.join(outDir, 'pages'), { recursive: true });

// texture pages (IMGBUF): TClientList.LST -> *Src.TIS
const pageSize = new Map<number, { w: number; h: number }>();
for (const file of readIndex('TClientList.LST').files) {
  const r = new Reader(fs.readFileSync(path.join(gameDir, 'Data', file)));
  while (!r.eof) {
    const c = new Reader(r.block());
    const id = c.u32(); c.u8(); const size = c.u32(); const len = c.u32();
    let data = c.bytes(len);
    if (size) data = zlib.inflateSync(data);
    try {
      const { w, h, rgba } = decodeDDS(data);
      fs.writeFileSync(path.join(outDir, 'pages', `${id}.png`), encodePNG(w, h, rgba));
      pageSize.set(id, { w, h });
    } catch (e) {
      console.warn(`page ${id} (${file}): ${(e as Error).message}`);
    }
  }
  console.log(`${file}: ${pageSize.size} pages so far`);
}

// image sets + images: TClientI.IDX -> *Img.TIM
type Part = { p: number; x: number; y: number; w: number; h: number; sx: number; sy: number; sw: number; sh: number };
const { r: idx, files: timFiles, total } = readIndex('TClientI.IDX');
const byPos = timFiles.map(file => {
  const map = new Map<number, { frames: number[]; color: number; image?: { w: number; h: number; parts: Part[] } }>();
  const r = new Reader(fs.readFileSync(path.join(gameDir, 'Data', file)));
  while (!r.eof) {
    const pos = r.pos;
    const c = new Reader(r.block());
    const frames: number[] = [];
    for (let n = c.i32(), i = 0; i < n; i++) frames.push(c.u32());
    let color = 0xFFFFFFFF;
    for (let n = c.i32(), i = 0; i < n; i++) { c.u32(); const clr = c.u32(); if (i === 0) color = clr; }
    c.u32(); c.u8();
    const size = c.u32();
    let image;
    if (size > 0) {
      const d = new Reader(c.bytes(size));
      const nParts = d.i32(), w = d.i32(), h = d.i32();
      const parts: Part[] = [];
      for (let i = 0; i < nParts; i++) {
        const p = d.u32();
        const v = [0, 1, 2, 3].map(() => { const x = d.f32(), y = d.f32(); d.f32(); d.f32(); return { x, y, u: d.f32(), v: d.f32() }; });
        const pg = pageSize.get(p);
        if (!pg) continue;
        const xs = v.map(q => q.x), ys = v.map(q => q.y), us = v.map(q => q.u), vs = v.map(q => q.v);
        const x0 = Math.min(...xs), y0 = Math.min(...ys), u0 = Math.min(...us), v0 = Math.min(...vs);
        parts.push({
          p, x: Math.floor(x0), y: Math.floor(y0),
          w: Math.round(Math.max(...xs) - x0), h: Math.round(Math.max(...ys) - y0),
          sx: Math.round(u0 * pg.w), sy: Math.round(v0 * pg.h),
          sw: Math.round((Math.max(...us) - u0) * pg.w), sh: Math.round((Math.max(...vs) - v0) * pg.h),
        });
      }
      image = { w, h, parts };
    }
    map.set(pos, { frames, color, image });
  }
  return map;
});

const sets: Record<number, { frames: number[]; color: number }> = {};
const images: Record<number, { w: number; h: number; parts: Part[] }> = {};
for (let i = 0; i < total; i++) {
  const id = idx.u32(), fileId = idx.u32(), pos = idx.u32();
  const e = byPos[fileId]?.get(pos);
  if (!e) continue;
  sets[id] = { frames: e.frames, color: e.color };
  if (e.image) images[id] = e.image;
}

const pages = Object.fromEntries([...pageSize].map(([id, s]) => [id, [s.w, s.h]]));
fs.writeFileSync(path.join(outDir, 'sprites.json'), JSON.stringify({ pages, sets, images }));
console.log(`${Object.keys(sets).length} sprite sets, ${Object.keys(images).length} images, ${pageSize.size} pages -> ${outDir}`);
