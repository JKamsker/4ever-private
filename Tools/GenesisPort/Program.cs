// GenesisPort — pulls mounts + costumes from the 4Genesis client into ours (Game/).
// Source is 4Genesis/ (the TCE1-decrypted mirror of 4Genesis-installer/, our formats).
//   dotnet run -- port [--apply]   import the chosen mounts + all missing costumes (Import.cs) + SQL migration 017
//   dotnet run -- analyze          what Genesis has that we lack (and the other exploration commands in Explore.cs)
using ClassicPort;

string repo = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "../../../../.."));
string game = Path.Combine(repo, "Game"), gen = Path.Combine(repo, "4Genesis");
string cmd = args.FirstOrDefault() ?? "analyze";

var G = new Client(game, "2_TClientS.IDX", false);
var N = new Client(gen, "2_TClientS.IDX", false);
Console.WriteLine($"Genesis catalogs: obj {N.Idx[Kind.Obj].ById.Count} mesh {N.Idx[Kind.Mesh].ById.Count} tex {N.Idx[Kind.Tex].ById.Count} ani {N.Idx[Kind.Ani].ById.Count} sfx {N.Idx[Kind.Sfx].ById.Count}");
Console.WriteLine($"ours:             obj {G.Idx[Kind.Obj].ById.Count} mesh {G.Idx[Kind.Mesh].ById.Count} tex {G.Idx[Kind.Tex].ById.Count} ani {G.Idx[Kind.Ani].ById.Count} sfx {G.Idx[Kind.Sfx].ById.Count}");

if (cmd == "rev-decode") { RevPack.Build(@"C:\Games\4Revelation", Path.Combine(repo, "4Revelation")); return; }
if (cmd.StartsWith("rev-"))
{
    string rev = Path.Combine(repo, "4Revelation");
    var R = new Client(rev, "2_TClientS.IDX", false);
    if (cmd == "rev-effects") Revelation.Effects(G, R, game, rev);
    if (cmd == "rev-mounts") Revelation.Mounts(G, R, game, rev);
    if (cmd == "rev-costumes") Revelation.Costumes(G, R, game, rev);
    if (cmd == "rev-walk") RevWalk.Run(rev);
    if (cmd == "rev-shape") RevShape.Run(G, R, game, rev);
    if (cmd == "rev-objcmp") RevObjCmp.Run(G, R, Convert.ToUInt32(args[1], 16));
    return;
}
if (cmd == "port")
{
    string mig = Path.Combine(repo, "Database", "migrations", "017_genesis_mounts_costumes.sql");
    new Import(game, gen, mig, G, N, Console.Out).Run(args.Contains("--apply"));
    return;
}
if (cmd == "collide") Explore.Collide(G, N, game, gen);
if (cmd == "collide2") Explore2.Run(G, N, gen);
if (cmd == "variants") Explore3.Run(G, N, game, gen);
if (cmd == "icons-verify") IconCheck.Run(game, gen, args[1]);
if (cmd == "tifprobe") TifProbe.Run(game, gen);
if (cmd == "packcheck") PackCheck.Run(game, gen);
if (cmd == "texdump") TexDump.Run(game, Convert.ToUInt32(args[1], 16), args[2]);
if (cmd == "winginst") WingInst.Run(N, game);
if (cmd == "mountdups") MountDups.Run(game);
if (cmd == "mountsfx") MountSfx.Run(N, game, gen);
if (cmd == "iconsheet") IconSheet.Run(args.Length > 4 ? args[4] : gen, int.Parse(args[1]), int.Parse(args[2]), args[3]);
if (cmd == "mounticons") MountIcons.Run(N, gen);
if (cmd == "funcsearch") FuncSearch.Run(N, [0x6F68, 0x6F69, 0x6F6A, 0x6F6B, 0x6F6C]);
if (cmd == "mon-info")    // mon-info <monId...> : name + object of our monsters (e.g. compare two mounts' looks)
{
    var mons = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
    foreach (var a in args.Skip(1))
        Console.WriteLine(mons.TryGetValue(ushort.Parse(a), out var m) ? $"{m.Id} '{m.Name}' object {m.Obj:X8}" : $"{a}: not found");
}
if (cmd == "icon-dump")   // icon-dump <outDir> <index...> : our item-icon indices -> PNG (>=60000 = Data\Img\Custom\<id>.png)
{
    var cut = new IconCut(game);
    Directory.CreateDirectory(args[1]);
    foreach (var a in args.Skip(2))
    {
        int idx = int.Parse(a);
        string dst = Path.Combine(args[1], $"{idx}.png");
        if (idx >= 60000) { string src = Path.Combine(game, "Data", "Img", "Custom", $"{idx}.png"); if (File.Exists(src)) File.Copy(src, dst, true); else Console.WriteLine($"{idx}: no custom png"); continue; }
        var ic = cut.Cut(idx);
        if (ic is { } x) File.WriteAllBytes(dst, Png.Encode(x.w, x.h, x.rgba)); else Console.WriteLine($"{idx}: no sprite");
    }
}

