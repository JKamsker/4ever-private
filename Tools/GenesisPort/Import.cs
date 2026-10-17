// GenesisPort `port [--apply]`: imports the 4Genesis mounts + costumes we lack into Game/ (+ a DB migration).
//
//   costumes  every Genesis IT_COSTUME item whose look (mesh + textures on the 6 player objects) we don't have
//             -> TItem rows renumbered from 12082, their TItemVisual rows, the cloth entries spliced into our player
//                objects (Character.TOB rebuilt in place), meshes + textures; colliding mesh keys get fresh keys
//   mounts    the chosen Genesis pets (TPet.tcd) -> TMount rows 124+, TMon templates 31966+, mount items 31412+,
//             every resource reachable from the monsters' objects that we lack
//   icons     Genesis icon indices whose picture differs from ours -> own images Data\Img\Custom\<60200+>.png
//   packs     new records go to Data\*\Genesis.* (+ per-level Skin\N_Genesis.TTX) appended to our Index\*.IDX
//   SQL       Database\migrations\017_genesis_mounts_costumes.sql (TMONSTERCHART / TMOUNTCHART / TITEMCHART)
// Resource ids that exist in both clients with different bytes keep OUR version (logged). One-shot: writes
// Game\Tcd\GenesisPort.applied and refuses to run again (restore Game/ from git to redo).
using System.Text;
using ClassicPort;

public class Import
{
    // Pets chosen after `mountmatch`/`meshmatch`/`variants`: new models + the two new variants (see memory note).
    static readonly ushort[] MountPets = [60, 66, 67, 68, 72, 76, 84, 89, 90, 92, 95, 104, 125];   // 105 = same look as 125 (The Red Rocket x2)
    const ushort FirstCostume = 12082, FirstMount = 124, FirstMountMon = 31966, FirstMountItem = 31412, FirstIcon = 60200;
    const ushort FallbackIcon = 1775;
    const int EsCount = 19;                       // equip WORDs in a TMon record, right after m_dwOBJ
    const uint RiderPivot = 0x5E7C;               // ID_PIVOT_MOUNT
    const string Tag = "Genesis";
    const string Marker = "GenesisPort.applied";

    readonly string game, gen, migration;
    readonly Client G, N;
    readonly TextWriter log;
    public Import(string game, string gen, string migration, Client g, Client n, TextWriter log)
    { this.game = game; this.gen = gen; this.migration = migration; G = g; N = n; this.log = log; }

    string GT(string f) => Path.Combine(game, "Tcd", f);
    string NT(string f) => Path.Combine(gen, "Tcd", f);

    // ---- state built by Run -------------------------------------------------------------------------------
    List<ItemRec> gItems = new(), nItems = new();
    Dictionary<ushort, byte[]> gVis = new(), nVis = new();
    readonly Dictionary<ushort, byte[]> addVis = new();                 // visual id -> row (ours after patching)
    readonly Dictionary<ushort, ushort> visMap = new();                 // genesis visual id -> our visual id
    readonly Dictionary<ushort, ushort> iconMap = new();                // genesis icon index -> our icon index
    readonly Dictionary<ushort, byte[]> pngs = new();                   // new own image id -> PNG
    readonly List<ItemRec> addItems = new();
    readonly HashSet<(Kind, uint)> pull = new();                        // genesis resources to copy
    IconCut? ourIcons, genIcons;
    int keptOurs;

    static ushort U16(byte[] b, int o) => BitConverter.ToUInt16(b, o);
    static uint U32(byte[] b, int o) => BitConverter.ToUInt32(b, o);
    static void Put(byte[] b, int o, ushort v) => BitConverter.GetBytes(v).CopyTo(b, o);
    static void Put(byte[] b, int o, uint v) => BitConverter.GetBytes(v).CopyTo(b, o);
    static int VisOfs(ItemRec it, int k) => it.TailOfs + 63 + 2 * k;

    static Dictionary<ushort, byte[]> LoadVis(string path) =>
        TVisualTcd.Load(path).GroupBy(v => v.Id).ToDictionary(g => g.Key, g => g.First().Raw.ToArray());

