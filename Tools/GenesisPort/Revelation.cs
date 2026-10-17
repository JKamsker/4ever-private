// 4Revelation (C:\Games\4Revelation, x64 client): its .tcd tables are "CTST" encrypted (18-byte header: magic, WORD 1,
// DWORD seedXor, DWORD size, DWORD crc32(plain); key = crc32("X9cyph3r!Key$4096") ^ seedXor, xorshift32 13/17/5 stepped
// every 4 bytes, byte = state >> 8*(i&3)). Resource packs/indexes are plain, our formats. 4Revelation/ = decrypted Tcd +
// junctions to its Data/Index. This file only ANALYSES what it has that we lack (rev-* commands).
using ClassicPort;

public static class Revelation
{
    static string H(byte[] b) => Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(b))[..16];

    /// <summary>TSFX.tcd (CTChart::InitTSFXTEMP): WORD count, rows DWORD id, DWORD sfx, DWORD posId.</summary>
    static Dictionary<uint, (uint sfx, uint pos)> Tsfx(string root)
    {
        var b = File.ReadAllBytes(Path.Combine(root, "Tcd", "TSFX.tcd"));
        int n = BitConverter.ToUInt16(b, 0);
        if (2 + 12 * n != b.Length) throw new InvalidDataException("TSFX.tcd layout");
        return Enumerable.Range(0, n).ToDictionary(i => BitConverter.ToUInt32(b, 2 + 12 * i), i => (BitConverter.ToUInt32(b, 6 + 12 * i), BitConverter.ToUInt32(b, 10 + 12 * i)));
    }

    /// <summary>rev-effects: wing / aura / chakra items we lack -> their TSFX entry -> SFX resource, vs ours.</summary>
    public static void Effects(Client G, Client R, string game, string rev)
    {
        var gi = TItem.Load(Path.Combine(game, "Tcd", "TItem.tcd")); var gIds = gi.Select(i => i.Id).ToHashSet();
        var ri = TItem.Load(Path.Combine(rev, "Tcd", "TItem.tcd"));
        var gs = Tsfx(game); var rs = Tsfx(rev);
        Console.WriteLine($"TSFX rows: ours {gs.Count}, Revelation {rs.Count}; Revelation-only ids: {string.Join(",", rs.Keys.Except(gs.Keys).Order())}");
        // which TSFX ids our own SFX costumes use (to see whether useValue really is a TSFX id)
        Console.WriteLine("ours (kind 250): " + string.Join(" ", gi.Where(i => i.Type == 17 && i.Kind == 250).Take(8).Select(i => $"{i.Id}:{i.UseValue}{(gs.ContainsKey(i.UseValue) ? "" : "(no TSFX)")}")));
        foreach (var it in ri.Where(i => i.Type == 17 && (i.Kind == 250 || i.Kind == 122) && !gIds.Contains(i.Id)))
        {
            string s;
            if (!rs.TryGetValue(it.UseValue, out var t)) s = "no TSFX row";
            else
            {
                var rr = R.Get(Kind.Sfx, t.sfx);
                string ours = !G.Has(Kind.Sfx, t.sfx) ? "NEW sfx" : G.Get(Kind.Sfx, t.sfx)!.Raw.AsSpan().SequenceEqual(rr?.Raw ?? []) ? "sfx same as ours" : "sfx id ours but DIFFERENT";
                string tsfx = !gs.TryGetValue(it.UseValue, out var g) ? "TSFX id new" : g == t ? "TSFX same" : $"TSFX id ours->sfx {g.sfx:X8}";
                s = $"TSFX {it.UseValue} -> sfx {t.sfx:X8} pos {t.pos}: {tsfx}, {ours}";
            }
            Console.WriteLine($"  {it.Id} k{it.Kind,-3} {it.Name,-40} {s}");
        }
    }

    /// <summary>rev-mounts: every Revelation mount, matched to ours by mesh content (+ equip look).</summary>
    public static void Mounts(Client G, Client R, string game, string rev)
    {
        var gMon = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var rMon = TMon.Load(Path.Combine(rev, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var cache = new Dictionary<(bool, uint), HashSet<string>>();
        HashSet<string> Meshes(Client c, uint obj)
        {
            if (cache.TryGetValue((c == G, obj), out var s)) return s;
            s = new();
            if (c.Get(Kind.Obj, obj) is { } r)
                foreach (var (_, id) in r.Refs.Where(x => x.Item1 == Kind.Mesh).Distinct())
                    if (c.Get(Kind.Mesh, id) is { } m) s.Add(H(m.Raw));
            return cache[(c == G, obj)] = s;
        }
        string Look(MonRec m) => Convert.ToHexString(m.Raw, m.TailOfs + 16, 38);
        var ours = TMount.Load(Path.Combine(game, "Tcd", "TMount.tcd"), false).Select(m => gMon[m.MonId]).ToList();
        var allOurMeshes = G.Idx[Kind.Mesh].ById.Keys.Select(id => G.Get(Kind.Mesh, id) is { } r ? H(r.Raw) : "").ToHashSet();
        int dup = 0, variant = 0, fresh = 0, known2 = 0;
        foreach (var rm in TMount.Load(Path.Combine(rev, "Tcd", "TMount.tcd"), false))
        {
            var mon = rMon[rm.MonId]; var ms = Meshes(R, mon.Obj);
            var best = ours.Select(o => (o, j: ms.Count == 0 ? 0 : (double)ms.Intersect(Meshes(G, o.Obj)).Count() / ms.Union(Meshes(G, o.Obj)).Count()))
                .OrderByDescending(x => x.j).ThenByDescending(x => Look(x.o) == Look(mon)).First();
            bool sameLook = best.j == 1 && ours.Any(o => Meshes(G, o.Obj).SetEquals(ms) && Look(o) == Look(mon));
            int known = ms.Count(allOurMeshes.Contains);
            string verdict = sameLook ? "HAVE" : best.j == 1 ? "VARIANT" : ms.Count > 0 && known == ms.Count ? "KNOWN" : "NEW";
            if (sameLook) dup++; else if (best.j == 1) variant++; else if (verdict == "KNOWN") known2++; else fresh++;
            Console.WriteLine($"{verdict,-8} mount {rm.Id,3} {mon.Name,-28} icon {rm.Icon,5} obj {mon.Obj:X8} meshes {known}/{ms.Count} in our client; closest mount '{best.o.Name}' {best.j:P0}");
        }
        Console.WriteLine($"mounts: have {dup}, same model as one of our mounts but other look {variant}, model in our client (not a mount) {known2}, new model {fresh}");
    }

    /// <summary>rev-costumes: Revelation costumes we lack by item id, judged by look (mesh + textures on the 6 player objects).</summary>
    public static void Costumes(Client G, Client R, string game, string rev)
    {
        var hash = new Dictionary<(bool, Kind, uint), string>();
        string HR(Client c, Kind k, uint id) => hash.TryGetValue((c == G, k, id), out var h) ? h : hash[(c == G, k, id)] = c.Get(k, id) is { } r ? H(r.Raw) : "-";
        Dictionary<(uint, uint, uint), string> Sigs(Client c)
        {
            var d = new Dictionary<(uint, uint, uint), List<string>>();
            foreach (var p in Genesis.Players)
                foreach (var k in ObjCloth.Read(c.Get(Kind.Obj, p)!.Raw).Kinds) foreach (var cl in k.Cloths) foreach (var m in cl.Meshes)
                    (d.TryGetValue((k.Id, cl.Id, m.Key), out var l) ? l : d[(k.Id, cl.Id, m.Key)] = new())
                        .Add($"{p:X}:{HR(c, Kind.Mesh, m.MeshId)}:{string.Join(",", m.TexIds.Select(t => t == 0 ? "0" : HR(c, Kind.Tex, t)))}");
            return d.ToDictionary(kv => kv.Key, kv => string.Join("|", kv.Value.Order()));
        }
        var ourSigs = Sigs(G).Values.ToHashSet(); var rSig = Sigs(R);
        var gIds = TItem.Load(Path.Combine(game, "Tcd", "TItem.tcd")).Select(i => i.Id).ToHashSet();
        var rVis = TVisualTcd.Load(Path.Combine(rev, "Tcd", "TItemVisual.tcd")).GroupBy(v => v.Id).ToDictionary(g => g.Key, g => g.First().Raw);
        int have = 0, neu = 0, nolook = 0;
        foreach (var it in TItem.Load(Path.Combine(rev, "Tcd", "TItem.tcd")).Where(i => i.Type == 17 && (i.Kind == 88 || i.Kind == 89 || i.Kind == 90) && !gIds.Contains(i.Id)))
        {
            var v = rVis.GetValueOrDefault(BitConverter.ToUInt16(it.Raw, it.TailOfs + 63));
            var key = v == null ? default : (BitConverter.ToUInt32(v, 10), BitConverter.ToUInt32(v, 14), BitConverter.ToUInt32(v, 18));
            string verdict = v == null || key.Item3 == 0 ? "NO-MESH" : !rSig.TryGetValue(key, out var sig) ? "MISSING" : ourSigs.Contains(sig) ? "HAVE" : "NEW";
            if (verdict == "HAVE") have++; else if (verdict == "NEW") neu++; else nolook++;
            Console.WriteLine($"{verdict,-8} {it.Id} k{it.Kind} {it.Name}");
        }
        Console.WriteLine($"costumes: have {have}, new look {neu}, no mesh/missing {nolook}");
    }
}


/// <summary>4Revelation resource packs. A packed file is a chain of [u32 len][len bytes], each frame XORed with a
/// 20-byte key that restarts per frame. The key depends on the pack type (Action\Character.TAC has its own); recovered
/// from records both clients share (recompressed with zlib level 1) and validated by inflating every frame. A decrypted
/// frame = our chunk [u32 orig][u32 comp][zlib]. Index positions point at the frame (file offset; TFX: offset in the
/// prefix-less stream). Some packs are plain (our format) and are copied as is.</summary>
public static class RevPack
{
    static readonly Dictionary<string, byte[]> Keys = new()
    {
        [".TFX"] = Convert.FromHexString("7F3E9C4BA52DE1086FB219C75AF386D40E9245A8"),
        [".TMH"] = Convert.FromHexString("663D7095F51818EDB30FF2D295435BFE16D8C10F"),
        [".TAC"] = Convert.FromHexString("DFDBC8296334BA04E0AB27BAB21B795C1416B532"),
        [".TTX"] = Convert.FromHexString("D94E55F3CB7E971B17E0A91BC7892336AE678DFE"),
        [".TOB"] = Convert.FromHexString("2CF697FBD290443496F8DF7D3A26809229FEE306"),
        [@"Action\Character.TAC"] = Convert.FromHexString("5A1B6C9D3E2FA08F7E4DBBDCCA1AF90826E57A3B"),
    };
    static readonly Dictionary<string, Kind> Seq = new() { [".TMH"] = Kind.Mesh, [".TAC"] = Kind.Ani, [".TOB"] = Kind.Obj };

    static bool PlainValid(string ext, byte[] b)
    {
        try
        {
            if (Seq.TryGetValue(ext, out var k)) { int p = 0; while (p < b.Length) p += Packs.Length(k, b, p); return p == b.Length; }
            for (int p = 0; p < b.Length;) { Client.Unchunk(b, p, out int len); p += len; }
            return true;
        }
        catch { return false; }
    }

    /// <summary>Decoded pack + old offset (file and stream) -> new offset, or null if a frame does not decrypt.</summary>
    static (byte[] data, Dictionary<uint, uint> fileMap, Dictionary<uint, uint> streamMap)? Decode(string ext, byte[] key, byte[] b)
    {
        var o = new MemoryStream(); var fm = new Dictionary<uint, uint>(); var sm = new Dictionary<uint, uint>();
        int p = 0, q = 0;
        while (p < b.Length)
        {
            if (p + 4 > b.Length) return null;
            int l = BitConverter.ToInt32(b, p);
            if (l < 10 || p + 4 + l > b.Length) return null;
            var f = b[(p + 4)..(p + 4 + l)];
            for (int i = 0; i < f.Length; i++) f[i] ^= key[i % key.Length];
            if (BitConverter.ToInt32(f, 4) != l - 8 || f[8] != 0x78) return null;
            fm[(uint)p] = (uint)o.Position; sm[(uint)q] = (uint)o.Position;
            if (Seq.ContainsKey(ext))
            {
                byte[] rec;
                try { rec = Client.Unchunk(f, 0, out _); if (ext == ".TOB") rec = ObjToOurs(rec); } catch { return null; }
                o.Write(rec);
            }
            else o.Write(f);
            p += 4 + l; q += l;
        }
        return (o.ToArray(), fm, sm);
    }

    /// <summary>A Revelation .TOB record = ours with 54-byte cloth texture entries (our trailing pad byte, always 1 in
    /// our data, dropped): re-insert it after every texture entry.</summary>
    static byte[] ObjToOurs(byte[] b)
    {
        int I(int p) => BitConverter.ToInt32(b, p);
        var o = new MemoryStream(); int q = 1;
        q += 4 + 8 * I(q); q += 4 + 49 * I(q); q += 4 + 20 * I(q);
        int k = I(q); q += 4; for (int i = 0; i < k; i++) { q += 5; q += 4 + I(q); }
        k = I(q); q += 4;
        for (int i = 0; i < k; i++)
        {
            q += 4; q += 4 + 49 * I(q); q += 4 + 20 * I(q);
            int m = I(q); q += 4;
            for (int j = 0; j < m; j++) { q += 8; q += 4 + 49 * I(q); q += 4 + 20 * I(q); }
        }
        o.Write(b, 0, q);
        k = I(q); o.Write(b, q, 4); q += 4;
        for (int i = 0; i < k; i++)
        {
            o.Write(b, q, 4); q += 4; int ncl = I(q); o.Write(b, q, 4); q += 4;
            for (int j = 0; j < ncl; j++)
            {
                o.Write(b, q, 4); q += 4; int nm = I(q); o.Write(b, q, 4); q += 4;
                for (int x = 0; x < nm; x++)
                {
                    o.Write(b, q, 8); q += 8; int nt = I(q); o.Write(b, q, 4); q += 4;
                    for (int t = 0; t < nt; t++) { o.Write(b, q, 54); o.WriteByte(1); q += 54; }
                }
            }
        }
        if (q != b.Length) throw new InvalidDataException("Revelation object record length");
        return o.ToArray();
    }

    /// <summary>rev-decode: 4Revelation\Data + Index rebuilt in our formats (plain copies where already ours).</summary>
    public static void Build(string src, string dst)
    {
        string dData = Path.Combine(dst, "Data"), dIdx = Path.Combine(dst, "Index");
        if (Directory.Exists(dData)) Directory.Delete(dData, true);
        var idxDir = new DirectoryInfo(dIdx);
        if (idxDir.Exists) { if (idxDir.LinkTarget != null) idxDir.Delete(); else idxDir.Delete(true); }
        Directory.CreateDirectory(dIdx);

        var maps = new Dictionary<string, (Dictionary<uint, uint> f, Dictionary<uint, uint> s)>(StringComparer.OrdinalIgnoreCase);
        var stats = new SortedDictionary<string, int>();
        foreach (var file in Directory.EnumerateFiles(Path.Combine(src, "Data"), "*", SearchOption.AllDirectories))
        {
            string rel = Path.GetRelativePath(Path.Combine(src, "Data"), file), ext = Path.GetExtension(file).ToUpperInvariant();
            string outp = Path.Combine(dData, rel);
            Directory.CreateDirectory(Path.GetDirectoryName(outp)!);
            string how = "copied";
            if (Keys.ContainsKey(ext))
            {
                var b = File.ReadAllBytes(file);
                if (PlainValid(ext, b)) { File.WriteAllBytes(outp, b); how = "plain"; }
                else if (Decode(ext, Keys.GetValueOrDefault(rel) ?? Keys[ext], b) is { } d) { File.WriteAllBytes(outp, d.data); maps[rel] = (d.fileMap, d.streamMap); how = "decrypted"; }
                else { File.WriteAllBytes(outp, b); how = "UNDECODED"; Console.WriteLine($"  ? {rel}"); }
            }
            else File.Copy(file, outp, true);
            stats[$"{ext} {how}"] = stats.GetValueOrDefault($"{ext} {how}") + 1;
        }
        foreach (var (k, v) in stats) Console.WriteLine($"{k}: {v}");

        // indexes: positions remapped into the decoded packs
        foreach (var f in Directory.EnumerateFiles(Path.Combine(src, "Index")))
        {
            string name = Path.GetFileName(f);
            if (!name.EndsWith(".IDX", StringComparison.OrdinalIgnoreCase) || name.Equals("TClientI.IDX", StringComparison.OrdinalIgnoreCase) || name.Equals("TClientP.IDX", StringComparison.OrdinalIgnoreCase))
            { File.Copy(f, Path.Combine(dIdx, name), true); continue; }
            var idx = Idx.Load(f);
            int moved = 0, miss = 0;
            idx.Entries = idx.Entries.Select(e =>
            {
                if (!maps.TryGetValue(idx.Files[e.File], out var m)) return e;
                if (m.f.TryGetValue(e.Pos, out var np) || m.s.TryGetValue(e.Pos, out np)) { moved++; return e with { Pos = np }; }
                miss++; return e;
            }).ToList();
            idx.Save(Path.Combine(dIdx, name));
            Console.WriteLine($"{name}: {moved} entries remapped, {miss} unmatched");
        }
        // TClientList.LST etc.
        foreach (var f in Directory.EnumerateFiles(Path.Combine(src, "Index"), "*.LST")) File.Copy(f, Path.Combine(dIdx, Path.GetFileName(f)), true);
    }
}