if (cmd == "analyze")
{
    var gItems = TItemTcd.Load(Path.Combine(game, "Tcd", "TItem.tcd")).ToDictionary(i => i.Id);
    var nItems = TItemTcd.Load(Path.Combine(gen, "Tcd", "TItem.tcd"));
    var gVis = TVisualTcd.Load(Path.Combine(game, "Tcd", "TItemVisual.tcd")).ToDictionary(v => v.Id);
    var nVis = TVisualTcd.Load(Path.Combine(gen, "Tcd", "TItemVisual.tcd")).ToDictionary(v => v.Id);

    // ---- costumes: visual -> (clk, cli, key) present in our player objects? ----
    uint[] players = [0xF56A4, 0xF56A5, 0xF56A6, 0xF56A7, 0xF56A8, 0xF56A9];
    HashSet<(uint, uint, uint)> Keys(Client c)
    {
        var s = new HashSet<(uint, uint, uint)>();
        foreach (var o in players)
            foreach (var k in ObjCloth.Read(c.Get(Kind.Obj, o)!.Raw).Kinds)
                foreach (var cl in k.Cloths) foreach (var m in cl.Meshes) s.Add((k.Id, cl.Id, m.Key));
        return s;
    }
    var gKeys = Keys(G); var nKeys = Keys(N);
    int same = 0, visDiff = 0, needKey = 0, missing = 0;
    foreach (var it in nItems.Where(i => i.Raw[2] == 17 && !gItems.ContainsKey(i.Id)))
    {
        ushort v = it.Visual0; var name = TItemTcd.Cp949.GetString(it.Raw, it.NameOfs + 1, it.NameLen - 1);
        if (!nVis.TryGetValue(v, out var nv)) { Console.WriteLine($"  {it.Id} {name}: visual {v} missing in Genesis"); missing++; continue; }
        uint clk = BitConverter.ToUInt32(nv.Raw, 10), cli = BitConverter.ToUInt32(nv.Raw, 14), key = BitConverter.ToUInt32(nv.Raw, 18);
        bool ourVis = gVis.TryGetValue(v, out var gv), visSame = ourVis && gv!.Raw.AsSpan().SequenceEqual(nv.Raw);
        bool haveKey = key == 0 || gKeys.Contains((clk, cli, key));
        if (visSame && haveKey) same++; else if (!haveKey) needKey++; else visDiff++;
        Console.WriteLine($"  {it.Id,5} k{it.Raw[3],-3} {name,-42} vis {v,5} {(ourVis ? (visSame ? "same" : "DIFF") : "new "),-4} clk {clk:X} cli {cli:X} key {key:X} ours={haveKey} gen={nKeys.Contains((clk, cli, key)) || key == 0} icon {BitConverter.ToUInt16(nv.Raw, TVisualTcd.IconOfs)}");
    }
    Console.WriteLine($"COSTUMES: visual+mesh already ours {same}, visual differs {visDiff}, need mesh entries {needKey}, broken {missing}");
}