    public void Run(bool apply)
    {
        if (File.Exists(GT(Marker))) throw new InvalidOperationException($"already applied ({GT(Marker)}) - restore Game/ from git to re-run");
        gItems = TItem.Load(GT("TItem.tcd")); nItems = TItem.Load(NT("TItem.tcd"));
        gVis = LoadVis(GT("TItemVisual.tcd")); nVis = LoadVis(NT("TItemVisual.tcd"));
        ourIcons = new IconCut(game); genIcons = new IconCut(gen);

        var (playerObjs, costumeItems) = Costumes();
        var (mounts, monsters, mountItems) = Mounts();
        log.WriteLine($"icons: {iconMap.Count(kv => kv.Key == kv.Value)} shared with ours, {pngs.Count} new own images");
        var res = pull.GroupBy(x => x.Item1).Select(g => $"{g.Key} {g.Count()}");
        log.WriteLine($"resources to copy: {string.Join(", ", res)}; kept ours on id clash: {keptOurs}");

        string sql = Sql.Build(costumeItems, mounts, monsters, mountItems);
        if (!apply) { log.WriteLine("dry run - nothing written (use: port --apply)"); return; }

        var before = Snapshot(G);
        WritePacks(playerObjs);
        WriteTables(mounts, monsters);
        string custom = Path.Combine(game, "Data", "Img", "Custom");
        foreach (var (id, png) in pngs) File.WriteAllBytes(Path.Combine(custom, $"{id}.png"), png);
        File.WriteAllText(migration, sql, new UTF8Encoding(false));
        File.WriteAllLines(GT(Marker), addItems.Select(i => $"item={i.Id}").Concat(addVis.Keys.Select(v => $"visual={v}"))
            .Concat(mounts.Select(m => $"mount={m.Id}")).Concat(monsters.Select(m => $"monster={m.Id}")).Concat(pngs.Keys.Select(p => $"png={p}")));
        log.WriteLine($"written: packs + indexes, Character.TOB, TItem +{addItems.Count}, TItemVisual +{addVis.Count}, TMount +{mounts.Count}, TMon +{monsters.Count}, {pngs.Count} PNGs, {Path.GetFileName(migration)}");
        Verify(before, playerObjs);
    }

