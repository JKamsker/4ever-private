// Moves the ported mount icons (Data\Img\Custom\<id>.png, "own images" with icon index >= 60000) into the
// real icon atlas, so they get normal icon indices after ours (2141+):
//   PNGs -> one new A8R8G8B8 DDS page appended to Img\IconSrc.TIS   (TClientList.LST page total +1)
//        -> one IMAGESET sprite per icon appended to Img\IconImg.TIM (+ Index\TClientI.IDX entries)
//        -> one child per sprite appended to the shared item-icon list of TClientCmd.tif (v2)
//   TItemVisual.m_wIcon / TMount.m_wIcon rewritten to the new indices; the PNGs are deleted.
// Formats: CTachyonRes::LoadIMGBUF / LoadIMG, CD3DImage::Load / GetMask, TCMLParser v2 (Tools/TifDedupe).
using System.IO.Compression;

namespace ClassicPort;

public class IconMerge
{
    const int Slot = 32, PageSize = 512;                 // 16x16 slots of 32 px, like our icon pages
    const byte FmtNonComp = 5;                            // NON_COMP -> D3DFMT_A8R8G8B8
    const int NodeFixed = 48 + 8 + 20 + 8 + 16 + 2 + 68;  // TCMLParser::LoadFRAME fixed part after dwID+bType
    const int ItemList = 0;                               // shared list #0 = item icons (2141 children)
    const int ItemListPrefix = 21;                        // shared list #21 = first 2099 item icons (5 users)
    const ushort FirstMergedIcon = 2141;                  // list #0 size before the first merge

    readonly string game; readonly TextWriter log;
    public IconMerge(string game, TextWriter log) { this.game = game; this.log = log; }

    string G(params string[] p) => Path.Combine([game, .. p]);