if (cmd == "walk")
{
    var starts = Walk.TobStarts(Path.Combine(gen, "Data", "OBJ", "4Genesis.TOB"));
    var set = starts.ToHashSet();
    var idx = N.Idx[Kind.Obj];
    Console.WriteLine($"4Genesis.TOB: {starts.Count} records");
    int f6 = idx.Files.IndexOf(@"OBJ\4Genesis.TOB");
    Console.WriteLine($"indexed file-6 positions that are record starts: {idx.Entries.Count(e => e.File == f6 && set.Contains(e.Pos))}/{idx.Entries.Count(e => e.File == f6)}");
    // For each missing pack, find a base B so that every entry's B+pos is a record start.
    for (int f = 0; f < idx.Files.Count; f++)
    {
        if (File.Exists(Path.Combine(gen, "Data", idx.Files[f]))) continue;
        var ps = idx.Entries.Where(e => e.File == f).Select(e => (long)e.Pos).ToList();
        var bases = starts.Select(s => s - ps.Min()).Where(B => ps.All(p => set.Contains(B + p))).ToList();
        Console.WriteLine($"{idx.Files[f]}: {ps.Count} entries, candidate bases [{string.Join(",", bases.Take(5))}]");
    }
}

if (cmd == "orphans")
{
    string path = Path.Combine(gen, "Data", "OBJ", "4Genesis.TOB");
    var b = File.ReadAllBytes(path);
    var starts = Walk.TobStarts(path);
    var idx = N.Idx[Kind.Obj];
    int f6 = idx.Files.IndexOf(@"OBJ\4Genesis.TOB");
    var indexed = idx.Entries.Where(e => e.File == f6).Select(e => (long)e.Pos).ToHashSet();
    var nVis = TVisualTcd.Load(Path.Combine(gen, "Tcd", "TItemVisual.tcd"));
    var nItems = TItemTcd.Load(Path.Combine(gen, "Tcd", "TItem.tcd"));
    var gIds = TItemTcd.Load(Path.Combine(game, "Tcd", "TItem.tcd")).Select(i => i.Id).ToHashSet();
    var visById = nVis.ToDictionary(v => v.Id);
    var want = nItems.Where(i => i.Raw[2] == 17 && !gIds.Contains(i.Id) && visById.ContainsKey(i.Visual0))
        .Select(i => visById[i.Visual0].Raw).Select(v => (BitConverter.ToUInt32(v, 10), BitConverter.ToUInt32(v, 14), BitConverter.ToUInt32(v, 18)))
        .Where(k => k.Item3 != 0).ToHashSet();
    Console.WriteLine($"new costume mesh keys wanted: {want.Count}");
    // our player objects' prefixes (everything before the cloth section) to identify orphans
    uint[] players = [0xF56A4, 0xF56A5, 0xF56A6, 0xF56A7, 0xF56A8, 0xF56A9];
    var ourPrefix = players.ToDictionary(p => p, p => { var r = G.Get(Kind.Obj, p)!.Raw; return r[..ObjCloth.Read(r).Start]; });
    for (int i = 0; i < starts.Count; i++)
    {
        long s = starts[i], e = i + 1 < starts.Count ? starts[i + 1] : b.Length;
        if (indexed.Contains(s)) continue;
        var rec = b[(int)s..(int)e];
        ObjCloth cl;
        try { cl = ObjCloth.Read(rec); } catch { continue; }
        var keys = cl.Kinds.SelectMany(k => k.Cloths.SelectMany(c => c.Meshes.Select(m => (k.Id, c.Id, m.Key)))).ToHashSet();
        int hit = keys.Count(want.Contains);
        if (hit == 0 && keys.Count < 50) continue;
        var pre = rec[..cl.Start];
        var who = ourPrefix.Where(kv => kv.Value.AsSpan().SequenceEqual(pre)).Select(kv => $"{kv.Key:X}").FirstOrDefault() ?? "?";
        Console.WriteLine($"orphan @{s} len {rec.Length} meshEntries {keys.Count} wantedHits {hit} prefix==our {who}");
    }
}

