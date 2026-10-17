import { parseTif, writeTif, type TifDoc } from './tif/tif.ts';

export interface SpritePart { p: number; x: number; y: number; w: number; h: number; sx: number; sy: number; sw: number; sh: number }
export interface SpriteImage { w: number; h: number; parts: SpritePart[] }
export interface Sprites {
  pages: Record<number, [number, number]>;
  sets: Record<number, { frames: number[]; color: number }>;
  images: Record<number, SpriteImage>;
}

export interface IdNames {
  frame: Map<number, string>;
  ctrl: Map<number, string>;
}

export async function loadTif(): Promise<TifDoc> {
  const res = await fetch('/api/tif');
  if (!res.ok) throw new Error(`GET /api/tif: ${res.status} ${await res.text()}`);
  return parseTif(new Uint8Array(await res.arrayBuffer()));
}

export async function saveTif(doc: TifDoc): Promise<string> {
  const res = await fetch('/api/tif', { method: 'PUT', body: writeTif(doc) as BodyInit });
  const text = await res.text();
  if (!res.ok) throw new Error(text);
  const { path, bytes } = JSON.parse(text);
  return `Saved ${(bytes / 1e6).toFixed(2)} MB to ${path}`;
}

export async function loadSprites(): Promise<Sprites | null> {
  const res = await fetch('/sprites/sprites.json');
  if (!res.ok || !res.headers.get('content-type')?.includes('json')) return null;
  return res.json();
}

// TClientID.h: `#define ID_FRAME_LOGIN (0x00000001)` -- frames are ID_FRAME_*, controls ID_CTRLINST_*
export async function loadIdNames(): Promise<IdNames> {
  const text = await (await fetch('/api/ids')).text();
  const names: IdNames = { frame: new Map(), ctrl: new Map() };
  for (const m of text.matchAll(/^\s*#define\s+(ID_FRAME_\w+|ID_CTRLINST_\w+)\s+\(?\s*(0x[0-9a-fA-F]+|\d+)\s*\)?/gm)) {
    const map = m[1].startsWith('ID_FRAME_') ? names.frame : names.ctrl;
    const id = Number(m[2]);
    if (!map.has(id)) map.set(id, m[1]);
  }
  return names;
}

export const hex = (v: number, digits = 8) => '0x' + (v >>> 0).toString(16).toUpperCase().padStart(digits, '0');

// TCML colors are D3DCOLOR (0xAARRGGBB); an alpha of 0 is treated as opaque like most UI text
export function cssColor(argb: number) {
  const a = (argb >>> 24) & 255, r = (argb >>> 16) & 255, g = (argb >>> 8) & 255, b = argb & 255;
  return `rgba(${r},${g},${b},${a === 0 ? 1 : a / 255})`;
}
