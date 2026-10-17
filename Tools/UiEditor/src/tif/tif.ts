// TClientCmd.tif v2 reader/writer -- the compiled UI loaded by TCMLParser::Load (Lib/Own/TCML).
//
//   i32 -2                                   magic (TCML_TIF_V2_MAGIC)
//   i32 sharedCount, per list: i32 n + n nodes   child lists shared by several image lists
//   i32 frameCount, frames                   a node whose child count is < 0 uses shared list (-count-1)
//   i32 fontCount, fonts                     TCML_LOGFONT minus its `next` pointer + 4 junk bytes = 72 B
//
// Node (TCMLParser::LoadFRAME, little-endian, packed):
//   u32 id; u8 type; u32 menu[12]; u32 images[2]; u32 tooltipFont, font, style, color, sound;
//   i32 marginH, marginV, x, y, width, height; u8 display, align; TSATR ex (68 B);
//   i32 len + tooltip; i32 len + text; i32 childCount; children
import { decodeCP949, encodeCP949 } from './cp949.ts';

export const TIF_V2_MAGIC = -2;
export const MENU_COUNT = 12;
export const EX_SIZE = 68;
export const FONT_SIZE = 72;
export const ID_NULL = 0xFFFFFFFF;

export const TYPE_NAMES: Record<number, string> = {
  0x00: 'Component', 0x01: 'Button', 0x02: 'Scroll', 0x03: 'Edit', 0x04: 'Combo', 0x05: 'List',
  0x06: 'Gauge', 0x08: 'TabCtrl', 0x09: 'Frame', 0x0A: 'ImageList', 0x0B: 'Meter', 0x0C: 'HttpCtrl',
};
export const TYPE_IMAGELIST = 0x0A;

export const ALIGN = { CUSTOM: 0x01, LEFT: 0x02, RIGHT: 0x04, CENTER: 0x08 } as const;
export const BLT = { FIT: 0x01, CENTER: 0x02, TILE: 0x04 } as const;

export interface TifNode {
  id: number;
  type: number;
  menu: number[];
  images: [number, number];
  tooltipFont: number;
  font: number;
  style: number;
  color: number;
  sound: number;
  marginH: number;
  marginV: number;
  x: number;
  y: number;
  width: number;
  height: number;
  display: number;
  align: number;
  ex: Uint8Array;
  tooltip: string;
  text: string;
  children: TifNode[];
  /** index into TifDoc.shared when the children are a shared list (read-only in the editor) */
  shared?: number;
  /** original bytes of tooltip/text, written back untouched while the decoded string is unchanged */
  raw: { tooltip: Uint8Array; text: Uint8Array; tooltipStr: string; textStr: string };
}

export interface TifFont {
  height: number;
  width: number;
  weight: number;
  italic: boolean;
  underline: boolean;
  face: string;
  id: number;
  color: number;
  bytes: Uint8Array;
}

export interface TifDoc {
  shared: TifNode[][];
  frames: TifNode[];
  fonts: TifFont[];
}

class Reader {
  pos = 0;
  readonly view: DataView;
  readonly buf: Uint8Array;
  constructor(buf: Uint8Array) {
    this.buf = buf;
    this.view = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  }
  u8() { return this.buf[this.pos++]; }
  i32() { const v = this.view.getInt32(this.pos, true); this.pos += 4; return v; }
  u32() { const v = this.view.getUint32(this.pos, true); this.pos += 4; return v; }
  bytes(n: number) { const v = this.buf.slice(this.pos, this.pos + n); this.pos += n; return v; }
}

class Writer {
  private chunks: Uint8Array[] = [];
  private cur = new Uint8Array(1 << 20);
  private view = new DataView(this.cur.buffer);
  private pos = 0;
  private room(n: number) {
    if (this.pos + n <= this.cur.length) return;
    this.chunks.push(this.cur.subarray(0, this.pos));
    this.cur = new Uint8Array(Math.max(1 << 20, n));
    this.view = new DataView(this.cur.buffer);
    this.pos = 0;
  }
  u8(v: number) { this.room(1); this.cur[this.pos++] = v; }
  i32(v: number) { this.room(4); this.view.setInt32(this.pos, v, true); this.pos += 4; }
  u32(v: number) { this.room(4); this.view.setUint32(this.pos, v >>> 0, true); this.pos += 4; }
  bytes(b: Uint8Array) { this.room(b.length); this.cur.set(b, this.pos); this.pos += b.length; }
  finish() {
    this.chunks.push(this.cur.subarray(0, this.pos));
    const total = this.chunks.reduce((n, c) => n + c.length, 0);
    const out = new Uint8Array(total);
    let o = 0;
    for (const c of this.chunks) { out.set(c, o); o += c.length; }
    return out;
  }
}