if (cmd == "mounts")
{
    var tp = File.ReadAllBytes(Path.Combine(gen, "Tcd", "TPet.tcd"));
    int n = BitConverter.ToUInt16(tp, 0);
    var pets = new SortedDictionary<int, (ushort icon, ushort mon, ushort speed)>();
    for (int i = 0; i < n; i++)
    {
        int o = 2 + 17 * i; ushort key = BitConverter.ToUInt16(tp, o);
        if ((key & 0xFF) != 0) continue;
        pets[key >> 8] = (BitConverter.ToUInt16(tp, o + 13), BitConverter.ToUInt16(tp, o + 15), BitConverter.ToUInt16(tp, o + 4));
    }
    var gMon = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
    var nMon = TMon.Load(Path.Combine(gen, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
    var ourObjByMonObj = gMon.Values.GroupBy(m => m.Obj).ToDictionary(g => g.Key, g => g.First());
    var gMounts = TMount.Load(Path.Combine(game, "Tcd", "TMount.tcd"), false);
    var ourMountMons = gMounts.Select(m => m.MonId).ToHashSet();
    foreach (var (pid, (icon, mon, speed)) in pets)
    {
        if (!nMon.TryGetValue(mon, out var nm)) { Console.WriteLine($"pet {pid,3}: mon {mon} MISSING in Genesis TMon"); continue; }
        uint obj = nm.Obj;
        string ours;
        if (G.Has(Kind.Obj, obj))
        {
            bool same = G.Get(Kind.Obj, obj)!.Raw.AsSpan().SequenceEqual(N.Get(Kind.Obj, obj)?.Raw ?? []);
            var om = ourObjByMonObj.GetValueOrDefault(obj);
            ours = $"obj ours({(same ? "same" : "DIFF")}) ourMon {om?.Id} {om?.Name} mount={(om != null && ourMountMons.Contains(om.Id))}";
        }
        else ours = $"obj NEW (gen has={N.Has(Kind.Obj, obj)} file={(N.Has(Kind.Obj, obj) ? N.Idx[Kind.Obj].Files[N.Idx[Kind.Obj].ById[obj].File] : "-")})";
        Console.WriteLine($"pet {pid,3} spd {speed,3} icon {icon,5} mon {mon} {nm.Name,-28} obj {obj:X8} {ours}");
    }
}

if (cmd == "ourmounts")
{
    var gMon = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
    foreach (var m in TMount.Load(Path.Combine(game, "Tcd", "TMount.tcd"), false))
        Console.WriteLine($"{m.Id,3} mon {m.MonId} icon {m.Icon} {gMon.GetValueOrDefault(m.MonId)?.Name} obj {gMon.GetValueOrDefault(m.MonId)?.Obj:X8}");
}

if (cmd == "mountmatch")
{
    var tp = File.ReadAllBytes(Path.Combine(gen, "Tcd", "TPet.tcd"));
    var pets = new SortedDictionary<int, ushort>();
    for (int i = 0; i < BitConverter.ToUInt16(tp, 0); i++)
    {
        int o = 2 + 17 * i; ushort key = BitConverter.ToUInt16(tp, o);
        if ((key & 0xFF) == 0) pets[key >> 8] = BitConverter.ToUInt16(tp, o + 15);
    }
    var gMon = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail);
    var nMon = TMon.Load(Path.Combine(gen, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
    var gMountMon = TMount.Load(Path.Combine(game, "Tcd", "TMount.tcd"), false).Select(m => m.MonId).ToHashSet();
    // our objects by content hash
    var byHash = new Dictionary<string, uint>();
    foreach (var id in G.Idx[Kind.Obj].ById.Keys)
        byHash.TryAdd(Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(G.Get(Kind.Obj, id)!.Raw)), id);
    // monster look = obj + equip tail (bytes after obj). Compare the tail bytes from obj onward.
    string Look(MonRec m) => Convert.ToHexString(m.Raw.AsSpan(m.TailOfs + 16, 40));
    foreach (var (pid, mon) in pets.Where(p => p.Key >= 47))
    {
        var nm = nMon[mon]; var raw = N.Get(Kind.Obj, nm.Obj)?.Raw;
        string h = raw == null ? "" : Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(raw));
        uint ourObj = byHash.GetValueOrDefault(h);
        var ourMons = gMon.Where(m => m.Obj == ourObj && ourObj != 0).ToList();
        var lookMatch = ourMons.FirstOrDefault(m => Look(m) == Look(nm));
        Console.WriteLine($"pet {pid,3} {nm.Name,-28} gobj {nm.Obj:X8} -> ourObj {(ourObj == 0 ? "NONE    " : ourObj.ToString("X8"))} sameLook {(lookMatch == null ? "-" : $"{lookMatch.Id} {lookMatch.Name} mount={gMountMon.Contains(lookMatch.Id)}")}  ourMonsOnObj [{string.Join(",", ourMons.Take(4).Select(m => m.Name))}]");
    }
}

if (cmd == "meshmatch")
{
    var tp = File.ReadAllBytes(Path.Combine(gen, "Tcd", "TPet.tcd"));
    var pets = new SortedDictionary<int, ushort>();
    for (int i = 0; i < BitConverter.ToUInt16(tp, 0); i++)
    {
        int o = 2 + 17 * i; ushort key = BitConverter.ToUInt16(tp, o);
        if ((key & 0xFF) == 0) pets[key >> 8] = BitConverter.ToUInt16(tp, o + 15);
    }
    var gMonL = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail); var gMon = gMonL.ToDictionary(m => m.Id);
    var nMon = TMon.Load(Path.Combine(gen, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
    var gMounts = TMount.Load(Path.Combine(game, "Tcd", "TMount.tcd"), false);
    HashSet<string> MeshSet(Client c, uint obj)
    {
        var r = c.Get(Kind.Obj, obj); var s = new HashSet<string>();
        if (r == null) return s;
        foreach (var (k, id) in r.Refs.Where(x => x.Item1 == Kind.Mesh).Distinct())
            if (c.Get(Kind.Mesh, id) is { } m) s.Add(Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(m.Raw)));
        return s;
    }
    var ours = gMounts.Select(m => gMon[m.MonId]).GroupBy(m => m.Obj).Select(g => (obj: g.Key, names: string.Join("/", g.Select(x => x.Name).Distinct().Take(3)), set: MeshSet(G, g.Key))).ToList();
    foreach (var (pid, mon) in pets.Where(p => p.Key >= 47))
    {
        var nm = nMon[mon]; var s = MeshSet(N, nm.Obj);
        var best = ours.Select(o => (o, j: s.Count == 0 ? 0 : (double)s.Intersect(o.set).Count() / s.Union(o.set).Count())).OrderByDescending(x => x.j).First();
        Console.WriteLine($"pet {pid,3} {nm.Name,-28} meshes {s.Count,2} best {best.j:P0} ourObj {best.o.obj:X8} {best.o.names}");
    }
}

if (cmd == "costumematch")
{
    string path = Path.Combine(gen, "Data", "OBJ", "4Genesis.TOB");
    var b = File.ReadAllBytes(path); var starts = Walk.TobStarts(path);
    int f6 = N.Idx[Kind.Obj].Files.IndexOf(@"OBJGenesis.TOB");
    var indexed = N.Idx[Kind.Obj].Entries.Where(e => e.File == f6).Select(e => (long)e.Pos).ToHashSet();
    var orphanIx = Enumerable.Range(0, starts.Count).Where(i => !indexed.Contains(starts[i])).ToList();
    // player objects were re-appended (orphaning the old copies) on every costume update: take the newest 6
    var last6 = orphanIx.Select(i => b[(int)starts[i]..(int)(i + 1 < starts.Count ? starts[i + 1] : b.Length)])
        .Where(r => r.Length > 100_000).TakeLast(6).ToList();
    var nVis = TVisualTcd.Load(Path.Combine(gen, "Tcd", "TItemVisual.tcd")).ToDictionary(v => v.Id);
    var gItems = TItemTcd.Load(Path.Combine(game, "Tcd", "TItem.tcd"));
    var gIds = gItems.Select(i => i.Id).ToHashSet();
    string H(byte[] x) => Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(x));
    var ourMesh = new HashSet<string>(G.Idx[Kind.Mesh].ById.Keys.Select(id => H(G.Get(Kind.Mesh, id)!.Raw)));
    var meshOf = new Dictionary<(uint, uint, uint), List<uint>>();
    foreach (var rec in last6)
        foreach (var k in ObjCloth.Read(rec).Kinds) foreach (var c in k.Cloths) foreach (var m in c.Meshes)
            (meshOf.TryGetValue((k.Id, c.Id, m.Key), out var l) ? l : meshOf[(k.Id, c.Id, m.Key)] = new()).Add(m.MeshId);
    Console.WriteLine($"last6 sizes [{string.Join(",", last6.Select(x => x.Length))}], meshOf keys {meshOf.Count}, sample {string.Join(" ", meshOf.Keys.Take(3))}");
    int dup = 0, fresh = 0, other = 0;
    foreach (var it in TItemTcd.Load(Path.Combine(gen, "Tcd", "TItem.tcd")).Where(i => i.Raw[2] == 17 && !gIds.Contains(i.Id)))
    {
        var name = TItemTcd.Cp949.GetString(it.Raw, it.NameOfs + 1, it.NameLen - 1);
        var v = nVis[it.Visual0].Raw;
        var key = (BitConverter.ToUInt32(v, 10), BitConverter.ToUInt32(v, 14), BitConverter.ToUInt32(v, 18));
        if (key.Item3 == 0) { other++; Console.WriteLine($"  {it.Id} {name}: no mesh key (sfx {BitConverter.ToUInt16(v, 2)}?)"); continue; }
        var meshes = meshOf.GetValueOrDefault(key) ?? new();
        if (meshes.Count == 0) Console.WriteLine($"    key {key} not found");
        int have = meshes.Count(m => N.Get(Kind.Mesh, m) is { } r && ourMesh.Contains(H(r.Raw)));
        bool allOurs = meshes.Count > 0 && have == meshes.Count;
        if (allOurs) dup++; else fresh++;
        Console.WriteLine($"  {it.Id} {name,-40} meshes {meshes.Count} ours {have} {(allOurs ? "DUP" : "NEW")}");
    }
    Console.WriteLine($"costumes: dup {dup}, new {fresh}, no-key {other}");
}