    // =========================================================================================== costumes
    (Dictionary<uint, byte[]> objs, List<ItemRec> items) Costumes()
    {
        var gp = Genesis.PlayerObjects(N);
        var ours = Genesis.Players.ToDictionary(p => p, p => G.Get(Kind.Obj, p)!.Raw);
        var oc = ours.ToDictionary(kv => kv.Key, kv => ObjCloth.Read(kv.Value));
        var nc = gp.ToDictionary(kv => kv.Key, kv => ObjCloth.Read(kv.Value));

        static IEnumerable<(uint clk, uint cli, ObjCloth.MeshEnt m)> Ents(ObjCloth c) =>
            c.Kinds.SelectMany(k => k.Cloths.SelectMany(cl => cl.Meshes.Select(m => (k.Id, cl.Id, m))));

        // look signature of a (clk, cli, key) across the 6 objects: mesh + texture bytes
        var hash = new Dictionary<(bool, Kind, uint), string>();
        string H(Client c, Kind k, uint id) => hash.TryGetValue((c == G, k, id), out var h) ? h : hash[(c == G, k, id)] =
            c.Get(k, id) is { } r ? Convert.ToHexString(System.Security.Cryptography.SHA1.HashData(r.Raw)) : "-";
        Dictionary<(uint, uint, uint), string> Sigs(Client c, Dictionary<uint, ObjCloth> objs)
        {
            var d = new Dictionary<(uint, uint, uint), List<string>>();
            foreach (var (p, cl) in objs)
                foreach (var (clk, cli, m) in Ents(cl))
                    (d.TryGetValue((clk, cli, m.Key), out var l) ? l : d[(clk, cli, m.Key)] = new())
                        .Add($"{p:X}:{H(c, Kind.Mesh, m.MeshId)}:{string.Join(",", m.TexIds.Select(t => t == 0 ? "0" : H(c, Kind.Tex, t)))}");
            return d.ToDictionary(kv => kv.Key, kv => string.Join("|", kv.Value.Order()));
        }
        var ourSigs = Sigs(G, oc).Values.ToHashSet();
        var genSig = Sigs(N, nc);

        var gIds = gItems.Select(i => i.Id).ToHashSet();
        var picked = new List<ItemRec>();
        foreach (var it in nItems.Where(i => i.Type == 17 && !gIds.Contains(i.Id)).OrderBy(i => i.Id))
        {
            var v = nVis[U16(it.Raw, VisOfs(it, 0))];
            var key = (U32(v, 10), U32(v, 14), U32(v, 18));
            if (genSig.TryGetValue(key, out var sig) && ourSigs.Contains(sig)) { log.WriteLine($"  costume {it.Id} '{it.Name}': same look as one of ours - skipped"); continue; }
            picked.Add(it);
        }
        log.WriteLine($"costumes to add: {picked.Count} -> items {FirstCostume}..{FirstCostume + picked.Count - 1}");

        // mesh keys: a (clk, cli, key) we already use for something else gets a fresh key
        var usedKeys = new HashSet<uint>(oc.Values.Concat(nc.Values).SelectMany(c => Ents(c).Select(e => e.m.Key)));
        foreach (var v in gVis.Values.Concat(nVis.Values)) { usedKeys.Add(U32(v, 18)); usedKeys.Add(U32(v, 22)); }
        uint nextKey = usedKeys.Where(k => k < 0x100000).Max() + 1;
        var ourEnt = new Dictionary<(uint, uint, uint, uint), byte[]>();
        foreach (var (p, cl) in oc) foreach (var (clk, cli, m) in Ents(cl)) ourEnt[(p, clk, cli, m.Key)] = m.Raw;
        var keyMap = new Dictionary<(uint, uint, uint), uint>();
        uint MapKey(uint clk, uint cli, uint key)
        {
            if (key == 0) return 0;
            if (keyMap.TryGetValue((clk, cli, key), out var k)) return k;
            bool clash = Genesis.Players.Any(p => ourEnt.TryGetValue((p, clk, cli, key), out var mine) &&
                !nc[p].Kinds.Where(x => x.Id == clk).SelectMany(x => x.Cloths).Where(x => x.Id == cli).SelectMany(x => x.Meshes)
                    .Any(m => m.Key == key && m.Raw.AsSpan().SequenceEqual(mine)));
            k = clash ? nextKey++ : key;
            if (clash) log.WriteLine($"  mesh key {clk:X}/{cli:X}/{key:X} is ours for another look -> {k:X}");
            return keyMap[(clk, cli, key)] = k;
        }

        // items + visuals (+ keys); collect the cloth entries to splice
        var items = new List<ItemRec>();
        var splice = new List<(uint clk, uint cli, uint key, uint newKey)>();
        for (int i = 0; i < picked.Count; i++)
        {
            var it = picked[i];
            var raw = (byte[])it.Raw.Clone();
            Put(raw, 0, (ushort)(FirstCostume + i));
            for (int k = 0; k < 5; k++)
            {
                ushort v = U16(raw, VisOfs(it, k));
                if (v == 0) continue;
                Put(raw, VisOfs(it, k), MapVisual(v, row =>
                {
                    uint clk = U32(row, 10), cli = U32(row, 14);
                    foreach (int o in new[] { 18, 22 })
                    {
                        uint key = U32(row, o), nk = MapKey(clk, cli, key);
                        if (key != 0) splice.Add((clk, cli, key, nk));
                        Put(row, o, nk);
                    }
                }));
            }
            var rec = Rec(raw);
            items.Add(rec); addItems.Add(rec);
        }

        // splice entries into our 6 player objects; pull their meshes + textures
        var outObjs = new Dictionary<uint, byte[]>();
        var roots = new List<(Kind, uint)>();
        foreach (var p in Genesis.Players)
        {
            var mine = oc[p]; int added = 0;
            foreach (var (clk, cli, key, nk) in splice.Distinct())
                foreach (var m in nc[p].Kinds.Where(x => x.Id == clk).SelectMany(x => x.Cloths).Where(x => x.Id == cli).SelectMany(x => x.Meshes).Where(m => m.Key == key))
                {
                    var gk = mine.Kinds.FirstOrDefault(x => x.Id == clk);
                    if (gk == null) mine.Kinds.Add(gk = new ObjCloth.ClKind { Id = clk });
                    var gcl = gk.Cloths.FirstOrDefault(x => x.Id == cli);
                    if (gcl == null) gk.Cloths.Add(gcl = new ObjCloth.Cloth { Id = cli });
                    var e = new ObjCloth.MeshEnt { Key = nk, MeshId = m.MeshId, TexIds = m.TexIds, Raw = (byte[])m.Raw.Clone() };
                    Put(e.Raw, 0, nk);
                    if (gcl.Meshes.Any(x => x.Key == nk)) { if (!gcl.Meshes.First(x => x.Key == nk).Raw.AsSpan().SequenceEqual(e.Raw)) throw new InvalidDataException($"key {nk:X} still clashes on {p:X}"); continue; }
                    gcl.Meshes.Add(e); added++;
                    roots.Add((Kind.Mesh, m.MeshId)); roots.AddRange(m.TexIds.Where(t => t != 0).Select(t => (Kind.Tex, t)));
                }
            if (added > 0) { outObjs[p] = mine.Write(ours[p]); log.WriteLine($"  player obj {p:X}: +{added} cloth entries"); }
        }

        // effects: a visual's m_dwEffectFuncID[mode] plays the player object's object-level SFX instances tagged with
        // that func (CTClientObjBase::ResetEquip -> GenerateSFX), e.g. the 5 Genesis wings
        var funcs = items.SelectMany(it => Enumerable.Range(0, 5).Select(k => U16(it.Raw, VisOfs(it, k))))
            .Where(v => v != 0).Select(v => addVis.GetValueOrDefault(v) ?? gVis[v])
            .SelectMany(v => new[] { U32(v, 51), U32(v, 55) }).Where(f => f != 0).ToHashSet();
        foreach (var p in Genesis.Players)
        {
            var rec = outObjs.GetValueOrDefault(p) ?? ours[p];
            var mineInst = ObjSfxList(rec).ToList();
            var add = new List<byte[]>();
            uint nextInstKey = mineInst.Select(x => U32(x, 0)).DefaultIfEmpty(0u).Max() + 1;
            foreach (var inst in ObjSfxList(gp[p]).Where(x => funcs.Contains(U32(x, 37))))
            {
                if (mineInst.Any(x => x.AsSpan().SequenceEqual(inst))) continue;
                var ni = (byte[])inst.Clone();
                if (mineInst.Any(x => U32(x, 0) == U32(inst, 0))) Put(ni, 0, nextInstKey++);
                add.Add(ni);
                roots.Add((Kind.Sfx, U32(ni, 4)));
            }
            if (add.Count == 0) continue;
            outObjs[p] = AddObjSfxList(rec, add);
            log.WriteLine($"  player obj {p:X}: +{add.Count} effect SFX (funcs {string.Join(",", add.Select(x => U32(x, 37).ToString("X")))})");
        }
        Pull(roots);
        return (outObjs, items);
    }

