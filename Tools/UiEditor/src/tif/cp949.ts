// CP949 (EUC-KR superset) text <-> bytes. Browsers and Node only decode it, so the encoder is the
// decoder's inverse, built once over every double-byte code.
const decoder = new TextDecoder('euc-kr');

export const decodeCP949 = (bytes: Uint8Array) => decoder.decode(bytes);

let table: Map<string, number> | null = null;

function encodeTable() {
  if (table) return table;
  table = new Map();
  const pair = new Uint8Array(2);
  for (let lead = 0x81; lead <= 0xFE; lead++)
    for (let trail = 0x41; trail <= 0xFE; trail++) {
      pair[0] = lead; pair[1] = trail;
      const ch = decoder.decode(pair);
      if (ch.length === 1 && ch !== '�' && !table.has(ch)) table.set(ch, (lead << 8) | trail);
    }
  return table;
}

export function encodeCP949(text: string): Uint8Array {
  const out: number[] = [];
  for (const ch of text) {
    const c = ch.codePointAt(0)!;
    if (c < 0x80) { out.push(c); continue; }
    const code = encodeTable().get(ch);
    if (code === undefined) out.push(0x3F);       // '?' for characters CP949 cannot hold
    else out.push(code >> 8, code & 0xFF);
  }
  return Uint8Array.from(out);
}