    // ------------------------------------------------------------------ PNG (8-bit RGB / RGBA, no interlace)
    public static (int w, int h, byte[] rgba) DecodePng(byte[] b)
    {
        int w = BE(b, 16), h = BE(b, 20); byte depth = b[24], ct = b[25], il = b[28];
        if (depth != 8 || (ct != 2 && ct != 6) || il != 0) throw new NotSupportedException($"png depth {depth} type {ct} interlace {il}");
        int bpp = ct == 6 ? 4 : 3;
        var idat = new MemoryStream();
        for (int p = 8; p < b.Length;)
        {
            int len = BE(b, p); string type = System.Text.Encoding.ASCII.GetString(b, p + 4, 4);
            if (type == "IDAT") idat.Write(b, p + 8, len);
            p += 12 + len;
        }
        idat.Position = 0;
        var raw = new MemoryStream();
        using (var z = new ZLibStream(idat, CompressionMode.Decompress)) z.CopyTo(raw);
        byte[] d = raw.ToArray();
        int stride = w * bpp; var px = new byte[h * stride]; var prev = new byte[stride];
        for (int y = 0; y < h; y++)
        {
            byte f = d[y * (stride + 1)]; var cur = new byte[stride];
            Buffer.BlockCopy(d, y * (stride + 1) + 1, cur, 0, stride);
            for (int x = 0; x < stride; x++)
            {
                int a = x >= bpp ? cur[x - bpp] : 0, up = prev[x], c = x >= bpp ? prev[x - bpp] : 0;
                cur[x] += f switch { 0 => 0, 1 => (byte)a, 2 => (byte)up, 3 => (byte)((a + up) / 2), 4 => Paeth(a, up, c), _ => throw new InvalidDataException("png filter") };
            }
            Buffer.BlockCopy(cur, 0, px, y * stride, stride); prev = cur;
        }
        var rgba = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            rgba[4 * i] = px[bpp * i]; rgba[4 * i + 1] = px[bpp * i + 1]; rgba[4 * i + 2] = px[bpp * i + 2];
            rgba[4 * i + 3] = bpp == 4 ? px[bpp * i + 3] : (byte)255;
        }
        return (w, h, rgba);
    }
    static int BE(byte[] b, int o) => (b[o] << 24) | (b[o + 1] << 16) | (b[o + 2] << 8) | b[o + 3];
    static byte Paeth(int a, int b, int c) { int p = a + b - c, pa = Math.Abs(p - a), pb = Math.Abs(p - b), pc = Math.Abs(p - c); return (byte)(pa <= pb && pa <= pc ? a : pb <= pc ? b : c); }

    // ------------------------------------------------------------------ TIF v2
    class Tif
    {
        public byte[] B = [];
        public List<(int count, int start, int end)> Lists = new();   // children byte span per shared list
        public int FramesStart, TailStart;
        public List<int> RefPositions = new();                         // positions of negative child counts in frames
        public List<int> ListChildStarts(int list)
        {
            var (count, start, _) = Lists[list]; var o = new List<int>(); int p = start;
            for (int i = 0; i < count; i++) { o.Add(p); p = SkipNode(B, p, null); }
            return o;
        }
    }

    static int I32(byte[] b, int p) => BitConverter.ToInt32(b, p);

    /// <summary>Skips one node at p (v1 children inline, or v2 shared ref); records ref positions.</summary>
    static int SkipNode(byte[] b, int p, List<int>? refs)
    {
        p += 5 + NodeFixed;
        int tl = I32(b, p); p += 4 + Math.Max(tl, 0);
        int xl = I32(b, p); p += 4 + Math.Max(xl, 0);
        int n = I32(b, p);
        if (n < 0) { refs?.Add(p); return p + 4; }
        p += 4;
        for (int i = 0; i < n; i++) p = SkipNode(b, p, refs);
        return p;
    }

    static Tif ParseTif(byte[] b)
    {
        var t = new Tif { B = b };
        if (I32(b, 0) != -2) throw new InvalidDataException("TClientCmd.tif is not v2");
        int nsh = I32(b, 4), p = 8;
        for (int s = 0; s < nsh; s++)
        {
            int n = I32(b, p); p += 4; int st = p;
            for (int i = 0; i < n; i++) p = SkipNode(b, p, null);
            t.Lists.Add((n, st, p));
        }
        t.FramesStart = p;
        int fc = I32(b, p); p += 4;
        for (int f = 0; f < fc; f++) p = SkipNode(b, p, t.RefPositions);
        t.TailStart = p;
        return t;
    }

    static uint NodeImg0(byte[] b, int nodeStart) => BitConverter.ToUInt32(b, nodeStart + 5 + 48);

    // ------------------------------------------------------------------ run
    /// <param name="all">merge every Custom PNG that TItemVisual / TMount use as an icon (any port), not only the
    /// Classic port's bookkeeping. Own images the client code uses directly (60000-60016, 61001, ...) are never
    /// item/mount icons in the tables, so they stay.</param>
    public void Run(bool apply, bool all = false)
    {
        string tcd = G("Tcd"), custom = G("Data", "Img", "Custom");
        string statePath = StateFiles.Get("ClassicPort.items.state");
        List<ushort> pngIds;
        if (all)
        {
            var used = new HashSet<ushort>();
            var v = File.ReadAllBytes(Path.Combine(tcd, "TItemVisual.tcd"));
            for (int i = 0, n = BitConverter.ToUInt16(v, 0); i < n; i++) used.Add(BitConverter.ToUInt16(v, 2 + 63 * i + 34));
            foreach (var m in TMount.Load(Path.Combine(tcd, "TMount.tcd"), false)) used.Add(m.Icon);
            pngIds = used.Where(id => id >= 60000 && File.Exists(Path.Combine(custom, $"{id}.png"))).OrderBy(x => x).ToList();
            var missing = used.Where(id => id >= 60000 && !pngIds.Contains(id)).ToList();
            if (missing.Count > 0) log.WriteLine($"warning: icons with no Custom PNG: {string.Join(",", missing)}");
        }
        else
        {
            if (!File.Exists(statePath)) { log.WriteLine("nothing to do: no port bookkeeping (" + statePath + ")"); return; }
            pngIds = File.ReadAllLines(statePath).Where(l => l.StartsWith("png=")).Select(l => ushort.Parse(Path.GetFileNameWithoutExtension(l[4..]))).OrderBy(x => x).ToList();
        }
        if (pngIds.Count == 0) { log.WriteLine("no ported icons left to merge"); return; }
        if (pngIds.Count > (PageSize / Slot) * (PageSize / Slot)) throw new InvalidDataException($"{pngIds.Count} icons do not fit one page");

        var tif = ParseTif(File.ReadAllBytes(G("TClientCmd.tif")));
        var listChildren = tif.ListChildStarts(ItemList);
        int firstIndex = tif.Lists[ItemList].count;

        // the first merge folded list #21 (a prefix of list #0) into #0; later merges just append to #0
        var prefix = tif.Lists.Count > ItemListPrefix ? tif.ListChildStarts(ItemListPrefix) : [];
        bool dropPrefix = prefix.Count > 0 && prefix.Count <= listChildren.Count
            && !prefix.Where((p, i) => NodeImg0(tif.B, p) != NodeImg0(tif.B, listChildren[i])).Any();
        log.WriteLine($"item icon list: {firstIndex} icons; list #{ItemListPrefix} " + (dropPrefix ? $"is a prefix of it ({prefix.Count} icons) and gets folded in" : "is unrelated, kept"));

        // sprites / pages
        var iidx = ClassicPort.Idx.Load(G("Index", "TClientI.IDX"));
        int timFile = iidx.Files.FindIndex(f => f.EndsWith(@"\IconImg.TIM", StringComparison.OrdinalIgnoreCase));
        uint nextSprite = iidx.Entries.Max(e => e.Id) + 1;
        byte[] lst = File.ReadAllBytes(G("Index", "TClientList.LST"));
        var tisFiles = new List<string>(); { int n = I32(lst, 0), p = 8; for (int i = 0; i < n; i++) { int l = I32(lst, p); tisFiles.Add(System.Text.Encoding.Latin1.GetString(lst, p + 4, l)); p += 4 + l; } }
        uint maxPage = 0; byte[]? fmt5Header = null;
        foreach (var f in tisFiles)
        {
            var b = File.ReadAllBytes(G("Data", f)); int p = 0;
            while (p + 8 <= b.Length)
            {
                var raw = Client.Unchunk(b, p, out int len);
                maxPage = Math.Max(maxPage, BitConverter.ToUInt32(raw, 0));
                if (fmt5Header == null && raw[4] == FmtNonComp)
                {
                    using var z = new ZLibStream(new MemoryStream(raw, 13, BitConverter.ToInt32(raw, 9)), CompressionMode.Decompress);
                    fmt5Header = new byte[128]; z.ReadExactly(fmt5Header);
                }
                p += len;
            }
        }
        uint pageId = maxPage + 1;
        log.WriteLine($"new page {pageId}, sprites {nextSprite}..{nextSprite + pngIds.Count - 1}, icon indices {firstIndex}..{firstIndex + pngIds.Count - 1}");

        // page pixels (BGRA) + one sprite per icon
        var page = new byte[PageSize * PageSize * 4];
        var spriteChunks = new List<(uint id, byte[] chunk)>();
        var newIndex = new Dictionary<ushort, ushort>();
        var pngs = new Dictionary<ushort, (int w, int h, byte[] rgba)>();
        for (int k = 0; k < pngIds.Count; k++)
        {
            var (w, h, rgba) = DecodePng(File.ReadAllBytes(Path.Combine(custom, $"{pngIds[k]}.png")));
            if (w > Slot || h > Slot) throw new InvalidDataException($"{pngIds[k]}.png is {w}x{h}");
            pngs[pngIds[k]] = (w, h, rgba);
            int sx = k % (PageSize / Slot) * Slot, sy = k / (PageSize / Slot) * Slot;
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    int s = 4 * (y * w + x), d = 4 * ((sy + y) * PageSize + sx + x);
                    page[d] = rgba[s + 2]; page[d + 1] = rgba[s + 1]; page[d + 2] = rgba[s]; page[d + 3] = rgba[s + 3];
                }
            uint sid = nextSprite + (uint)k;
            spriteChunks.Add((sid, Client.Chunk(SpriteRecord(sid, pageId, sx, sy, w, h, rgba))));
            newIndex[pngIds[k]] = (ushort)(firstIndex + k);
        }

        // DDS page: header copied from an existing A8R8G8B8 page, dimensions/pitch patched
        if (fmt5Header == null) throw new InvalidDataException("no A8R8G8B8 page to copy the DDS header from");
        var dds = (byte[])fmt5Header.Clone();
        BitConverter.GetBytes(PageSize).CopyTo(dds, 12); BitConverter.GetBytes(PageSize).CopyTo(dds, 16);
        BitConverter.GetBytes(PageSize * 4).CopyTo(dds, 20);
        dds = [.. dds, .. page];
        var zms = new MemoryStream(); using (var z = new ZLibStream(zms, CompressionLevel.Optimal, true)) z.Write(dds);
        byte[] zdds = zms.ToArray();
        byte[] pageRaw = [.. BitConverter.GetBytes(pageId), FmtNonComp, .. BitConverter.GetBytes(dds.Length), .. BitConverter.GetBytes(zdds.Length), .. zdds];

        // TIF: append children to list #0 (clone of its last child), redirect list #21 users, drop list #21
        byte[] lastChild = tif.B[listChildren[^1]..tif.Lists[ItemList].end];
        uint lastId = BitConverter.ToUInt32(lastChild, 0);
        var added = new MemoryStream();
        for (int k = 0; k < pngIds.Count; k++)
        {
            var c = (byte[])lastChild.Clone();
            BitConverter.GetBytes(lastId + 1 + (uint)k).CopyTo(c, 0);
            BitConverter.GetBytes(nextSprite + (uint)k).CopyTo(c, 5 + 48);
            added.Write(c);
        }
        byte[] newTif = RewriteTif(tif, added.ToArray(), pngIds.Count, dropPrefix);

        // icon fields
        var vis = File.ReadAllBytes(Path.Combine(tcd, "TItemVisual.tcd"));
        int visChanged = 0;
        for (int i = 0, n = BitConverter.ToUInt16(vis, 0); i < n; i++)
        {
            int o = 2 + 63 * i + 34;
            if (newIndex.TryGetValue(BitConverter.ToUInt16(vis, o), out var ni)) { BitConverter.GetBytes(ni).CopyTo(vis, o); visChanged++; }
        }
        var mounts = TMount.Load(Path.Combine(tcd, "TMount.tcd"), false);
        int mountChanged = mounts.Count(m => newIndex.ContainsKey(m.Icon));
        mounts = mounts.Select(m => newIndex.TryGetValue(m.Icon, out var ni) ? m with { Icon = ni } : m).ToList();
        log.WriteLine($"icon fields rewritten: {visChanged} visuals, {mountChanged} mounts");

        if (!apply) { log.WriteLine("dry run — nothing written (use: icons --apply)"); return; }

        using (var fs = new FileStream(G("Data", "Img", "IconSrc.TIS"), FileMode.Append)) fs.Write(Client.Chunk(pageRaw));
        BitConverter.GetBytes(I32(lst, 4) + 1).CopyTo(lst, 4);                      // total page count (progress)
        File.WriteAllBytes(G("Index", "TClientList.LST"), lst);
        using (var fs = new FileStream(G("Data", iidx.Files[timFile]), FileMode.Append))   // Position starts at the old length
            foreach (var (id, chunk) in spriteChunks) { iidx.Entries.Add(new IdxEntry(id, timFile, (uint)fs.Position)); fs.Write(chunk); }
        iidx.Save(G("Index", "TClientI.IDX"));
        File.WriteAllBytes(G("TClientCmd.tif"), newTif);
        File.WriteAllBytes(Path.Combine(tcd, "TItemVisual.tcd"), vis);
        TMount.Save(Path.Combine(tcd, "TMount.tcd"), mounts, withScale: false);

        // read every new icon back through TIF -> IDX -> TIM -> TIS and compare with the PNG
        int ok = VerifyIcons(newIndex, pngs);
        log.WriteLine($"icons read back through the atlas: {ok}/{pngIds.Count} pixel-identical");
        if (ok != pngIds.Count) throw new InvalidDataException("atlas read-back mismatch — restore from git");

        foreach (var id in pngIds) File.Delete(Path.Combine(custom, $"{id}.png"));
        if (File.Exists(statePath)) File.WriteAllLines(statePath, File.ReadAllLines(statePath).Where(l => !l.StartsWith("png=")));
        log.WriteLine($"written: IconSrc.TIS (+1 page), IconImg.TIM (+{pngIds.Count}), TClientI.IDX, TClientList.LST, TClientCmd.tif, TItemVisual.tcd, TMount.tcd; {pngIds.Count} PNGs deleted");
    }

    // ------------------------------------------------------------------ check
    /// <summary>Resolves every TItemVisual / TMount icon through TIF list #0 -> TClientI.IDX -> IconImg.TIM ->
    /// TIS page, compares icons that used to be Custom PNGs against those PNGs (taken from <paramref name="before"/>:
    /// the old TItemVisual.tcd, TMount.tcd and *.png), and writes an HTML contact sheet of every icon on an
    /// A8R8G8B8 page (= every merged icon) with the items / mounts using it.</summary>
    public int Check(string? before, string? html)
    {
        string tcd = G("Tcd");
        var tif = ParseTif(File.ReadAllBytes(G("TClientCmd.tif")));
        var kids = tif.ListChildStarts(ItemList);
        var iidx = ClassicPort.Idx.Load(G("Index", "TClientI.IDX"));
        byte[] lst = File.ReadAllBytes(G("Index", "TClientList.LST"));
        var pageFmt = new Dictionary<uint, byte>(); var pages = new Dictionary<uint, byte[]>();
        { int n = I32(lst, 0), p = 8; for (int i = 0; i < n; i++) { int l = I32(lst, p); var f = System.Text.Encoding.Latin1.GetString(lst, p + 4, l); p += 4 + l;
            var b = File.ReadAllBytes(G("Data", f));
            for (int q = 0; q + 8 <= b.Length;)
            {
                var raw = Client.Unchunk(b, q, out int len); q += len;
                uint id = BitConverter.ToUInt32(raw, 0); pageFmt[id] = raw[4];
                if (raw[4] != FmtNonComp) continue;
                using var z = new ZLibStream(new MemoryStream(raw, 13, BitConverter.ToInt32(raw, 9)), CompressionMode.Decompress);
                var ms = new MemoryStream(); z.CopyTo(ms); pages[id] = ms.ToArray();
            } } }

        var files = new Dictionary<int, byte[]>();
        int errors = 0;
        void Err(string s) { errors++; log.WriteLine("  ERROR " + s); }

        // icon index -> (w, h, page, sx, sy) or null when broken
        var sprite = new Dictionary<ushort, (int w, int h, uint page, int sx, int sy)?>();
        (int w, int h, uint page, int sx, int sy)? Resolve(ushort icon, string who)
        {
            if (sprite.TryGetValue(icon, out var s)) return s;
            sprite[icon] = null;
            if (icon >= 60000) { if (!File.Exists(G("Data", "Img", "Custom", $"{icon}.png"))) Err($"{who}: own image {icon} has no Custom PNG"); return null; }
            if (icon >= kids.Count) { Err($"{who}: icon {icon} >= list size {kids.Count}"); return null; }
            uint sid = NodeImg0(tif.B, kids[icon]);
            if (!iidx.ById.TryGetValue(sid, out var e)) { Err($"{who}: icon {icon} sprite {sid} not in TClientI.IDX"); return null; }
            if (!files.TryGetValue(e.File, out var fb)) files[e.File] = fb = File.ReadAllBytes(G("Data", iidx.Files[e.File]));
            var r = new BinaryReader(new MemoryStream(Client.Unchunk(fb, (int)e.Pos, out _)));
            int nf = r.ReadInt32(); r.ReadBytes(4 * nf); int nk = r.ReadInt32(); r.ReadBytes(8 * nk); r.ReadUInt32(); r.ReadByte(); r.ReadInt32();
            r.ReadInt32(); int w = r.ReadInt32(), h = r.ReadInt32(); uint pid = r.ReadUInt32();
            r.ReadBytes(16); float u0 = r.ReadSingle(), v0 = r.ReadSingle();
            if (!pageFmt.ContainsKey(pid)) { Err($"{who}: icon {icon} sprite {sid} on missing page {pid}"); return null; }
            return sprite[icon] = (w, h, pid, (int)Math.Round(u0 * PageSize), (int)Math.Round(v0 * PageSize));
        }
        byte[]? Pixels(ushort icon)
        {
            if (sprite.GetValueOrDefault(icon) is not { } s || !pages.TryGetValue(s.page, out var dds)) return null;
            var rgba = new byte[s.w * s.h * 4];
            for (int y = 0; y < s.h; y++) for (int x = 0; x < s.w; x++)
            {
                int d = 128 + 4 * ((s.sy + y) * PageSize + s.sx + x), o = 4 * (y * s.w + x);
                rgba[o] = dds[d + 2]; rgba[o + 1] = dds[d + 1]; rgba[o + 2] = dds[d]; rgba[o + 3] = dds[d + 3];
            }
            return rgba;
        }

        static Dictionary<ushort, ushort> VisualIcons(byte[] v)
        {
            var d = new Dictionary<ushort, ushort>();
            for (int i = 0, n = BitConverter.ToUInt16(v, 0); i < n; i++) d[BitConverter.ToUInt16(v, 2 + 63 * i)] = BitConverter.ToUInt16(v, 2 + 63 * i + 34);
            return d;
        }
        var vis = VisualIcons(File.ReadAllBytes(Path.Combine(tcd, "TItemVisual.tcd")));
        var mounts = TMount.Load(Path.Combine(tcd, "TMount.tcd"), false);
        var users = new Dictionary<ushort, List<string>>();
        void Use(ushort icon, string s) { if (!users.TryGetValue(icon, out var l)) users[icon] = l = new(); l.Add(s); }

        var items = TItem.Load(Path.Combine(tcd, "TItem.tcd"));
        int itemsChecked = 0;
        foreach (var it in items)
        {
            ushort v0 = BitConverter.ToUInt16(it.Raw, it.TailOfs + 63);
            if (v0 == 0) continue;
            if (!vis.TryGetValue(v0, out var icon)) { if (v0 < 60000) Err($"item {it.Id} {it.Name}: visual {v0} not in TItemVisual"); continue; }
            itemsChecked++;
            Resolve(icon, $"item {it.Id} {it.Name}");
            Use(icon, $"{it.Id} {it.Name}");
        }
        foreach (var m in mounts) { Resolve(m.Icon, $"mount {m.Id}"); Use(m.Icon, $"mount {m.Id}"); }
        foreach (var (vid, icon) in vis) Resolve(icon, $"visual {vid}");
        log.WriteLine($"resolved: {itemsChecked} items, {mounts.Count} mounts, {vis.Count} visuals -> {sprite.Count} distinct icons, {sprite.Count(s => s.Value == null)} broken");

        // merged icons nobody uses
        var merged = sprite.Where(s => s.Key >= FirstMergedIcon && s.Key < 60000 && s.Value is { } v && pages.ContainsKey(v.page)).Select(s => s.Key).ToHashSet();
        for (ushort i = FirstMergedIcon; i < kids.Count; i++)
            if (!sprite.ContainsKey(i) && iidx.ById.TryGetValue(NodeImg0(tif.B, kids[i]), out var e) && Resolve(i, $"list #0 child {i}") is { } s && pages.ContainsKey(s.page))
                log.WriteLine($"  note: merged icon {i} is used by no item / mount");

        // old Custom PNG -> new icon, pixel compare
        if (before != null)
        {
            var oldVis = VisualIcons(File.ReadAllBytes(Path.Combine(before, "TItemVisual.tcd")));
            var oldMounts = TMount.Load(Path.Combine(before, "TMount.tcd"), false).ToDictionary(m => m.Id);
            int same = 0, diff = 0;
            void Cmp(string who, ushort oldIcon, ushort newIcon)
            {
                string png = Path.Combine(before, $"{oldIcon}.png");
                if (oldIcon < 60000 || !File.Exists(png)) return;
                var (w, h, rgba) = DecodePng(File.ReadAllBytes(png));
                var px = Pixels(newIcon);
                if (px != null && sprite[newIcon] is { } s && s.w == w && s.h == h && px.AsSpan().SequenceEqual(rgba)) same++;
                else { diff++; Err($"{who}: was {oldIcon}.png, now icon {newIcon} shows something else"); }
            }
            foreach (var (vid, oi) in oldVis) if (vis.TryGetValue(vid, out var ni)) Cmp($"visual {vid}", oi, ni);
            foreach (var m in mounts) if (oldMounts.TryGetValue(m.Id, out var om)) Cmp($"mount {m.Id}", om.Icon, m.Icon);
            log.WriteLine($"old Custom PNG icons: {same} identical in the atlas, {diff} different");
        }

        if (html != null)
        {
            var sb = new System.Text.StringBuilder();
            sb.Append("<!doctype html><meta charset=utf-8><title>Merged icons</title><style>body{font:12px system-ui;background:#222;color:#ddd;margin:16px}"
                + ".g{display:grid;grid-template-columns:repeat(auto-fill,minmax(220px,1fr));gap:6px}.c{display:flex;gap:8px;background:#333;padding:6px;border-radius:4px}"
                + "img{width:64px;height:64px;image-rendering:pixelated;background:#555}b{color:#fff}</style><div class=g>");
            foreach (var icon in merged.OrderBy(x => x))
            {
                var s = sprite[icon]!.Value; var px = Pixels(icon)!;
                var who = users.GetValueOrDefault(icon) ?? ["(unused)"];
                sb.Append($"<div class=c><img src=\"data:image/png;base64,{Convert.ToBase64String(EncodePng(s.w, s.h, px))}\"><div><b>#{icon}</b><br>"
                    + string.Join("<br>", who.Take(4).Select(System.Net.WebUtility.HtmlEncode)) + (who.Count > 4 ? $"<br>+{who.Count - 4} more" : "") + "</div></div>");
            }
            sb.Append("</div>");
            File.WriteAllText(html, sb.ToString());
            log.WriteLine($"contact sheet: {merged.Count} merged icons -> {html}");
        }
        log.WriteLine(errors == 0 ? "OK: every icon resolves" : $"{errors} errors");
        return errors;
    }

    static byte[] EncodePng(int w, int h, byte[] rgba)
    {
        var raw = new MemoryStream();
        for (int y = 0; y < h; y++) { raw.WriteByte(0); raw.Write(rgba, y * w * 4, w * 4); }
        var z = new MemoryStream(); using (var zs = new ZLibStream(z, CompressionLevel.Optimal, true)) { raw.Position = 0; raw.CopyTo(zs); }
        var o = new MemoryStream();
        o.Write([137, 80, 78, 71, 13, 10, 26, 10]);
        void ChunkW(string t, byte[] d)
        {
            var tb = System.Text.Encoding.ASCII.GetBytes(t);
            o.Write([(byte)(d.Length >> 24), (byte)(d.Length >> 16), (byte)(d.Length >> 8), (byte)d.Length]); o.Write(tb); o.Write(d);
            uint c = Crc([.. tb, .. d]); o.Write([(byte)(c >> 24), (byte)(c >> 16), (byte)(c >> 8), (byte)c]);
        }
        ChunkW("IHDR", [(byte)(w >> 24), (byte)(w >> 16), (byte)(w >> 8), (byte)w, (byte)(h >> 24), (byte)(h >> 16), (byte)(h >> 8), (byte)h, 8, 6, 0, 0, 0]);
        ChunkW("IDAT", z.ToArray()); ChunkW("IEND", []);
        return o.ToArray();
    }
    static uint Crc(byte[] b) { uint c = 0xFFFFFFFF; foreach (var x in b) { c ^= x; for (int k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320 ^ (c >> 1) : c >> 1; } return ~c; }

    /// <summary>IMAGESET record (LoadIMG) holding one CD3DImage blob (CD3DImage::Load) with one part.</summary>
    static byte[] SpriteRecord(uint sid, uint pageId, int sx, int sy, int w, int h, byte[] rgba)
    {
        var blob = new MemoryStream(); var bw = new BinaryWriter(blob);
        bw.Write(1); bw.Write(w); bw.Write(h);
        bw.Write(pageId);
        foreach (var (vx, vy) in new[] { (0, 0), (w, 0), (0, h), (w, h) })
        {
            bw.Write(vx + 0.5f); bw.Write(vy + 0.5f); bw.Write(0.5f); bw.Write(1f);
            bw.Write((sx + vx) / (float)PageSize); bw.Write((sy + vy) / (float)PageSize);
        }
        int pitch = (w + 7) / 8; var mask = new byte[pitch * h];
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if (rgba[4 * (y * w + x) + 3] != 0) mask[pitch * y + x / 8] |= (byte)(0x80 >> (x % 8));
        bw.Write(mask.Length); bw.Write(mask);
        byte[] b = blob.ToArray();

        var rec = new MemoryStream(); var rw = new BinaryWriter(rec);
        rw.Write(1); rw.Write(sid);            // frames: the sprite's own image (ComplateIMG maps it by id)
        rw.Write(0);                           // no colour keys
        rw.Write(1000u);                       // total tick, as our icons
        rw.Write((byte)0);                     // format byte, as our icons
        rw.Write(b.Length); rw.Write(b);
        return rec.ToArray();
    }

    static byte[] RewriteTif(Tif t, byte[] addedChildren, int addedCount, bool dropPrefix)
    {
        var o = new MemoryStream(); var w = new BinaryWriter(o);
        w.Write(-2);
        w.Write(dropPrefix ? t.Lists.Count - 1 : t.Lists.Count);
        for (int s = 0; s < t.Lists.Count; s++)
        {
            if (dropPrefix && s == ItemListPrefix) continue;
            var (n, st, en) = t.Lists[s];
            w.Write(s == ItemList ? n + addedCount : n);
            w.Write(t.B, st, en - st);
            if (s == ItemList) w.Write(addedChildren);
        }
        // frames + fonts, with shared-list references remapped (#21 -> #0, above #21 shifted down)
        var fr = t.B[t.FramesStart..];
        if (dropPrefix) foreach (int rp in t.RefPositions)
        {
            int list = -I32(t.B, rp) - 1;
            int nl = list == ItemListPrefix ? ItemList : list > ItemListPrefix ? list - 1 : list;
            BitConverter.GetBytes(-(nl + 1)).CopyTo(fr, rp - t.FramesStart);
        }
        w.Write(fr);
        return o.ToArray();
    }

    int VerifyIcons(Dictionary<ushort, ushort> newIndex, Dictionary<ushort, (int w, int h, byte[] rgba)> pngs)
    {
        var tif = ParseTif(File.ReadAllBytes(G("TClientCmd.tif")));
        var kids = tif.ListChildStarts(ItemList);
        if (tif.RefPositions.Any(r => -I32(tif.B, r) - 1 >= tif.Lists.Count)) throw new InvalidDataException("dangling shared list ref");
        var iidx = ClassicPort.Idx.Load(G("Index", "TClientI.IDX"));
        var tis = File.ReadAllBytes(G("Data", "Img", "IconSrc.TIS"));
        var pages = new Dictionary<uint, byte[]>();
        for (int p = 0; p < tis.Length;)
        {
            var raw = Client.Unchunk(tis, p, out int len); p += len;
            if (raw[4] != FmtNonComp) continue;
            using var z = new ZLibStream(new MemoryStream(raw, 13, BitConverter.ToInt32(raw, 9)), CompressionMode.Decompress);
            var ms = new MemoryStream(); z.CopyTo(ms); pages[BitConverter.ToUInt32(raw, 0)] = ms.ToArray();
        }
        int ok = 0;
        foreach (var (pngId, idx) in newIndex)
        {
            uint sid = NodeImg0(tif.B, kids[idx]);
            var e = iidx.ById[sid];
            var rec = Client.Unchunk(File.ReadAllBytes(G("Data", iidx.Files[e.File])), (int)e.Pos, out _);
            var r = new BinaryReader(new MemoryStream(rec));
            r.ReadInt32(); r.ReadUInt32(); r.ReadInt32(); r.ReadUInt32(); r.ReadByte(); r.ReadInt32();
            r.ReadInt32(); int w = r.ReadInt32(), h = r.ReadInt32(); uint pid = r.ReadUInt32();
            r.ReadBytes(16); float u0 = r.ReadSingle(), v0 = r.ReadSingle();
            var dds = pages[pid]; int sx = (int)Math.Round(u0 * PageSize), sy = (int)Math.Round(v0 * PageSize);
            var (pw, ph, rgba) = pngs[pngId];
            bool same = w == pw && h == ph;
            for (int y = 0; same && y < h; y++)
                for (int x = 0; same && x < w; x++)
                {
                    int d = 128 + 4 * ((sy + y) * PageSize + sx + x), s = 4 * (y * w + x);
                    same = dds[d] == rgba[s + 2] && dds[d + 1] == rgba[s + 1] && dds[d + 2] == rgba[s] && dds[d + 3] == rgba[s + 3];
                }
            if (same) ok++;
        }
        return ok;
    }
}