if (cmd == "lookmatch")
{
    string path = Path.Combine(gen, "Data", "OBJ", "4Genesis.TOB");
    var b = File.ReadAllBytes(path); var starts = Walk.TobStarts(path);
    int f6 = N.Idx[Kind.Obj].Files.IndexOf(@"OBJ\4Genesis.TOB");
    var indexed = N.Idx[Kind.Obj].Entries.Where(e => e.File == f6).Select(e => (long)e.Pos).ToHashSet();
    var gen6 = Enumerable.Range(0, starts.Count).Where(i => !indexed.Contains(starts[i]))
        .Select(i => b[(int)starts[i]..(int)(i + 1 < starts.Count ? starts[i + 1] : b.Length)]).Where(r => r.Length > 100_000).TakeLast(6).ToList();
    uint[] players = [0xF56A4, 0xF56A5, 0xF56A6, 0xF56A7, 0xF56A8, 0xF56A9];
    var hc = new Dictionary<(Client, Kind, uint), string>();
    string H(Client c, Kind k, uint id) => hc.TryGetValue((c, k, id), out var h) ? h : hc[(c, k, id)] =
        c.Get(k, id) is { } r ? Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(r.Raw))[..16] : "MISSING";
    string Look(Client c, ObjCloth.MeshEnt m) => H(c, Kind.Mesh, m.MeshId) + ":" + string.Join(",", m.TexIds.Select(t => t == 0 ? "0" : H(c, Kind.Tex, t)));
    // look signature per (clk,cli,key) = sorted looks across the 6 player objects
    Dictionary<(uint, uint, uint), string> Sigs(Client c, IEnumerable<byte[]> objs)
    {
        var d = new Dictionary<(uint, uint, uint), List<string>>();
        foreach (var rec in objs) foreach (var k in ObjCloth.Read(rec).Kinds) foreach (var cl in k.Cloths) foreach (var m in cl.Meshes)
            (d.TryGetValue((k.Id, cl.Id, m.Key), out var l) ? l : d[(k.Id, cl.Id, m.Key)] = new()).Add(Look(c, m));
        return d.ToDictionary(kv => kv.Key, kv => string.Join("|", kv.Value.Order()));
    }
    var gSig = Sigs(G, players.Select(p => G.Get(Kind.Obj, p)!.Raw));
    var nSig = Sigs(N, gen6);
    var bySig = gSig.GroupBy(kv => kv.Value).ToDictionary(g => g.Key, g => g.First().Key);
    var gVis = TVisualTcd.Load(Path.Combine(game, "Tcd", "TItemVisual.tcd"));
    var gItems = TItemTcd.Load(Path.Combine(game, "Tcd", "TItem.tcd"));
    var nVis = TVisualTcd.Load(Path.Combine(gen, "Tcd", "TItemVisual.tcd")).ToDictionary(v => v.Id);
    var gIds = gItems.Select(i => i.Id).ToHashSet();
    string Name(TItemRec i) => TItemTcd.Cp949.GetString(i.Raw, i.NameOfs + 1, i.NameLen - 1);
    (uint, uint, uint) Key(byte[] v) => (BitConverter.ToUInt32(v, 10), BitConverter.ToUInt32(v, 14), BitConverter.ToUInt32(v, 18));
    var gVisById = gVis.ToDictionary(v => v.Id);
    int dup = 0, neu = 0;
    foreach (var it in TItemTcd.Load(Path.Combine(gen, "Tcd", "TItem.tcd")).Where(i => i.Raw[2] == 17 && !gIds.Contains(i.Id)))
    {
        var key = Key(nVis[it.Visual0].Raw);
        string? ourItem = null;
        if (nSig.TryGetValue(key, out var sig) && bySig.TryGetValue(sig, out var gk))
        {
            var oi = gItems.FirstOrDefault(i => i.Raw[2] == 17 && gVisById.TryGetValue(i.Visual0, out var v) && Key(v.Raw) == gk);
            ourItem = oi == null ? $"look {gk} (no item)" : $"{oi.Id} {Name(oi)}";
        }
        if (ourItem != null) dup++; else neu++;
        Console.WriteLine($"{(ourItem != null ? "DUP" : "NEW")} {it.Id} k{it.Raw[3],-3} {Name(it),-42} {(ourItem ?? "")}");
    }
    Console.WriteLine($"costumes: dup {dup}, new {neu}");
}

if (cmd == "wings")
{
    foreach (var (root, ids) in new[] { (game, new ushort[] { 25004, 25005, 25030 }), (gen, new ushort[] { 31500, 31501, 31502, 31503, 31504, 31505 }) })
    {
        var items = TItemTcd.Load(Path.Combine(root, "Tcd", "TItem.tcd")).ToDictionary(i => i.Id);
        var vis = TVisualTcd.Load(Path.Combine(root, "Tcd", "TItemVisual.tcd")).ToDictionary(v => v.Id);
        foreach (var id in ids)
        {
            if (!items.TryGetValue(id, out var it)) continue;
            int t = it.NameOfs + it.NameLen;
            Console.WriteLine($"{Path.GetFileName(root)} {id} {TItemTcd.Cp949.GetString(it.Raw, it.NameOfs + 1, it.NameLen - 1)} useValue {BitConverter.ToUInt16(it.Raw, t)} tail {Convert.ToHexString(it.Raw, t + 2, 101)}");
            Console.WriteLine($"   visual {it.Visual0}: {(vis.TryGetValue(it.Visual0, out var v) ? Convert.ToHexString(v.Raw) : "none")}");
        }
    }
}