    /// <summary>Object-level SFXINST records of a .TOB record: right after the pivots, 49 bytes each
    /// (key, sfx, pos, rot, pivot, bias, func, tick, deleteOnStop - LoadOBJ order).</summary>
    const int SfxInst = 49;
    public static IEnumerable<byte[]> ObjSfxList(byte[] rec)
    {
        int o = 5 + 8 * BitConverter.ToInt32(rec, 1), n = BitConverter.ToInt32(rec, o);
        for (int i = 0; i < n; i++) yield return rec[(o + 4 + SfxInst * i)..(o + 4 + SfxInst * (i + 1))];
    }

    public static byte[] AddObjSfxList(byte[] rec, List<byte[]> add)
    {
        int o = 5 + 8 * BitConverter.ToInt32(rec, 1), n = BitConverter.ToInt32(rec, o), end = o + 4 + SfxInst * n;
        var outp = (byte[])[.. rec[..end], .. add.SelectMany(x => x), .. rec[end..]];
        BitConverter.GetBytes(n + add.Count).CopyTo(outp, o);
        return outp;
    }

    // =========================================================================================== mounts
    public record MountRow(ushort Id, ushort MonId, ushort Icon, string Name)
    {
        public ushort SaddleMonId { get; init; } = MonId;      // saddled recall monster (Genesis mounts: none -> same)
    }

