// Exploration commands that sized the port (kept to document how the decisions were made).
using ClassicPort;

public static class Explore
{
    static (uint, uint, uint) Key(byte[] v, int o) => (BitConverter.ToUInt32(v, 10), BitConverter.ToUInt32(v, 14), BitConverter.ToUInt32(v, o));

    /// <summary>collide: for the chosen costumes + mounts, how many resources/ids clash with ours.</summary>
    public static void Collide(Client G, Client N, string game, string gen)
    {
        var gp = Genesis.PlayerObjects(N);
        var gVis = TVisualTcd.Load(Path.Combine(game, "Tcd", "TItemVisual.tcd")).ToDictionary(v => v.Id);
        var nVis = TVisualTcd.Load(Path.Combine(gen, "Tcd", "TItemVisual.tcd")).ToDictionary(v => v.Id);
        var gIds = TItemTcd.Load(Path.Combine(game, "Tcd", "TItem.tcd")).Select(i => i.Id).ToHashSet();
        var costumes = TItemTcd.Load(Path.Combine(gen, "Tcd", "TItem.tcd")).Where(i => i.Raw[2] == 17 && !gIds.Contains(i.Id) && i.Id != 31727).ToList();
        var vis = costumes.Select(i => nVis[i.Visual0].Raw).ToList();
        var keys = vis.SelectMany(v => new[] { Key(v, 18), Key(v, 22) }).Where(k => k.Item3 != 0).ToHashSet();
        var funcs = vis.SelectMany(v => new[] { BitConverter.ToUInt32(v, 51), BitConverter.ToUInt32(v, 55) }).Where(f => f != 0).ToHashSet();
        int visCollide = costumes.Count(i => gVis.TryGetValue(i.Visual0, out var gv) && !gv.Raw.AsSpan().SequenceEqual(nVis[i.Visual0].Raw));
        Console.WriteLine($"costume visuals {vis.Count}: visual-id collisions {visCollide}, mesh keys {keys.Count}, effect funcs {funcs.Count} [{string.Join(",", funcs.Select(f => f.ToString("X")))}]");

        // mesh-key collisions: same (clk, cli, key) in our player object with a different mesh entry
        var ourEnt = new Dictionary<(uint, uint, uint, uint), byte[]>();
        foreach (var p in Genesis.Players)
            foreach (var k in ObjCloth.Read(G.Get(Kind.Obj, p)!.Raw).Kinds) foreach (var c in k.Cloths) foreach (var m in c.Meshes)
                ourEnt[(p, k.Id, c.Id, m.Key)] = m.Raw;
        var roots = new List<(Kind, uint)>(); int keyCollide = 0, keySame = 0, entries = 0;
        var funcSfx = new List<(uint p, uint fn, uint sfx)>();
        foreach (var (p, rec) in gp)
        {
            foreach (var k in ObjCloth.Read(rec).Kinds) foreach (var c in k.Cloths) foreach (var m in c.Meshes)
            {
                if (!keys.Contains((k.Id, c.Id, m.Key))) continue;
                entries++;
                if (ourEnt.TryGetValue((p, k.Id, c.Id, m.Key), out var mine)) { if (mine.AsSpan().SequenceEqual(m.Raw)) keySame++; else keyCollide++; }
                roots.Add((Kind.Mesh, m.MeshId)); roots.AddRange(m.TexIds.Select(t => (Kind.Tex, t)));
            }
            // object-level SFX instances (49 bytes each, right after the pivots) tagged with an effect func
            int o = 1; int np = BitConverter.ToInt32(rec, o); o += 4 + 8 * np; int ns = BitConverter.ToInt32(rec, o); o += 4;
            for (int i = 0; i < ns; i++, o += 49)
            {
                uint fn = BitConverter.ToUInt32(rec, o + 37);
                if (funcs.Contains(fn)) { funcSfx.Add((p, fn, BitConverter.ToUInt32(rec, o + 4))); roots.Add((Kind.Sfx, BitConverter.ToUInt32(rec, o + 4))); }
            }
        }
        Console.WriteLine($"cloth entries to splice {entries}: already ours {keySame}, KEY COLLIDES {keyCollide}; func SFX insts {funcSfx.Count} (funcs {string.Join(",", funcSfx.Select(x => x.fn).Distinct().Select(f => f.ToString("X")))})");
        var cc = new Closure(G, N); cc.Walk(roots);
        Console.WriteLine("costume closure: " + cc.Summary());

        // mounts: chosen pets -> monsters -> objects
        int[] chosen = [60, 66, 67, 68, 72, 76, 84, 89, 90, 92, 93, 94, 95, 104, 105, 125];
        var pets = Genesis.LoadPets(Path.Combine(gen, "Tcd", "TPet.tcd")).Where(p => p.Race == 0 && chosen.Contains(p.Id)).ToList();
        var nMon = TMon.Load(Path.Combine(gen, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var mc = new Closure(G, N); mc.Walk(pets.Select(p => (Kind.Obj, nMon[p.MonId].Obj)));
        Console.WriteLine("mount closure:   " + mc.Summary());
        foreach (var x in mc.Collide.Where(x => x.Item1 == Kind.Obj)) Console.WriteLine($"   colliding object {x.Item2:X8}");
    }
}

public static class Explore2
{
    /// <summary>collide2: who references the colliding mount resources, and are the shared objects supersets?</summary>
    public static void Run(Client G, Client N, string gen)
    {
        int[] chosen = [60, 66, 67, 68, 72, 76, 84, 89, 90, 92, 93, 94, 95, 104, 105, 125];
        var pets = Genesis.LoadPets(Path.Combine(gen, "Tcd", "TPet.tcd")).Where(p => p.Race == 0 && chosen.Contains(p.Id)).ToList();
        var nMon = TMon.Load(Path.Combine(gen, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var mc = new Closure(G, N); mc.Walk(pets.Select(p => (Kind.Obj, nMon[p.MonId].Obj)));
        foreach (var (k, id) in mc.Collide)
        {
            var users = mc.New.Concat(mc.Collide).Where(x => N.Get(x.Item1, x.Item2)!.Refs.Contains((k, id))).Select(x => $"{x.Item1}:{x.Item2:X8}");
            var a = G.Get(k, id)!.Raw; var b = N.Get(k, id)!.Raw;
            Console.WriteLine($"{k} {id:X8}: ours {a.Length} B, genesis {b.Length} B, users [{string.Join(",", users)}]");
        }
        foreach (uint o in new uint[] { 0x110B7E1C, 0x110B7E20 })
        {
            var a = ObjCloth.Read(G.Get(Kind.Obj, o)!.Raw); var b = ObjCloth.Read(N.Get(Kind.Obj, o)!.Raw);
            var ka = a.Kinds.SelectMany(k => k.Cloths.SelectMany(c => c.Meshes.Select(m => $"{k.Id:X}/{c.Id:X}/{m.Key:X}"))).ToHashSet();
            var kb = b.Kinds.SelectMany(k => k.Cloths.SelectMany(c => c.Meshes.Select(m => $"{k.Id:X}/{c.Id:X}/{m.Key:X}"))).ToHashSet();
            var ga = G.Get(Kind.Obj, o)!.Raw; var nb = N.Get(Kind.Obj, o)!.Raw;
            Console.WriteLine($"obj {o:X8}: ours {ka.Count} cloth entries, genesis {kb.Count}; ours subset={ka.IsSubsetOf(kb)}; prefix equal={ga.AsSpan(0, a.Start).SequenceEqual(nb.AsSpan(0, b.Start))}; genesis-only [{string.Join(" ", kb.Except(ka))}]");
        }
    }
}

public static class Explore3
{
    static List<string> SfxInsts(byte[] rec)
    {
        int o = 1; int np = BitConverter.ToInt32(rec, o); o += 4 + 8 * np; int ns = BitConverter.ToInt32(rec, o); o += 4;
        var l = new List<string>();
        for (int i = 0; i < ns; i++, o += 49) l.Add($"key {BitConverter.ToUInt32(rec, o):X} sfx {BitConverter.ToUInt32(rec, o + 4):X8} func {BitConverter.ToUInt32(rec, o + 33):X} pivot {BitConverter.ToInt32(rec, o + 28)}");
        return l;
    }

    /// <summary>variants: object-level SFX of the shared mount objects + the monster records using them.</summary>
    public static void Run(Client G, Client N, string game, string gen)
    {
        foreach (uint o in new uint[] { 0x110B7E1C, 0x110B7E20 })
        {
            Console.WriteLine($"obj {o:X8} ours:    {string.Join(" | ", SfxInsts(G.Get(Kind.Obj, o)!.Raw))}");
            Console.WriteLine($"obj {o:X8} genesis: {string.Join(" | ", SfxInsts(N.Get(Kind.Obj, o)!.Raw))}");
        }
        var g = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var n = TMon.Load(Path.Combine(gen, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        foreach (var (c, id) in new[] { (g, (ushort)31855), (g, (ushort)31859), (g, (ushort)31862), (n, (ushort)31656), (n, (ushort)31896), (n, (ushort)31897), (n, (ushort)31898), (n, (ushort)31899), (n, (ushort)31900), (g, (ushort)31002) })
            Console.WriteLine($"{(c == g ? "ours" : "gen ")} {id} {c[id].Name,-20} tail {Convert.ToHexString(c[id].Raw, c[id].TailOfs, 125)}");
    }
}

public static class IconCheck
{
    /// <summary>icons-verify: cut shared icon indices from both clients (Genesis v1 TIF, ours v2) and compare pixels;
    /// dumps a few PNGs to the given folder for eyeballing.</summary>
    public static void Run(string game, string gen, string outDir)
    {
        var a = new IconCut(game); var b = new IconCut(gen);
        Console.WriteLine($"item icon lists: ours {a.Count}, genesis {b.Count}");
        int same = 0, diff = 0, none = 0;
        for (int i = 0; i < Math.Min(a.Count, b.Count); i++)
        {
            var x = a.Cut(i); var y = b.Cut(i);
            if (x == null || y == null) { none++; continue; }
            if (x.Value.w == y.Value.w && x.Value.h == y.Value.h && x.Value.rgba.AsSpan().SequenceEqual(y.Value.rgba)) same++; else diff++;
        }
        Console.WriteLine($"shared indices: pixel-identical {same}, different {diff}, missing {none}");
        Directory.CreateDirectory(outDir);
        foreach (int i in new[] { 0, 100, 1777, 2085, 2205, 2213 })
        {
            if (b.Cut(i) is { } g) File.WriteAllBytes(Path.Combine(outDir, $"gen_{i}.png"), Png.Encode(g.w, g.h, g.rgba));
            if (a.Cut(i) is { } o) File.WriteAllBytes(Path.Combine(outDir, $"our_{i}.png"), Png.Encode(o.w, o.h, o.rgba));
        }
    }
}

public static class TifProbe
{
    /// <summary>tifprobe: locate the item-icon list in Genesis' (non-standard) TIF by our first sprite ids.</summary>
    public static void Run(string game, string gen)
    {
        var ours = new IconCut(game).Sprites;
        var b = File.ReadAllBytes(Path.Combine(gen, "TClientCmd.tif"));
        uint s0 = ours[0], s1 = ours[1], s2 = ours[2];
        Console.WriteLine($"our first sprites {s0:X} {s1:X} {s2:X}");
        for (int p = 0; p + 4 <= b.Length; p++)
        {
            if (BitConverter.ToUInt32(b, p) != s0) continue;
            for (int stride = 100; stride < 400; stride++)
                if (p + 2 * stride + 4 <= b.Length && BitConverter.ToUInt32(b, p + stride) == s1 && BitConverter.ToUInt32(b, p + 2 * stride) == s2)
                {
                    int n = 0; while (p + (n + 1) * stride + 4 <= b.Length && n < 5000 && (n >= ours.Count || BitConverter.ToUInt32(b, p + n * stride) == ours[n])) n++;
                    int count = BitConverter.ToInt32(b, p - 57 - 4);
                    Console.WriteLine($"hit at {p} stride {stride}: matches our list for {n} entries; parent child count {count}");
                }
        }
    }
}

public static class PackCheck
{
    /// <summary>packcheck: the exact-length walkers consume whole packs of both clients.</summary>
    public static void Run(string game, string gen)
    {
        foreach (var (k, dir, ext) in new[] { (Kind.Mesh, "Mesh", "*.TMH"), (Kind.Ani, "Action", "*.TAC"), (Kind.Obj, "OBJ", "*.TOB") })
            foreach (var root in new[] { game, gen })
            {
                int ok = 0, bad = 0; long recs = 0;
                foreach (var f in Directory.GetFiles(Path.Combine(root, "Data", dir), ext))
                {
                    try { var (n, rest) = Packs.WalkFile(k, f); recs += n; if (rest == 0) ok++; else { bad++; Console.WriteLine($"  {f}: rest {rest}"); } }
                    catch (Exception e) { bad++; Console.WriteLine($"  {f}: {e.GetType().Name}"); }
                }
                Console.WriteLine($"{k} {Path.GetFileName(root)}: {ok} packs exact, {bad} bad, {recs} records");
            }
    }
}

public static class FuncSearch
{
    /// <summary>funcsearch: every SFX instance (object / action / ani level) of every Genesis object whose func id matches.</summary>
    public static void Run(Client N, uint[] funcs)
    {
        var hits = new List<string>();
        void Scan(string who, byte[] rec)
        {
            var s = new BinaryReader(new MemoryStream(rec));
            void Sfx(string where, int n)
            {
                for (int i = 0; i < n; i++)
                {
                    uint key = s.ReadUInt32(), sfx = s.ReadUInt32(); s.BaseStream.Seek(28, SeekOrigin.Current);
                    s.ReadByte(); uint fn = s.ReadUInt32(); s.BaseStream.Seek(8, SeekOrigin.Current);
                    if (funcs.Contains(fn)) hits.Add($"{who} {where} key {key:X} sfx {sfx:X8} func {fn:X}");
                }
            }
            void Snd(int n) => s.BaseStream.Seek(20 * n, SeekOrigin.Current);
            s.ReadByte(); int np = s.ReadInt32(); s.BaseStream.Seek(8 * np, SeekOrigin.Current);
            Sfx("obj", s.ReadInt32()); Snd(s.ReadInt32());
            int na = s.ReadInt32();
            for (int i = 0; i < na; i++) { s.ReadUInt32(); int sz = s.ReadByte() == 0 ? 0 : 0; s.BaseStream.Seek(-1, SeekOrigin.Current); s.ReadByte(); s.BaseStream.Seek(s.ReadInt32(), SeekOrigin.Current); }
            int nact = s.ReadInt32();
            for (int i = 0; i < nact; i++)
            {
                uint act = s.ReadUInt32();
                Sfx($"act {act:X}", s.ReadInt32()); Snd(s.ReadInt32());
                int nani = s.ReadInt32();
                for (int j = 0; j < nani; j++) { uint ani = s.ReadUInt32(); s.ReadUInt32(); Sfx($"act {act:X} ani {ani:X}", s.ReadInt32()); Snd(s.ReadInt32()); }
            }
        }
        foreach (var (p, rec) in Genesis.PlayerObjects(N)) Scan($"player {p:X}", rec);
        foreach (var id in N.Idx[Kind.Obj].ById.Keys)
        {
            try { if (N.Get(Kind.Obj, id) is { } r) Scan($"obj {id:X8}", r.Raw); } catch (FileNotFoundException) { }
        }
        Console.WriteLine($"{hits.Count} hits"); foreach (var h in hits.Take(60)) Console.WriteLine("  " + h);
    }
}

public static class MountIcons
{
    /// <summary>mounticons: per ported mount, the TPet icon vs the icons of every Genesis item that makes it; and Red Rocket objects.</summary>
    public static void Run(Client N, string gen)
    {
        var items = TItem.Load(Path.Combine(gen, "Tcd", "TItem.tcd"));
        var vis = TVisualTcd.Load(Path.Combine(gen, "Tcd", "TItemVisual.tcd")).GroupBy(v => v.Id).ToDictionary(g => g.Key, g => g.First().Raw);
        var pets = Genesis.LoadPets(Path.Combine(gen, "Tcd", "TPet.tcd")).Where(p => p.Race == 0).GroupBy(p => p.Id).ToDictionary(g => g.Key, g => g.First());
        var mon = TMon.Load(Path.Combine(gen, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        foreach (ushort pid in new ushort[] { 60, 66, 67, 68, 72, 76, 84, 89, 90, 92, 95, 104, 105, 125 })
        {
            var its = items.Where(i => i.Type == 12 && i.UseValue == pid).Select(i => $"{i.Id}:{BitConverter.ToUInt16(vis[BitConverter.ToUInt16(i.Raw, i.TailOfs + 63)], 34)}");
            Console.WriteLine($"pet {pid,3} {mon[pets[pid].MonId].Name,-28} petIcon {pets[pid].Icon,5}  items [{string.Join(" ", its)}]");
        }
        foreach (uint o in new uint[] { 0x476501C7, 0x4765017E })
        {
            var r = N.Get(Kind.Obj, o)!;
            var ms = r.Refs.Where(x => x.Item1 == Kind.Mesh).Select(x => Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(N.Get(Kind.Mesh, x.Item2)!.Raw))[..8]).Distinct();
            var ts = r.Refs.Where(x => x.Item1 == Kind.Tex).Select(x => Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(N.Get(Kind.Tex, x.Item2)!.Raw))[..8]).Distinct();
            Console.WriteLine($"obj {o:X8}: {r.Raw.Length} B, meshes [{string.Join(",", ms)}] tex [{string.Join(",", ts)}]");
        }
        foreach (ushort m in new ushort[] { 31657, 31651 }) Console.WriteLine($"mon {m} {mon[m].Name} obj {mon[m].Obj:X8} tail {Convert.ToHexString(mon[m].Raw, mon[m].TailOfs, 60)}");
    }
}

public static class IconSheet
{
    /// <summary>iconsheet from to out.png: Genesis icons [from,to) on a labelled-by-position grid (16 per row, 34 px cells).</summary>
    public static void Run(string gen, int from, int to, string outPath)
    {
        var cut = new IconCut(gen);
        int cols = 16, cell = 34, rows = (to - from + cols - 1) / cols;
        int W = cols * cell, H = rows * cell;
        var img = new byte[4 * W * H];
        for (int i = 0; i < W * H; i++) { img[4 * i] = 40; img[4 * i + 1] = 40; img[4 * i + 2] = 40; img[4 * i + 3] = 255; }
        for (int idx = from; idx < to; idx++)
        {
            if (cut.Cut(idx) is not { } c) continue;
            int k = idx - from, x0 = k % cols * cell + 1, y0 = k / cols * cell + 1;
            for (int y = 0; y < Math.Min(c.h, 32); y++)
                for (int x = 0; x < Math.Min(c.w, 32); x++)
                {
                    int s = 4 * (y * c.w + x), d = 4 * ((y0 + y) * W + x0 + x); int a = c.rgba[s + 3];
                    for (int ch = 0; ch < 3; ch++) img[d + ch] = (byte)((c.rgba[s + ch] * a + img[d + ch] * (255 - a)) / 255);
                }
        }
        File.WriteAllBytes(outPath, Png.Encode(W, H, img));
    }
}

public static class MountSfx
{
    static List<(string where, uint sfx, uint func)> Insts(byte[] rec)
    {
        var o = new List<(string, uint, uint)>();
        var s = new BinaryReader(new MemoryStream(rec));
        void Sfx(string w, int n) { for (int i = 0; i < n; i++) { s.ReadUInt32(); uint sfx = s.ReadUInt32(); s.BaseStream.Seek(29, SeekOrigin.Current); uint fn = s.ReadUInt32(); s.BaseStream.Seek(8, SeekOrigin.Current); o.Add((w, sfx, fn)); } }
        void Snd(int n) => s.BaseStream.Seek(20 * n, SeekOrigin.Current);
        s.ReadByte(); s.BaseStream.Seek(8 * s.ReadInt32(), SeekOrigin.Current);
        Sfx("obj", s.ReadInt32()); Snd(s.ReadInt32());
        int na = s.ReadInt32(); for (int i = 0; i < na; i++) { s.ReadUInt32(); s.ReadByte(); s.BaseStream.Seek(s.ReadInt32(), SeekOrigin.Current); }
        int nact = s.ReadInt32();
        for (int i = 0; i < nact; i++)
        {
            uint act = s.ReadUInt32(); Sfx($"act{act:X}", s.ReadInt32()); Snd(s.ReadInt32());
            int nani = s.ReadInt32();
            for (int j = 0; j < nani; j++) { s.ReadUInt32(); s.ReadUInt32(); Sfx($"act{act:X}/ani", s.ReadInt32()); Snd(s.ReadInt32()); }
        }
        return o;
    }

    /// <summary>mountsfx: SFX instances of every ported mount object, Genesis vs ours (after the port).</summary>
    public static void Run(Client N, string game, string gen)
    {
        var G2 = new Client(game, "2_TClientS.IDX", false);
        var gm = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).Where(m => m.Id >= 31966 && m.Id <= 31979);
        foreach (var m in gm)
        {
            var nRec = N.Get(Kind.Obj, m.Obj)!.Raw; var gRec = G2.Get(Kind.Obj, m.Obj)!.Raw;
            var ni = Insts(nRec); var gi = Insts(gRec);
            bool same = nRec.AsSpan().SequenceEqual(gRec);
            var missingSfx = ni.Where(x => x.sfx != 0 && !G2.Has(Kind.Sfx, x.sfx)).Select(x => x.sfx.ToString("X8")).Distinct();
            Console.WriteLine($"{m.Id} {m.Name,-28} obj {m.Obj:X8} {(same ? "same as Genesis" : "OURS DIFFERS")}: genesis {ni.Count} SFX insts ({string.Join(",", ni.GroupBy(x => x.where.Split('/')[0].StartsWith("act") ? "act" : x.where).Select(g => $"{g.Key}:{g.Count()}"))}), ours {gi.Count}; missing sfx [{string.Join(",", missingSfx)}]; spawnSFX/equip tail {Convert.ToHexString(m.Raw, m.TailOfs + 16, 6)}");
        }
    }
}

public static class MountDups
{
    /// <summary>mountdups: new mount monsters that share object + whole template tail (= identical look).</summary>
    public static void Run(string game)
    {
        var gm = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).Where(m => m.Id >= 31966 && m.Id <= 31979).ToList();
        foreach (var g in gm.GroupBy(m => Convert.ToHexString(m.Raw, m.TailOfs, TMon.OurTail)).Where(g => g.Count() > 1))
            Console.WriteLine("identical look: " + string.Join(" | ", g.Select(m => $"{m.Id} {m.Name}")));
        foreach (var g in gm.GroupBy(m => m.Obj).Where(g => g.Count() > 1))
            foreach (var m in g) Console.WriteLine($"shared obj {m.Obj:X8}: {m.Id} {m.Name} equip {Convert.ToHexString(m.Raw, m.TailOfs + 16, 38)}");
    }
}

public static class WingInst
{
    /// <summary>winginst: the wing SFX instances in our (ported) vs Genesis player objects + their pivot tables.</summary>
    public static void Run(Client N, string game)
    {
        var G2 = new Client(game, "2_TClientS.IDX", false);
        var gp = Genesis.PlayerObjects(N);
        foreach (uint p in new uint[] { 0xF56A4 })
        {
            foreach (var (who, rec) in new[] { ("ours", G2.Get(Kind.Obj, p)!.Raw), ("gen ", gp[p]) })
            {
                int np = BitConverter.ToInt32(rec, 1);
                var piv = Enumerable.Range(0, np).Select(i => (idx: BitConverter.ToUInt32(rec, 5 + 8 * i), id: BitConverter.ToUInt32(rec, 9 + 8 * i))).ToList();
                Console.WriteLine($"{who} {p:X}: pivotCount byte {rec[0]}, {np} pivot entries, max index {piv.Max(x => x.idx)}");
                int o = 5 + 8 * np, ns = BitConverter.ToInt32(rec, o);
                for (int i = 0; i < ns; i++)
                {
                    int q = o + 4 + 49 * i; uint fn = BitConverter.ToUInt32(rec, q + 37);
                    if (fn < 0x6F68 || fn > 0x6F6C) continue;
                    int pivot = BitConverter.ToInt32(rec, q + 32);
                    Console.WriteLine($"   inst key {BitConverter.ToUInt32(rec, q):X} sfx {BitConverter.ToUInt32(rec, q + 4):X8} pos ({BitConverter.ToSingle(rec, q + 8):F2},{BitConverter.ToSingle(rec, q + 12):F2},{BitConverter.ToSingle(rec, q + 16):F2}) pivot {pivot} (id->{string.Join(",", piv.Where(x => x.idx == pivot).Select(x => x.id.ToString("X")))}) bias {rec[q + 36]} func {fn:X} tick {BitConverter.ToUInt32(rec, q + 41)} delOnStop {BitConverter.ToInt32(rec, q + 45)}");
                }
            }
            foreach (uint s in new uint[] { 0x47660017, 0x4766001A })
            {
                var r = G2.Get(Kind.Sfx, s); var n = N.Get(Kind.Sfx, s);
                Console.WriteLine($"sfx {s:X8}: ours {(r == null ? "MISSING" : r.Raw.Length + " B")}, same={r != null && n != null && r.Raw.AsSpan().SequenceEqual(n.Raw)}, refs [{string.Join(",", n!.Refs.Select(x => $"{x.Item1}:{x.Item2:X8}{(G2.Has(x.Item1, x.Item2) ? "" : "(MISSING)")}"))}]");
            }
        }
    }
}

public static class TexDump
{
    /// <summary>texdump id out.png: a texture record's image (TEXTURESET: refs, keys, tick/mip/opt/fmt, orgsize, size + DDS).</summary>
    public static void Run(string game, uint id, string outPath)
    {
        var G = new Client(game, "2_TClientS.IDX", false);
        var r = G.Get(Kind.Tex, id)!.Raw; int p = 0;
        int n = BitConverter.ToInt32(r, p); p += 4 + 4 * n;
        n = BitConverter.ToInt32(r, p); p += 4 + 24 * n;
        p += 12 + 2; int org = BitConverter.ToInt32(r, p); p += 4; int size = BitConverter.ToInt32(r, p); p += 4;
        byte[] blob = r[p..(p + size)];
        if (BitConverter.ToUInt32(blob, 0) != 0x20534444)
        {
            using var z = new System.IO.Compression.ZLibStream(new MemoryStream(blob), System.IO.Compression.CompressionMode.Decompress);
            var ms = new MemoryStream(); z.CopyTo(ms); blob = ms.ToArray();
        }
        var (w, h, rgba) = Dds.Decode(blob);
        Console.WriteLine($"tex {id:X8}: {w}x{h} fmt {System.Text.Encoding.ASCII.GetString(blob, 84, 4)} orgsize {org}");
        File.WriteAllBytes(outPath, Png.Encode(w, h, rgba));
    }
}

public static class RevWalk
{
    /// <summary>rev-walk: decoded Revelation sequential packs parse with our walkers?</summary>
    public static void Run(string rev)
    {
        foreach (var (k, dir, ext) in new[] { (Kind.Obj, "OBJ", "*.TOB"), (Kind.Mesh, "Mesh", "*.TMH"), (Kind.Ani, "Action", "*.TAC") })
            foreach (var f in Directory.GetFiles(Path.Combine(rev, "Data", dir), ext))
            {
                try { var (n, rest) = Packs.WalkFile(k, f); if (rest != 0) Console.WriteLine($"{Path.GetFileName(f)}: {n} records, rest {rest}"); }
                catch (Exception e) { Console.WriteLine($"{Path.GetFileName(f)}: {e.GetType().Name}"); }
            }
        Console.WriteLine("done");
    }
}

public static class RevObjCmp
{
    /// <summary>rev-objcmp obj: refs of one object in both clients, with mesh/texture byte equality.</summary>
    public static void Run(Client G, Client R, uint obj)
    {
        foreach (var (n, c) in new[] { ("ours", G), ("rev ", R) })
        {
            var r = c.Get(Kind.Obj, obj);
            Console.WriteLine($"{n} obj {obj:X8}: {(r == null ? "missing" : r.Raw.Length + " B")} refs [{(r == null ? "" : string.Join(" ", r.Refs.Distinct().Take(12).Select(x => $"{x.Item1}:{x.Item2:X}")))}]");
        }
        var rr = R.Get(Kind.Obj, obj)!;
        foreach (var (k, id) in rr.Refs.Where(x => x.Item1 is Kind.Mesh or Kind.Tex).Distinct().Take(8))
        {
            var a = G.Get(k, id); var b = R.Get(k, id);
            Console.WriteLine($"  {k} {id:X8}: ours {(a == null ? "-" : a.Raw.Length.ToString())} rev {(b == null ? "-" : b.Raw.Length.ToString())} same={a != null && b != null && a.Raw.AsSpan().SequenceEqual(b.Raw)}");
        }
    }
}

public static class RevShape
{
    /// <summary>rev-shape: for Revelation mounts whose meshes are not byte-identical to any of ours, find our meshes with
    /// the same shape (bones, vertices, record size) and which of our mounts/monsters use them.</summary>
    public static void Run(Client G, Client R, string game, string rev)
    {
        (int, int, int) Shape(byte[] m) => (BitConverter.ToInt32(m, 4), BitConverter.ToInt32(m, 29 + 64 * BitConverter.ToInt32(m, 4)), m.Length);
        var byShape = new Dictionary<(int, int, int), List<uint>>();
        foreach (var id in G.Idx[Kind.Mesh].ById.Keys)
            if (G.Get(Kind.Mesh, id) is { } r) (byShape.TryGetValue(Shape(r.Raw), out var l) ? l : byShape[Shape(r.Raw)] = new()).Add(id);
        var gMon = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail);
        var objOfMesh = new Dictionary<uint, List<uint>>();
        foreach (var o in gMon.Select(m => m.Obj).Distinct())
            if (o != 0 && G.Get(Kind.Obj, o) is { } r)
                foreach (var (_, mid) in r.Refs.Where(x => x.Item1 == Kind.Mesh)) (objOfMesh.TryGetValue(mid, out var l) ? l : objOfMesh[mid] = new()).Add(o);
        var monByObj = gMon.GroupBy(m => m.Obj).ToDictionary(g => g.Key, g => g.Select(m => m.Name).Where(n => n != "_").Distinct().Take(3));
        var rMon = TMon.Load(Path.Combine(rev, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        foreach (var rm in TMount.Load(Path.Combine(rev, "Tcd", "TMount.tcd"), false).Where(m => m.Id >= 50))
        {
            var mon = rMon[rm.MonId]; var r = R.Get(Kind.Obj, mon.Obj)!;
            var hits = new List<string>();
            foreach (var (_, mid) in r.Refs.Where(x => x.Item1 == Kind.Mesh).Distinct())
            {
                var shape = Shape(R.Get(Kind.Mesh, mid)!.Raw);
                foreach (var gid in byShape.GetValueOrDefault(shape) ?? new())
                    foreach (var o in objOfMesh.GetValueOrDefault(gid) ?? new())
                        hits.Add($"{string.Join("/", monByObj.GetValueOrDefault(o) ?? [])}");
            }
            Console.WriteLine($"mount {rm.Id,3} {mon.Name,-24} same-shape mesh in ours: {(hits.Count == 0 ? "none" : string.Join(", ", hits.Distinct().Take(3)))}");
        }
    }
}
