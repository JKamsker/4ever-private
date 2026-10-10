// Cuts item icons out of a client's atlas as RGBA images / PNGs:
//   icon index -> TClientCmd.tif item-icon IMAGELIST child -> sprite id -> Index\TClientI.IDX -> *Img.TIM IMAGESET
//   -> CD3DImage parts (page id + UV quad) + 1-bit mask -> *Src.TIS page (DDS: DXT1/3/5 or uncompressed).
// Reads TIF v2 (ours: shared lists, list #0 = item icons) and Genesis' variant of v1: header [999][frame count],
// frames with a 174-byte fixed node part (ours 170: one extra DWORD), children inline.
// Formats: TCMLParser::LoadFRAME, CTachyonRes::LoadIMGBUF / LoadIMG, CD3DImage::Load (see [[icon-atlas-decoding]]).
using System.IO.Compression;
using ClassicPort;

public class IconCut
{
    const int NodeFixed = 48 + 8 + 20 + 8 + 16 + 2 + 68;   // LoadFRAME fixed part after dwID + bType (ours)
    const int GenesisMagic = 999, GenesisNodeFixed = NodeFixed + 4;
    const byte TypeImageList = 0x0A;

    readonly string root;
    readonly List<uint> sprites;                               // item-icon list: index -> sprite id
    public IReadOnlyList<uint> Sprites => sprites;
    readonly Idx iidx;
    readonly Dictionary<uint, (string file, int pos)> pageAt = new();
    readonly Dictionary<string, byte[]> files = new();
    readonly Dictionary<uint, (int w, int h, byte[] rgba)> pages = new();

    public int Count => sprites.Count;

    public IconCut(string root)
    {
        this.root = root;
        sprites = ItemIconSprites(File.ReadAllBytes(Path.Combine(root, "TClientCmd.tif")));
        iidx = Idx.Load(Path.Combine(root, "Index", "TClientI.IDX"));
        byte[] lst = File.ReadAllBytes(Path.Combine(root, "Index", "TClientList.LST"));
        for (int i = 0, n = I32(lst, 0), p = 8; i < n; i++)
        {
            int l = I32(lst, p); string f = System.Text.Encoding.Latin1.GetString(lst, p + 4, l); p += 4 + l;
            byte[] b = Bytes(Path.Combine(root, "Data", f));
            for (int q = 0; q + 8 <= b.Length;)
            {
                int len = 8 + I32(b, q + 4);
                var raw = Client.Unchunk(b, q, out _);
                pageAt.TryAdd(BitConverter.ToUInt32(raw, 0), (f, q));
                q += len;
            }
        }
    }

    static int I32(byte[] b, int p) => BitConverter.ToInt32(b, p);
    byte[] Bytes(string path) => files.TryGetValue(path, out var b) ? b : files[path] = File.ReadAllBytes(path);

    // ------------------------------------------------------------------ TIF
    static int SkipNode(byte[] b, int p, int fixedLen, Action<int, int, int>? onList)
    {
        int start = p;
        byte type = b[p + 4];
        p += 5 + fixedLen;
        int tl = I32(b, p); p += 4 + Math.Max(tl, 0);
        int xl = I32(b, p); p += 4 + Math.Max(xl, 0);
        int n = I32(b, p); p += 4;
        if (n < 0) return p;                                   // v2 shared-list reference
        if (type == TypeImageList) onList?.Invoke(start, n, p);
        for (int i = 0; i < n; i++) p = SkipNode(b, p, fixedLen, onList);
        return p;
    }

    static uint Img0(byte[] b, int node) => BitConverter.ToUInt32(b, node + 5 + 48);

    /// <summary>Sprite id of every child of the item-icon list (v2: shared list #0; Genesis: the IMAGELIST with the most children).</summary>
    static List<uint> ItemIconSprites(byte[] b)
    {
        var o = new List<uint>();
        if (I32(b, 0) == -2)
        {
            int n = I32(b, 8), p = 12;
            for (int i = 0; i < n; i++) { o.Add(Img0(b, p)); p = SkipNode(b, p, NodeFixed, null); }
            return o;
        }
        if (I32(b, 0) != GenesisMagic) throw new InvalidDataException("unsupported TClientCmd.tif layout");
        int best = -1, bestStart = 0;
        int fc = I32(b, 4), q = 8;
        for (int f = 0; f < fc; f++) q = SkipNode(b, q, GenesisNodeFixed, (_, n, cs) => { if (n > best) { best = n; bestStart = cs; } });
        for (int i = 0, p = bestStart; i < best; i++) { o.Add(Img0(b, p)); p = SkipNode(b, p, GenesisNodeFixed, null); }
        return o;
    }