    (List<MountRow> mounts, List<MonRec> monsters, List<ItemRec> items) Mounts()
    {
        var pets = Genesis.LoadPets(NT("TPet.tcd")).Where(p => p.Race == 0).GroupBy(p => p.Id).ToDictionary(g => g.Key, g => g.First());
        var nMon = TMon.Load(NT("TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var gMon = TMon.Load(GT("TMon.tcd"), TMon.OurTail).Select(m => m.Id).ToHashSet();
        var gMounts = TMount.Load(GT("TMount.tcd"), false).Select(m => m.Id).ToHashSet();
        var gItemIds = gItems.Select(i => i.Id).ToHashSet();

        var mounts = new List<MountRow>(); var monsters = new List<MonRec>();
        var monMap = new Dictionary<ushort, ushort>(); var petMap = new Dictionary<ushort, ushort>();
        foreach (var pid in MountPets)
        {
            var pet = pets[pid];
            var src = nMon[pet.MonId];
            if (!monMap.TryGetValue(src.Id, out var newMon))
            {
                newMon = (ushort)(FirstMountMon + monsters.Count);
                if (gMon.Contains(newMon)) throw new InvalidDataException($"monster id {newMon} is taken");
                var raw = (byte[])src.Raw.Clone(); Put(raw, 0, newMon);
                var m = TMon.ParseOne(raw, TMon.OurTail);
                Health(src);
                for (int k = 0; k < EsCount; k++)
                {
                    ushort eq = U16(raw, m.TailOfs + 16 + 2 * k);
                    if (eq != 0 && !gItemIds.Contains(eq) && addItems.All(a => a.Id != eq)) PortEquipItem(eq);
                }
                monsters.Add(m); monMap[src.Id] = newMon;
                Pull([(Kind.Obj, src.Obj)]);
            }
            ushort id = (ushort)(FirstMount + mounts.Count);
            if (gMounts.Contains(id)) throw new InvalidDataException($"mount id {id} is taken");
            mounts.Add(new MountRow(id, newMon, MapIcon(pet.Icon, $"mount {src.Name}"), src.Name));
            petMap[pid] = id;
            log.WriteLine($"  pet {pid,3} '{src.Name}': mount {id}, monster {src.Id} -> {newMon}, object {src.Obj:X8}");
        }

        // the scrolls that make them
        var items = new List<ItemRec>();
        ushort NextItem()
        {
            ushort nid = (ushort)(FirstMountItem + items.Count);
            if (gItemIds.Contains(nid)) throw new InvalidDataException($"item id {nid} is taken");
            return nid;
        }
        string MountName(ushort mount) => mounts.First(m => m.Id == mount).Name;
        foreach (var it in nItems.Where(i => i.Type == 12 && petMap.ContainsKey(i.UseValue)).OrderBy(i => i.Id))
        {
            var raw = (byte[])it.Raw.Clone();
            ushort nid = NextItem(), mount = petMap[it.UseValue];
            Put(raw, 0, nid); Put(raw, it.TailOfs - 2, mount);
            for (int k = 0; k < 5; k++) { ushort v = U16(raw, VisOfs(it, k)); if (v != 0) Put(raw, VisOfs(it, k), MapVisual(v, null)); }
            // Genesis left one scroll named after the mount it was cloned from (30049 "Behemoth Scroll" makes The Red Rocket)
            string name = it.Name.Contains(MountName(mount).Split(' ')[0]) ? it.Name : $"{MountName(mount)} Scroll";
            var rec = Rec(WithName(raw, name)); items.Add(rec); addItems.Add(rec);
            log.WriteLine($"  item {it.Id} '{it.Name}' -> {nid} '{name}' (mount {mount})");
        }

        // pets Genesis only hands out by GM: a scroll cloned from the first ported one, showing the mount's icon
        var template = items.First();
        foreach (var pid in MountPets.Where(p => !items.Any(i => i.UseValue == petMap[p])))
        {
            ushort nid = NextItem(), mount = petMap[pid];
            var raw = (byte[])template.Raw.Clone();
            Put(raw, 0, nid); Put(raw, template.TailOfs - 2, mount);
            var vis = (byte[])addVis.GetValueOrDefault(U16(raw, VisOfs(template, 0)), gVis.GetValueOrDefault(U16(raw, VisOfs(template, 0)))!).Clone();
            ushort vid = (ushort)(Math.Max(gVis.Keys.Max(), Math.Max(nVis.Keys.Max(), addVis.Keys.Max())) + 1);
            Put(vis, 0, vid); Put(vis, 34, mounts.First(m => m.Id == mount).Icon);
            addVis[vid] = vis; Put(raw, VisOfs(template, 0), vid);
            var rec = Rec(WithName(raw, $"{MountName(mount)} Scroll")); items.Add(rec); addItems.Add(rec);
            log.WriteLine($"  item {nid} '{rec.Name}' synthesized (Genesis has no item for pet {pid} -> mount {mount}), visual {vid}");
        }
        return (mounts, monsters, items);
    }

    /// <summary>A Genesis-only look item a monster wears: same id (free in ours), visual mapped.</summary>
    void PortEquipItem(ushort id)
    {
        var it = nItems.First(i => i.Id == id);
        var raw = (byte[])it.Raw.Clone();
        for (int k = 0; k < 5; k++) { ushort v = U16(raw, VisOfs(it, k)); if (v != 0) Put(raw, VisOfs(it, k), MapVisual(v, null)); }
        addItems.Add(Rec(raw));
        log.WriteLine($"  equip item {id} '{it.Name}' ported (worn by a mount monster)");
    }

    void Health(MonRec m)
    {
        if (m.Obj == 0) throw new InvalidDataException($"monster {m.Id} '{m.Name}' has no object");
        var rec = Packs.Stored(N, Kind.Obj, m.Obj);
        int n = BitConverter.ToInt32(rec, 1);
        bool rider = Enumerable.Range(0, n).Any(i => U32(rec, 5 + 8 * i + 4) == RiderPivot);
        if (!rider) log.WriteLine($"  warn: monster {m.Id} '{m.Name}' object {m.Obj:X8} has no rider pivot");
    }

    // =========================================================================================== shared
    static byte[] WithName(byte[] raw, string name)
    {
        var b = TMon.Cp949.GetBytes(name);
        if (b.Length >= 0xFF || raw[6] == 0xFF) throw new ArgumentException(name);
        return [.. raw[..6], (byte)b.Length, .. b, .. raw[(7 + raw[6])..]];
    }

    /// <summary>One bare TItem record (InitTITEMTEMP order; names are short, 1-byte CString length).</summary>
    static ItemRec Rec(byte[] raw)
    {
        int l = raw[6];
        if (l == 0xFF) throw new InvalidDataException("long item name");
        return new ItemRec
        {
            Id = U16(raw, 0), Type = raw[2], Kind = raw[3], Name = TMon.Cp949.GetString(raw, 7, l),
            UseValue = U16(raw, 7 + l), TailOfs = 9 + l, Raw = raw,
        };
    }

    /// <summary>Our visual id for a Genesis visual: same id when free (or identical), else a fresh id.
    /// patch (if any) edits the row before it is compared/added (mesh keys).</summary>
    ushort MapVisual(ushort v, Action<byte[]>? patch)
    {
        if (visMap.TryGetValue(v, out var m)) return m;
        var row = (byte[])nVis[v].Clone();
        patch?.Invoke(row);
        Put(row, 34, MapIcon(U16(row, 34), $"visual {v}"));
        ushort id = v;
        if (gVis.TryGetValue(v, out var mine))
        {
            if (mine.AsSpan().SequenceEqual(row)) return visMap[v] = v;
            id = (ushort)(Math.Max(gVis.Keys.Max(), Math.Max(nVis.Keys.Max(), addVis.Count == 0 ? (ushort)0 : addVis.Keys.Max())) + 1);
            log.WriteLine($"  visual {v} is ours with other content -> {id}");
        }
        Put(row, 0, id);
        addVis[id] = row;
        return visMap[v] = id;
    }

    /// <summary>Our icon index for a Genesis icon index: same index when the picture is identical, else an own image.</summary>
    ushort MapIcon(ushort icon, string what)
    {
        if (iconMap.TryGetValue(icon, out var m)) return m;
        if (icon >= 60000) { log.WriteLine($"  warn: {what}: Genesis own image {icon} -> fallback {FallbackIcon}"); return iconMap[icon] = FallbackIcon; }
        var g = genIcons!.Cut(icon);
        if (g == null) { log.WriteLine($"  warn: {what}: Genesis icon {icon} has no picture -> fallback {FallbackIcon}"); return iconMap[icon] = FallbackIcon; }
        var o = ourIcons!.Cut(icon);
        if (o is { } x && x.w == g.Value.w && x.h == g.Value.h && x.rgba.AsSpan().SequenceEqual(g.Value.rgba)) return iconMap[icon] = icon;
        ushort id = (ushort)(FirstIcon + pngs.Count);
        pngs[id] = Png.Encode(g.Value.w, g.Value.h, g.Value.rgba);
        return iconMap[icon] = id;
    }

    void Pull(IEnumerable<(Kind, uint)> roots)
    {
        var c = new Closure(G, N); c.Walk(roots);
        if (c.Missing.Count > 0) throw new InvalidDataException($"Genesis resources missing: {string.Join(",", c.Missing.Select(x => $"{x.Item1}:{x.Item2:X8}"))}");
        foreach (var x in c.Collide.Where(x => !pull.Contains(x)))
        {
            log.WriteLine($"  {x.Item1} {x.Item2:X8} exists in ours with other bytes -> keeping ours");
            keptOurs++;
        }
        foreach (var x in c.New) pull.Add(x);
    }

    // =========================================================================================== writing
    static readonly Dictionary<Kind, (string pack, string idx)> Out = new()
    {
        [Kind.Obj] = (@"OBJ\Genesis.TOB", "TClientO.IDX"),
        [Kind.Mesh] = (@"Mesh\Genesis.TMH", "TClientM.IDX"),
        [Kind.Ani] = (@"Action\Genesis.TAC", "TClientA.IDX"),
        [Kind.Sfx] = (@"SFX\Genesis.TFX", "TClientX.IDX"),
    };

    void WritePacks(Dictionary<uint, byte[]> playerObjs)
    {
        string D(string rel) => Path.Combine(game, "Data", rel);
        string I(string name) => Path.Combine(game, "Index", name);

        // Character.TOB: player objects replaced in place, everything after them shifted
        var oIdx = G.Idx[Kind.Obj];
        if (playerObjs.Count > 0)
        {
            int f = oIdx.Files.FindIndex(x => x.Equals(@"OBJ\Character.TOB", StringComparison.OrdinalIgnoreCase));
            byte[] tob = File.ReadAllBytes(D(@"OBJ\Character.TOB"));
            var idAt = oIdx.Entries.Where(e => e.File == f).GroupBy(e => e.Pos).ToDictionary(g => g.Key, g => g.First().Id);
            var outp = new MemoryStream(); var move = new Dictionary<uint, uint>();
            for (int p = 0; p < tob.Length;)
            {
                int len = Packs.ObjLength(tob, p);
                move[(uint)p] = (uint)outp.Position;
                if (idAt.TryGetValue((uint)p, out var id) && playerObjs.TryGetValue(id, out var rec)) outp.Write(rec);
                else outp.Write(tob, p, len);
                p += len;
            }
            oIdx.Entries = oIdx.Entries.Select(e => e.File == f ? e with { Pos = move[e.Pos] } : e).ToList();
            File.WriteAllBytes(D(@"OBJ\Character.TOB"), outp.ToArray());
        }

        // sequential packs + SFX
        foreach (var (k, (pack, idxName)) in Out)
        {
            var ids = pull.Where(x => x.Item1 == k).Select(x => x.Item2).OrderBy(x => x).ToList();
            if (ids.Count == 0) { if (k == Kind.Obj && playerObjs.Count > 0) oIdx.Save(I(idxName)); continue; }
            var idx = G.Idx[k];
            int fi = idx.Files.Count; idx.Files.Add(pack);
            using (var fs = File.Create(D(pack)))
                foreach (var id in ids) { idx.Entries.Add(new IdxEntry(id, fi, (uint)fs.Position)); fs.Write(Packs.Stored(N, k, id)); }
            idx.Save(I(idxName));
        }

        // textures: per detail level, from Genesis' own level packs
        var tex = pull.Where(x => x.Item1 == Kind.Tex).Select(x => x.Item2).OrderBy(x => x).ToList();
        if (tex.Count > 0)
            for (int lvl = 0; lvl < 3; lvl++)
            {
                var gi = Idx.Load(I($"{lvl}_TClientS.IDX")); var ni = Idx.Load(Path.Combine(gen, "Index", $"{lvl}_TClientS.IDX"));
                string pack = $@"Skin\{lvl}_{Tag}.TTX";
                int fi = gi.Files.Count; gi.Files.Add(pack);
                var srcFiles = new Dictionary<string, byte[]>();
                using (var fs = File.Create(D(pack)))
                    foreach (var t in tex)
                    {
                        var e = ni.ById[t]; string sf = ni.Files[e.File];
                        if (!srcFiles.TryGetValue(sf, out var b)) srcFiles[sf] = b = File.ReadAllBytes(Path.Combine(gen, "Data", sf));
                        int len = 8 + BitConverter.ToInt32(b, (int)e.Pos + 4);
                        gi.Entries.Add(new IdxEntry(t, fi, (uint)fs.Position)); fs.Write(b, (int)e.Pos, len);
                    }
                gi.Save(I($"{lvl}_TClientS.IDX"));
            }

        // media: whole source files copied under a Genesis_ name
        var media = pull.Where(x => x.Item1 == Kind.Media).Select(x => x.Item2).ToList();
        if (media.Count > 0)
        {
            var idx = G.Idx[Kind.Media]; var nidx = N.Idx[Kind.Media];
            var fileMap = new Dictionary<int, int>();
            foreach (var id in media)
            {
                var e = nidx.ById[id];
                if (!fileMap.TryGetValue(e.File, out var fi))
                {
                    string src = nidx.Files[e.File], rel = Path.Combine(Path.GetDirectoryName(src)!, Tag + "_" + Path.GetFileName(src));
                    File.Copy(Path.Combine(gen, "Data", src), D(rel), true);
                    fileMap[e.File] = fi = idx.Files.Count; idx.Files.Add(rel);
                }
                idx.Entries.Add(new IdxEntry(id, fi, e.Pos));
            }
            idx.Save(I("TClientW.IDX"));
        }
    }

    void WriteTables(List<MountRow> mounts, List<MonRec> monsters)
    {
        TItem.Save(GT("TItem.tcd"), gItems.Concat(addItems).OrderBy(i => i.Id));
        TVisualTcd.Save(GT("TItemVisual.tcd"), gVis.Concat(addVis).OrderBy(kv => kv.Key).Select(kv => kv.Value));
        var gMon = TMon.Load(GT("TMon.tcd"), TMon.OurTail);
        TMon.Save(GT("TMon.tcd"), gMon.Concat(monsters).OrderBy(m => m.Id));
        var gMount = TMount.Load(GT("TMount.tcd"), false);
        TMount.Save(GT("TMount.tcd"), gMount.Concat(mounts.Select(m => new Mount(m.Id, m.MonId, m.MonId, m.Icon, 1f))).OrderBy(m => m.Id), false);
    }

    // =========================================================================================== verify
    static Dictionary<string, byte[]> Snapshot(Client c)
    {
        var s = new Dictionary<string, byte[]>();
        foreach (var k in new[] { Kind.Obj, Kind.Mesh, Kind.Ani, Kind.Sfx, Kind.Tex })
            foreach (var id in c.Idx[k].ById.Keys) s[$"{k}:{id:X8}"] = c.Get(k, id)!.Raw;
        return s;
    }

    void Verify(Dictionary<string, byte[]> before, Dictionary<uint, byte[]> playerObjs)
    {
        var after = Snapshot(new Client(game, "2_TClientS.IDX", false));
        var expect = new Dictionary<string, byte[]>(before);
        foreach (var (id, rec) in playerObjs) expect[$"Obj:{id:X8}"] = rec;
        foreach (var (k, id) in pull.Where(x => x.Item1 != Kind.Media))
            expect[$"{k}:{id:X8}"] = k is Kind.Mesh or Kind.Ani ? Packs.Stored(N, k, id) : N.Get(k, id)!.Raw;
        int bad = expect.Count(kv => !after.TryGetValue(kv.Key, out var a) || !a.AsSpan().SequenceEqual(kv.Value)) + Math.Abs(after.Count - expect.Count);
        log.WriteLine($"read-back: {after.Count} records, {bad} unexpected");
        foreach (var (k, (pack, _)) in Out)
            if (File.Exists(Path.Combine(game, "Data", pack)) && k != Kind.Sfx)
            {
                var (n, rest) = Packs.WalkFile(k, Path.Combine(game, "Data", pack));
                log.WriteLine($"  {pack}: {n} records, {rest} trailing bytes");
                if (rest != 0) bad++;
            }
        if (bad != 0) throw new InvalidDataException("read-back mismatch - restore Game/ from git");
    }
}