export function parseTif(buf: Uint8Array): TifDoc {
  const r = new Reader(buf);
  if (r.i32() !== TIF_V2_MAGIC) throw new Error('Not a v2 .tif (run Tools/TifDedupe on v1 files first)');

  const shared: TifNode[][] = [];
  const readList = (n: number) => { const kids: TifNode[] = []; for (let i = 0; i < n; i++) kids.push(readNode()); return kids; };

  function readNode(): TifNode {
    const id = r.u32();
    const type = r.u8();
    const menu = Array.from({ length: MENU_COUNT }, () => r.u32());
    const images: [number, number] = [r.u32(), r.u32()];
    const tooltipFont = r.u32(), font = r.u32(), style = r.u32(), color = r.u32(), sound = r.u32();
    const marginH = r.i32(), marginV = r.i32(), x = r.i32(), y = r.i32(), width = r.i32(), height = r.i32();
    const display = r.u8(), align = r.u8();
    const ex = r.bytes(EX_SIZE);
    const tooltipBytes = r.bytes(Math.max(r.i32(), 0));
    const textBytes = r.bytes(Math.max(r.i32(), 0));
    const tooltip = decodeCP949(tooltipBytes), text = decodeCP949(textBytes);
    const n = r.i32();
    const node: TifNode = {
      id, type, menu, images, tooltipFont, font, style, color, sound, marginH, marginV, x, y, width, height,
      display, align, ex, tooltip, text, children: [],
      raw: { tooltip: tooltipBytes, text: textBytes, tooltipStr: tooltip, textStr: text },
    };
    if (n < 0) {
      node.shared = -(n + 1);
      node.children = shared[node.shared] ?? [];
    } else node.children = readList(n);
    return node;
  }

  for (let i = 0, n = r.i32(); i < n; i++) shared.push(readList(r.i32()));
  const frames = readList(r.i32());

  const fonts: TifFont[] = [];
  for (let i = 0, n = r.i32(); i < n; i++) {
    const bytes = r.bytes(FONT_SIZE);
    const v = new DataView(bytes.buffer);
    const faceBytes = bytes.subarray(28, 60);
    const end = faceBytes.indexOf(0);
    fonts.push({
      height: v.getInt32(0, true), width: v.getInt32(4, true), weight: v.getInt32(16, true),
      italic: bytes[20] !== 0, underline: bytes[21] !== 0,
      face: decodeCP949(faceBytes.subarray(0, end < 0 ? 32 : end)),
      id: v.getUint32(60, true), color: v.getUint32(64, true), bytes,
    });
  }
  if (r.pos !== buf.length) throw new Error(`Trailing bytes after fonts: ${buf.length - r.pos}`);
  return { shared, frames, fonts };
}

export function writeTif(doc: TifDoc): Uint8Array {
  const w = new Writer();
  const str = (s: string, raw: Uint8Array, orig: string) => {
    const b = s === orig ? raw : encodeCP949(s);
    w.i32(b.length);
    w.bytes(b);
  };
  const writeNode = (nd: TifNode) => {
    w.u32(nd.id); w.u8(nd.type);
    nd.menu.forEach(m => w.u32(m));
    w.u32(nd.images[0]); w.u32(nd.images[1]);
    w.u32(nd.tooltipFont); w.u32(nd.font); w.u32(nd.style); w.u32(nd.color); w.u32(nd.sound);
    w.i32(nd.marginH); w.i32(nd.marginV); w.i32(nd.x); w.i32(nd.y); w.i32(nd.width); w.i32(nd.height);
    w.u8(nd.display); w.u8(nd.align);
    w.bytes(nd.ex);
    str(nd.tooltip, nd.raw.tooltip, nd.raw.tooltipStr);
    str(nd.text, nd.raw.text, nd.raw.textStr);
    if (nd.shared !== undefined) { w.i32(-(nd.shared + 1)); return; }
    w.i32(nd.children.length);
    nd.children.forEach(writeNode);
  };

  w.i32(TIF_V2_MAGIC);
  w.i32(doc.shared.length);
  for (const list of doc.shared) { w.i32(list.length); list.forEach(writeNode); }
  w.i32(doc.frames.length);
  doc.frames.forEach(writeNode);
  w.i32(doc.fonts.length);
  for (const f of doc.fonts) w.bytes(f.bytes);
  return w.finish();
}

export const isNullImage = (id: number) => id === ID_NULL || id === 0;