    // ------------------------------------------------------------------ pages
    (int w, int h, byte[] rgba) Page(uint id)
    {
        if (pages.TryGetValue(id, out var pg)) return pg;
        var (f, pos) = pageAt[id];
        var raw = Client.Unchunk(Bytes(Path.Combine(root, "Data", f)), pos, out _);
        using var z = new ZLibStream(new MemoryStream(raw, 13, I32(raw, 9)), CompressionMode.Decompress);
        var ms = new MemoryStream(); z.CopyTo(ms);
        return pages[id] = Dds.Decode(ms.ToArray());
    }

    // ------------------------------------------------------------------ cut
    /// <summary>The icon at an item-icon index, or null if the index has no sprite.</summary>
    public (int w, int h, byte[] rgba)? Cut(int index)
    {
        if (index < 0 || index >= sprites.Count) return null;
        uint sid = sprites[index];
        if (sid == 0xFFFFFFFF || !iidx.ById.TryGetValue(sid, out var e)) return null;
        var rec = Client.Unchunk(Bytes(Path.Combine(root, "Data", iidx.Files[e.File])), (int)e.Pos, out _);
        var r = new BinaryReader(new MemoryStream(rec));
        int frames = r.ReadInt32(); r.BaseStream.Seek(4 * frames, SeekOrigin.Current);
        int keys = r.ReadInt32(); r.BaseStream.Seek(8 * keys, SeekOrigin.Current);
        r.ReadUInt32(); r.ReadByte(); r.ReadInt32();          // total tick, format, blob size
        int parts = r.ReadInt32(), w = r.ReadInt32(), h = r.ReadInt32();
        var outp = new byte[4 * w * h];
        for (int pi = 0; pi < parts; pi++)
        {
            uint pid = r.ReadUInt32();
            var v = new (float x, float y, float u, float vv)[4];
            for (int k = 0; k < 4; k++) { float x = r.ReadSingle(), y = r.ReadSingle(); r.ReadSingle(); r.ReadSingle(); v[k] = (x, y, r.ReadSingle(), r.ReadSingle()); }
            var (pw, ph, px) = Page(pid);
            // quad 0=(left,top) 3=(right,bottom); positions carry the +0.5 TVERTEX offset
            int dx0 = (int)Math.Floor(v[0].x), dy0 = (int)Math.Floor(v[0].y), dx1 = (int)Math.Floor(v[3].x), dy1 = (int)Math.Floor(v[3].y);
            int sx0 = (int)Math.Round(v[0].u * pw), sy0 = (int)Math.Round(v[0].vv * ph);
            for (int y = dy0; y < dy1; y++)
                for (int x = dx0; x < dx1; x++)
                {
                    int sx = sx0 + x - dx0, sy = sy0 + y - dy0;
                    if (x < 0 || y < 0 || x >= w || y >= h || sx < 0 || sy < 0 || sx >= pw || sy >= ph) continue;
                    Buffer.BlockCopy(px, 4 * (sy * pw + sx), outp, 4 * (y * w + x), 4);
                }
        }
        int mlen = r.ReadInt32(); var mask = r.ReadBytes(mlen);
        int pitch = (w + 7) / 8;
        if (mlen >= pitch * h)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    if ((mask[pitch * y + x / 8] & (0x80 >> (x % 8))) == 0) outp[4 * (y * w + x) + 3] = 0;
        return (w, h, outp);
    }
}

/// <summary>DDS -> RGBA8: DXT1/DXT3/DXT5 and uncompressed RGB(A) via the header bit masks.</summary>
public static class Dds
{
    public static (int w, int h, byte[] rgba) Decode(byte[] d)
    {
        if (BitConverter.ToUInt32(d, 0) != 0x20534444) throw new InvalidDataException("not a DDS");
        int h = BitConverter.ToInt32(d, 12), w = BitConverter.ToInt32(d, 16);
        uint pfFlags = BitConverter.ToUInt32(d, 80), fourcc = BitConverter.ToUInt32(d, 84);
        var o = new byte[4 * w * h];
        if ((pfFlags & 4) != 0)
        {
            string cc = System.Text.Encoding.ASCII.GetString(d, 84, 4);
            int bs = cc == "DXT1" ? 8 : 16, p = 128;
            for (int by = 0; by < (h + 3) / 4; by++)
                for (int bx = 0; bx < (w + 3) / 4; bx++, p += bs)
                    Block(d, p, cc, o, w, h, bx * 4, by * 4);
            return (w, h, o);
        }
        int bits = BitConverter.ToInt32(d, 88);
        uint rm = BitConverter.ToUInt32(d, 92), gm = BitConverter.ToUInt32(d, 96), bm = BitConverter.ToUInt32(d, 100), am = BitConverter.ToUInt32(d, 104);
        bool hasA = (pfFlags & 1) != 0;
        int bpp = bits / 8, q = 128;
        for (int i = 0; i < w * h; i++, q += bpp)
        {
            uint px = bpp == 4 ? BitConverter.ToUInt32(d, q) : bpp == 2 ? BitConverter.ToUInt16(d, q) : (uint)(d[q] | d[q + 1] << 8 | d[q + 2] << 16);
            o[4 * i] = Chan(px, rm); o[4 * i + 1] = Chan(px, gm); o[4 * i + 2] = Chan(px, bm);
            o[4 * i + 3] = hasA ? Chan(px, am) : (byte)255;
        }
        return (w, h, o);
    }

    static byte Chan(uint px, uint mask)
    {
        if (mask == 0) return 0;
        int shift = System.Numerics.BitOperations.TrailingZeroCount(mask);
        uint max = mask >>> shift, v = (px & mask) >>> shift;
        return (byte)(v * 255 / max);
    }

    static void Rgb565(ushort c, out int r, out int g, out int b) { r = (c >> 11 & 31) * 255 / 31; g = (c >> 5 & 63) * 255 / 63; b = (c & 31) * 255 / 31; }

    static void Block(byte[] d, int p, string cc, byte[] o, int w, int h, int x0, int y0)
    {
        var alpha = new byte[16];
        int cp = p;
        if (cc == "DXT3")
        {
            for (int i = 0; i < 16; i++) alpha[i] = (byte)((d[p + i / 2] >> (4 * (i % 2)) & 15) * 17);
            cp += 8;
        }
        else if (cc == "DXT5")
        {
            int a0 = d[p], a1 = d[p + 1];
            var pal = new int[8]; pal[0] = a0; pal[1] = a1;
            if (a0 > a1) for (int i = 1; i < 7; i++) pal[i + 1] = ((7 - i) * a0 + i * a1) / 7;
            else { for (int i = 1; i < 5; i++) pal[i + 1] = ((5 - i) * a0 + i * a1) / 5; pal[6] = 0; pal[7] = 255; }
            ulong bitsA = 0; for (int i = 0; i < 6; i++) bitsA |= (ulong)d[p + 2 + i] << (8 * i);
            for (int i = 0; i < 16; i++) alpha[i] = (byte)pal[(int)(bitsA >> (3 * i) & 7)];
            cp += 8;
        }
        else Array.Fill(alpha, (byte)255);
        ushort c0 = BitConverter.ToUInt16(d, cp), c1 = BitConverter.ToUInt16(d, cp + 2);
        Rgb565(c0, out int r0, out int g0, out int b0); Rgb565(c1, out int r1, out int g1, out int b1);
        var col = new (int r, int g, int b, int a)[4];
        col[0] = (r0, g0, b0, 255); col[1] = (r1, g1, b1, 255);
        if (cc != "DXT1" || c0 > c1) { col[2] = ((2 * r0 + r1) / 3, (2 * g0 + g1) / 3, (2 * b0 + b1) / 3, 255); col[3] = ((r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3, 255); }
        else { col[2] = ((r0 + r1) / 2, (g0 + g1) / 2, (b0 + b1) / 2, 255); col[3] = (0, 0, 0, 0); }
        uint idx = BitConverter.ToUInt32(d, cp + 4);
        for (int i = 0; i < 16; i++)
        {
            int x = x0 + i % 4, y = y0 + i / 4;
            if (x >= w || y >= h) continue;
            var c = col[idx >> (2 * i) & 3];
            int t = 4 * (y * w + x);
            o[t] = (byte)c.r; o[t + 1] = (byte)c.g; o[t + 2] = (byte)c.b;
            o[t + 3] = cc == "DXT1" ? (byte)c.a : alpha[i];
        }
    }
}

/// <summary>Minimal PNG writer (RGBA8, filter 0).</summary>
public static class Png
{
    static readonly uint[] Crc = Enumerable.Range(0, 256).Select(n => { uint c = (uint)n; for (int k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320 ^ (c >> 1) : c >> 1; return c; }).ToArray();

    public static byte[] Encode(int w, int h, byte[] rgba)
    {
        var raw = new MemoryStream();
        for (int y = 0; y < h; y++) { raw.WriteByte(0); raw.Write(rgba, 4 * w * y, 4 * w); }
        var z = new MemoryStream();
        raw.Position = 0;
        using (var zs = new ZLibStream(z, CompressionLevel.Optimal, true)) raw.CopyTo(zs);
        var o = new MemoryStream();
        o.Write([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]);
        Chunk(o, "IHDR", [.. BE(w), .. BE(h), 8, 6, 0, 0, 0]);
        Chunk(o, "IDAT", z.ToArray());
        Chunk(o, "IEND", []);
        return o.ToArray();
    }

    static byte[] BE(int v) => [(byte)(v >> 24), (byte)(v >> 16), (byte)(v >> 8), (byte)v];

    static void Chunk(Stream o, string type, byte[] data)
    {
        o.Write(BE(data.Length));
        byte[] td = [.. System.Text.Encoding.ASCII.GetBytes(type), .. data];
        o.Write(td);
        uint c = 0xFFFFFFFF; foreach (var b in td) c = Crc[(c ^ b) & 0xFF] ^ (c >> 8);
        o.Write(BE((int)~c));
    }
}
