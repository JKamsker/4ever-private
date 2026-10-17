// GenesisPort `rev-port [--apply]`: imports 4Revelation content we lack into Game/ (+ DB migration + client table).
// Source = 4Revelation/ (decrypted mirror, our formats, built by `rev-decode`).
//
//   costumes  IT_COSTUME hats/outfits (kinds 88/89) whose look we lack -> items 12215+, visuals, cloth entries spliced
//             into our 6 player objects (colliding mesh keys remapped), meshes + textures
//   effects   wings / chakras / auras (kind 250, kind 122 -> 250): same costume pipeline (the wings also have a mesh)
//             + the Revelation-only TSFX rows and their SFX; auras whose effect Revelation hardcodes by item id
//             (TClient.exe 0x140159E1D) -> Client\TClient\TCostumeSFXTable.inl (raw SFX / TSFX + tint per item)
//   mounts    Revelation mounts 45+ whose (object, equip look) none of our mounts has -> TMount 137+, TMon 31979+,
//             scrolls 31425+, every resource their objects need
//   icons     Revelation icons that differ from ours -> own images Data\Img\Custom\<60231+>.png
//   packs     Data\*\Revelation.* (+ Skin\N_Revelation.TTX, the same texture for the 3 detail levels)
//   SQL       Database\migrations\019_revelation_port.sql
// Resource ids existing in both clients with different bytes keep OUR version. One-shot (Game\Tcd\RevelationPort.applied).
using System.Text;
using ClassicPort;

public class RevImport
{
    const ushort FirstCostume = 12215, FirstMount = 137, FirstMountMon = 31979, FirstMountItem = 31425, FirstIcon = 60231;
    const ushort FallbackIcon = 1775, FirstRevMount = 45;
    const int EsCount = 19;
    const string Tag = "Revelation", Marker = "RevelationPort.applied";
    static readonly ushort[] EffectKinds = [250, 122];

    readonly string game, rev, migration, auraTable;
    readonly Client G, R;
    readonly TextWriter log;
    public RevImport(string game, string rev, string migration, string auraTable, Client g, Client r, TextWriter log)
    { this.game = game; this.rev = rev; this.migration = migration; this.auraTable = auraTable; G = g; R = r; this.log = log; }

    string GT(string f) => Path.Combine(game, "Tcd", f);
    string RT(string f) => Path.Combine(rev, "Tcd", f);

    List<ItemRec> gItems = new(), rItems = new();
    Dictionary<ushort, byte[]> gVis = new(), rVis = new();
    readonly Dictionary<ushort, byte[]> addVis = new();
    readonly Dictionary<ushort, ushort> visMap = new(), iconMap = new();
    readonly Dictionary<ushort, byte[]> pngs = new();
    readonly List<ItemRec> addItems = new();
    readonly HashSet<(Kind, uint)> pull = new();
    readonly List<(uint id, uint sfx, uint pos)> addTsfx = new();
    IconCut? ourIcons, revIcons;
    int keptOurs;

    static ushort U16(byte[] b, int o) => BitConverter.ToUInt16(b, o);
    static uint U32(byte[] b, int o) => BitConverter.ToUInt32(b, o);
    static void Put(byte[] b, int o, ushort v) => BitConverter.GetBytes(v).CopyTo(b, o);
    static void Put(byte[] b, int o, uint v) => BitConverter.GetBytes(v).CopyTo(b, o);
    static int VisOfs(ItemRec it, int k) => it.TailOfs + 63 + 2 * k;
    static Dictionary<ushort, byte[]> LoadVis(string p) => TVisualTcd.Load(p).GroupBy(v => v.Id).ToDictionary(g => g.Key, g => g.First().Raw.ToArray());

    static Dictionary<uint, (uint sfx, uint pos)> Tsfx(string path)
    {
        var b = File.ReadAllBytes(path); int n = BitConverter.ToUInt16(b, 0);
        return Enumerable.Range(0, n).ToDictionary(i => U32(b, 2 + 12 * i), i => (U32(b, 6 + 12 * i), U32(b, 10 + 12 * i)));
    }

    // Revelation's hardcoded costume effects (decoded from its ApplyCharacterSFX switch): item -> effects to play.
    // Raw = PlayRawSFX(sfx resource); else PlayTActionSFX(TSFX id or raw id, tint). Revelation item ids.
    record Fx(uint Id, bool Raw, uint Color);
    static readonly Dictionary<ushort, Fx[]> RevAura = new()
    {
        [31744] = [new(0x110B9300, true, 0)],                     // Blue Aura
        [31745] = [new(0x110B93D3, true, 0)],                     // Dark Aura
        [31746] = [new(0x1128B26B, true, 0)],                     // Sacred Aura
        [31747] = [new(1024, false, 0xFF6E1E91)],                 // Enhanced Pink Chakra (TSFX 1024, tinted)
        [31748] = [new(0x110B9355, true, 0x25FFFFFF)],            // Pure Aura
        [31749] = [new(0x110B9355, true, 0x80006400)],            // Forest Aura
        [31750] = [new(0x110B9355, true, 0x4040E0D0)],            // Sea Aura
        [31751] = [new(0x110B9355, true, 0x50FFFF00)],            // Golden Aura
        [31752] = [new(0x3B8F2594, true, 0xF04B0082)],            // Redolent Aura
        [31753] = [new(0x0023551C, true, 0)],                     // Bubble Aura
        [31754] = [new(0x3B8F256F, true, 0)],                     // Voracious Aura
        [31755] = [new(0x3B8F25B1, true, 0)],                     // Zealous Aura
        [31756] = [new(0x3B8F258E, true, 0)],                     // Arcana Aura
        [31757] = [new(0x3B8F256E, true, 0xFFFF00FF)],            // Nefarious Aura
        [31760] = [new(1024, false, 0xFFEEA5CD)],                 // Ultra Instinct Aura (TSFX 1024, tinted)
        [31804] = [new(1054, false, 0x100074FF), new(1082, false, 0x100074FF), new(1015, false, 1)],   // Bloody Set
        [31805] = [new(1054, false, 0), new(1082, false, 0), new(1015, false, 0)],                    // Demon Set
    };

    public void Run(bool apply)
    {
        if (File.Exists(GT(Marker))) throw new InvalidOperationException($"already applied ({GT(Marker)}) - restore Game/ from git to re-run");
        gItems = TItem.Load(GT("TItem.tcd")); rItems = TItem.Load(RT("TItem.tcd"));
        gVis = LoadVis(GT("TItemVisual.tcd")); rVis = LoadVis(RT("TItemVisual.tcd"));
        ourIcons = new IconCut(game); revIcons = new IconCut(rev);

        var (playerObjs, costumeItems, auraItems) = Costumes();
        var (mounts, monsters, mountItems) = Mounts();
        log.WriteLine($"icons: {iconMap.Count(kv => kv.Key == kv.Value)} shared with ours, {pngs.Count} new own images");
        log.WriteLine($"resources to copy: {string.Join(", ", pull.GroupBy(x => x.Item1).Select(g => $"{g.Key} {g.Count()}"))}; kept ours on id clash: {keptOurs}");
        string sql = Sql.Build(costumeItems, mounts, monsters, mountItems, "019_revelation_port", "4Revelation", "Tools/GenesisPort `rev-port --apply`");
        string table = AuraTable(auraItems);
        if (!apply) { log.WriteLine("dry run - nothing written (use: rev-port --apply)"); return; }

        var before = Snapshot(G);
        WritePacks(playerObjs);
        WriteTables(mounts, monsters);
        foreach (var (id, png) in pngs) File.WriteAllBytes(Path.Combine(game, "Data", "Img", "Custom", $"{id}.png"), png);
        File.WriteAllText(migration, sql, new UTF8Encoding(false));
        File.WriteAllText(auraTable, table, new UTF8Encoding(true));
        File.WriteAllLines(GT(Marker), addItems.Select(i => $"item={i.Id}").Concat(addVis.Keys.Select(v => $"visual={v}"))
            .Concat(mounts.Select(m => $"mount={m.Id}")).Concat(monsters.Select(m => $"monster={m.Id}")).Concat(addTsfx.Select(t => $"tsfx={t.id}")).Concat(pngs.Keys.Select(p => $"png={p}")));
        log.WriteLine($"written: packs, Character.TOB, TItem +{addItems.Count}, TItemVisual +{addVis.Count}, TSFX +{addTsfx.Count}, TMount +{mounts.Count}, TMon +{monsters.Count}, {pngs.Count} PNGs, {Path.GetFileName(migration)}, {Path.GetFileName(auraTable)}");
        Verify(before, playerObjs);
    }

    // =========================================================================================== costumes + effects
    (Dictionary<uint, byte[]>, List<ItemRec>, List<(ushort ourId, ushort revId)>) Costumes()
    {
        var rp = Genesis.Players.ToDictionary(p => p, p => R.Get(Kind.Obj, p)!.Raw);
        var ours = Genesis.Players.ToDictionary(p => p, p => G.Get(Kind.Obj, p)!.Raw);
        var oc = ours.ToDictionary(kv => kv.Key, kv => ObjCloth.Read(kv.Value));
        var rc = rp.ToDictionary(kv => kv.Key, kv => ObjCloth.Read(kv.Value));
        static IEnumerable<(uint clk, uint cli, ObjCloth.MeshEnt m)> Ents(ObjCloth c) =>
            c.Kinds.SelectMany(k => k.Cloths.SelectMany(cl => cl.Meshes.Select(m => (k.Id, cl.Id, m))));

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
        var ourSigs = Sigs(G, oc).Values.ToHashSet(); var revSig = Sigs(R, rc);

        var gIds = gItems.Select(i => i.Id).ToHashSet();
        var picked = new List<ItemRec>();
        foreach (var it in rItems.Where(i => i.Type == 17 && !gIds.Contains(i.Id)).OrderBy(i => EffectKinds.Contains(i.Kind) ? 1 : 0).ThenBy(i => i.Id))
        {
            bool effect = EffectKinds.Contains(it.Kind);
            if (!effect && it.Kind != 88 && it.Kind != 89) { log.WriteLine($"  costume {it.Id} '{it.Name}' kind {it.Kind}: not ported (race swap)"); continue; }
            if (!rVis.TryGetValue(U16(it.Raw, VisOfs(it, 0)), out var v)) { log.WriteLine($"  costume {it.Id} '{it.Name}': no visual - skipped"); continue; }
            var key = (U32(v, 10), U32(v, 14), U32(v, 18));
            if (!effect)
            {
                if (key.Item3 == 0 || !revSig.TryGetValue(key, out var sig)) { log.WriteLine($"  costume {it.Id} '{it.Name}': no mesh in Revelation's player objects - skipped"); continue; }
                if (ourSigs.Contains(sig)) { log.WriteLine($"  costume {it.Id} '{it.Name}': same look as one of ours - skipped"); continue; }
            }
            else if (!RevAura.ContainsKey(it.Id) && it.UseValue <= 1) { log.WriteLine($"  effect {it.Id} '{it.Name}': no known effect - skipped"); continue; }
            picked.Add(it);
        }
        log.WriteLine($"costumes + effects to add: {picked.Count} -> items {FirstCostume}..{FirstCostume + picked.Count - 1}");

        // mesh keys: a (clk, cli, key) we use for another look gets a fresh key
        var usedKeys = new HashSet<uint>(oc.Values.Concat(rc.Values).SelectMany(c => Ents(c).Select(e => e.m.Key)));
        foreach (var vv in gVis.Values.Concat(rVis.Values)) { usedKeys.Add(U32(vv, 18)); usedKeys.Add(U32(vv, 22)); }
        uint nextKey = usedKeys.Where(k => k < 0x100000).Max() + 1;
        var ourEnt = new Dictionary<(uint, uint, uint, uint), byte[]>();
        foreach (var (p, cl) in oc) foreach (var (clk, cli, m) in Ents(cl)) ourEnt[(p, clk, cli, m.Key)] = m.Raw;
        var keyMap = new Dictionary<(uint, uint, uint), uint>();
        uint MapKey(uint clk, uint cli, uint key)
        {
            if (key == 0) return 0;
            if (keyMap.TryGetValue((clk, cli, key), out var k)) return k;
            bool clash = Genesis.Players.Any(p => ourEnt.TryGetValue((p, clk, cli, key), out var mine) &&
                !rc[p].Kinds.Where(x => x.Id == clk).SelectMany(x => x.Cloths).Where(x => x.Id == cli).SelectMany(x => x.Meshes)
                    .Any(m => m.Key == key && m.Raw.AsSpan().SequenceEqual(mine)));
            k = clash ? nextKey++ : key;
            if (clash) log.WriteLine($"  mesh key {clk:X}/{cli:X}/{key:X} is ours for another look -> {k:X}");
            return keyMap[(clk, cli, key)] = k;
        }

        var items = new List<ItemRec>(); var auras = new List<(ushort, ushort)>();
        var splice = new List<(uint clk, uint cli, uint key, uint newKey)>();
        for (int i = 0; i < picked.Count; i++)
        {
            var it = picked[i]; var raw = (byte[])it.Raw.Clone();
            ushort nid = (ushort)(FirstCostume + i);
            if (gIds.Contains(nid)) throw new InvalidDataException($"item id {nid} is taken");
            Put(raw, 0, nid);
            if (it.Kind == 122) raw[3] = 250;                                   // our client knows the effect costume as kind 250
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
            var rec = Rec(raw); items.Add(rec); addItems.Add(rec);
            if (RevAura.ContainsKey(it.Id)) auras.Add((nid, it.Id));
        }

        var outObjs = new Dictionary<uint, byte[]>(); var roots = new List<(Kind, uint)>();
        foreach (var p in Genesis.Players)
        {
            var mine = oc[p]; int added = 0;
            foreach (var (clk, cli, key, nk) in splice.Distinct())
                foreach (var m in rc[p].Kinds.Where(x => x.Id == clk).SelectMany(x => x.Cloths).Where(x => x.Id == cli).SelectMany(x => x.Meshes).Where(m => m.Key == key))
                {
                    var gk = mine.Kinds.FirstOrDefault(x => x.Id == clk);
                    if (gk == null) mine.Kinds.Add(gk = new ObjCloth.ClKind { Id = clk });
                    var gcl = gk.Cloths.FirstOrDefault(x => x.Id == cli);
                    if (gcl == null) gk.Cloths.Add(gcl = new ObjCloth.Cloth { Id = cli });
                    var e = new ObjCloth.MeshEnt { Key = nk, MeshId = m.MeshId, TexIds = m.TexIds, Raw = (byte[])m.Raw.Clone() };
                    Put(e.Raw, 0, nk);
                    if (gcl.Meshes.Any(x => x.Key == nk)) continue;
                    gcl.Meshes.Add(e); added++;
                    roots.Add((Kind.Mesh, m.MeshId)); roots.AddRange(m.TexIds.Where(t => t != 0).Select(t => (Kind.Tex, t)));
                }
            if (added > 0) { outObjs[p] = mine.Write(ours[p]); log.WriteLine($"  player obj {p:X}: +{added} cloth entries"); }
        }

        // TSFX rows Revelation has and we lack (wings, chakras, set effects) + their SFX
        var gT = Tsfx(GT("TSFX.tcd")); var rT = Tsfx(RT("TSFX.tcd"));
        foreach (var (id, (sfx, pos)) in rT.Where(kv => !gT.ContainsKey(kv.Key)).OrderBy(kv => kv.Key))
        {
            if (!R.Has(Kind.Sfx, sfx) && !G.Has(Kind.Sfx, sfx)) { log.WriteLine($"  TSFX {id}: sfx {sfx:X8} missing in Revelation too - skipped"); continue; }
            addTsfx.Add((id, sfx, pos)); roots.Add((Kind.Sfx, sfx));
        }
        foreach (var fx in RevAura.Values.SelectMany(x => x).Where(f => f.Raw)) roots.Add((Kind.Sfx, fx.Id));

        // effect anchors: object-level SFX instances whose func is a ported visual's effect func or a TSFX position
        var funcs = items.SelectMany(it => Enumerable.Range(0, 5).Select(k => U16(it.Raw, VisOfs(it, k))))
            .Where(v => v != 0).Select(v => addVis.GetValueOrDefault(v) ?? gVis[v])
            .SelectMany(v => new[] { U32(v, 51), U32(v, 55) }).Concat(addTsfx.Select(t => t.pos)).Where(f => f != 0).ToHashSet();
        foreach (var p in Genesis.Players)
        {
            var rec = outObjs.GetValueOrDefault(p) ?? ours[p];
            var mineInst = Import.ObjSfxList(rec).ToList();
            var have = mineInst.Select(x => U32(x, 37)).ToHashSet();
            var add = new List<byte[]>();
            uint nextInst = mineInst.Select(x => U32(x, 0)).DefaultIfEmpty(0u).Max() + 1;
            foreach (var inst in Import.ObjSfxList(rp[p]).Where(x => funcs.Contains(U32(x, 37)) && !have.Contains(U32(x, 37))))
            {
                var ni = (byte[])inst.Clone();
                if (mineInst.Any(x => U32(x, 0) == U32(inst, 0)) || add.Any(x => U32(x, 0) == U32(inst, 0))) Put(ni, 0, nextInst++);
                add.Add(ni); roots.Add((Kind.Sfx, U32(ni, 4)));
            }
            if (add.Count == 0) continue;
            outObjs[p] = Import.AddObjSfxList(rec, add);
            log.WriteLine($"  player obj {p:X}: +{add.Count} effect anchors (funcs {string.Join(",", add.Select(x => U32(x, 37)))})");
        }
        Pull(roots);
        return (outObjs, items, auras);
    }

    // =========================================================================================== mounts
    (List<Import.MountRow>, List<MonRec>, List<ItemRec>) Mounts()
    {
        var rMon = TMon.Load(RT("TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var gMonL = TMon.Load(GT("TMon.tcd"), TMon.OurTail); var gMon = gMonL.ToDictionary(m => m.Id);
        var gMount = TMount.Load(GT("TMount.tcd"), false);
        var gItemIds = gItems.Select(i => i.Id).ToHashSet();
        string Look(MonRec m) => $"{m.Obj:X8}:{Convert.ToHexString(m.Raw, m.TailOfs + 16, 2 * EsCount)}";
        var ourLooks = gMount.SelectMany(m => new[] { m.MonId, m.SaddleMonId }).Where(gMon.ContainsKey).Select(id => Look(gMon[id])).ToHashSet();

        var mounts = new List<Import.MountRow>(); var monsters = new List<MonRec>();
        var monMap = new Dictionary<ushort, ushort>(); var mountMap = new Dictionary<ushort, ushort>();
        var seenLooks = new HashSet<string>();
        ushort NewMon(MonRec src)
        {
            if (monMap.TryGetValue(src.Id, out var id)) return id;
            id = (ushort)(FirstMountMon + monsters.Count);
            if (gMon.ContainsKey(id)) throw new InvalidDataException($"monster id {id} is taken");
            var raw = (byte[])src.Raw.Clone(); Put(raw, 0, id);
            var m = TMon.ParseOne(raw, TMon.OurTail);
            for (int k = 0; k < EsCount; k++)
            {
                ushort eq = U16(raw, m.TailOfs + 16 + 2 * k);
                if (eq != 0 && !gItemIds.Contains(eq) && addItems.All(a => a.Id != eq)) PortEquipItem(eq);
            }
            monsters.Add(m); Pull([(Kind.Obj, src.Obj)]);
            return monMap[src.Id] = id;
        }
        foreach (var rm in TMount.Load(RT("TMount.tcd"), false).Where(m => m.Id >= FirstRevMount))
        {
            var mon = rMon[rm.MonId];
            if (mon.Obj == 0) { log.WriteLine($"  rev mount {rm.Id} '{mon.Name}': no object - skipped"); continue; }
            string look = Look(mon);
            if (ourLooks.Contains(look)) { log.WriteLine($"  rev mount {rm.Id} '{mon.Name}': one of our mounts looks the same - skipped"); continue; }
            if (!seenLooks.Add(look)) { log.WriteLine($"  rev mount {rm.Id} '{mon.Name}': same look as another Revelation mount - skipped"); continue; }
            ushort id = (ushort)(FirstMount + mounts.Count);
            if (gMount.Any(m => m.Id == id)) throw new InvalidDataException($"mount id {id} is taken");
            ushort monId = NewMon(mon), saddle = rm.SaddleMonId == rm.MonId || !rMon.ContainsKey(rm.SaddleMonId) ? monId : NewMon(rMon[rm.SaddleMonId]);
            mounts.Add(new Import.MountRow(id, monId, MapIcon(rm.Icon, $"mount {mon.Name}"), mon.Name) { SaddleMonId = saddle });
            mountMap[rm.Id] = id;
            log.WriteLine($"  rev mount {rm.Id,3} '{mon.Name}': mount {id}, monster {mon.Id} -> {monId}{(saddle != monId ? $" (saddled {saddle})" : "")}, object {mon.Obj:X8}");
        }

        var items = new List<ItemRec>();
        foreach (var it in rItems.Where(i => i.Type == 12 && mountMap.ContainsKey(i.UseValue)).OrderBy(i => i.Id))
        {
            var raw = (byte[])it.Raw.Clone();
            ushort nid = (ushort)(FirstMountItem + items.Count);
            if (gItemIds.Contains(nid)) throw new InvalidDataException($"item id {nid} is taken");
            Put(raw, 0, nid); Put(raw, it.TailOfs - 2, mountMap[it.UseValue]);
            for (int k = 0; k < 5; k++) { ushort v = U16(raw, VisOfs(it, k)); if (v != 0) Put(raw, VisOfs(it, k), MapVisual(v, null)); }
            var rec = Rec(raw); items.Add(rec); addItems.Add(rec);
        }
        foreach (var (rid, id) in mountMap.Where(kv => !items.Any(i => i.UseValue == kv.Value)))
            log.WriteLine($"  warn: rev mount {rid} has no item (mount {id} only obtainable by GM)");
        log.WriteLine($"mounts to add: {mounts.Count}, monsters {monsters.Count}, mount items {items.Count}");
        return (mounts, monsters, items);
    }

    void PortEquipItem(ushort id)
    {
        var it = rItems.FirstOrDefault(i => i.Id == id);
        if (it == null) { log.WriteLine($"  warn: equip item {id} is not defined in Revelation either - left as is (ignored by the client)"); return; }
        var raw = (byte[])it.Raw.Clone();
        for (int k = 0; k < 5; k++) { ushort v = U16(raw, VisOfs(it, k)); if (v != 0) Put(raw, VisOfs(it, k), MapVisual(v, null)); }
        addItems.Add(Rec(raw));
        log.WriteLine($"  equip item {id} '{it.Name}' ported (worn by a mount monster)");
    }

    // =========================================================================================== shared
    static ItemRec Rec(byte[] raw)
    {
        int l = raw[6];
        if (l == 0xFF) throw new InvalidDataException("long item name");
        return new ItemRec { Id = U16(raw, 0), Type = raw[2], Kind = raw[3], Name = TMon.Cp949.GetString(raw, 7, l), UseValue = U16(raw, 7 + l), TailOfs = 9 + l, Raw = raw };
    }

    ushort MapVisual(ushort v, Action<byte[]>? patch)
    {
        if (visMap.TryGetValue(v, out var m)) return m;
        if (!rVis.TryGetValue(v, out var src)) { log.WriteLine($"  warn: visual {v} missing in Revelation"); return visMap[v] = v; }
        var row = (byte[])src.Clone();
        patch?.Invoke(row);
        Put(row, 34, MapIcon(U16(row, 34), $"visual {v}"));
        ushort id = v;
        if (gVis.TryGetValue(v, out var mine) || addVis.ContainsKey(v))
        {
            if (mine != null && mine.AsSpan().SequenceEqual(row)) return visMap[v] = v;
            id = (ushort)(new[] { gVis.Keys.Max(), rVis.Keys.Max(), addVis.Count == 0 ? (ushort)0 : addVis.Keys.Max() }.Max() + 1);
        }
        Put(row, 0, id);
        addVis[id] = row;
        return visMap[v] = id;
    }

    ushort MapIcon(ushort icon, string what)
    {
        if (iconMap.TryGetValue(icon, out var m)) return m;
        var g = revIcons!.Cut(icon);
        if (g == null || g.Value.w <= 1) { log.WriteLine($"  warn: {what}: Revelation icon {icon} has no picture -> fallback {FallbackIcon}"); return iconMap[icon] = FallbackIcon; }
        var o = icon < 60000 ? ourIcons!.Cut(icon) : null;
        if (o is { } x && x.w == g.Value.w && x.h == g.Value.h && x.rgba.AsSpan().SequenceEqual(g.Value.rgba)) return iconMap[icon] = icon;
        ushort id = (ushort)(FirstIcon + pngs.Count);
        pngs[id] = Png.Encode(g.Value.w, g.Value.h, g.Value.rgba);
        return iconMap[icon] = id;
    }

    void Pull(IEnumerable<(Kind, uint)> roots)
    {
        var c = new Closure(G, R); c.Walk(roots);
        if (c.Missing.Count > 0) log.WriteLine($"  warn: resources missing in Revelation too: {string.Join(",", c.Missing.Select(x => $"{x.Item1}:{x.Item2:X8}"))}");
        foreach (var x in c.Collide.Where(x => !pull.Contains(x))) keptOurs++;
        foreach (var x in c.New) pull.Add(x);
    }

    string AuraTable(List<(ushort ourId, ushort revId)> auras)
    {
        var sb = new StringBuilder();
        sb.AppendLine("// Costume effects that 4Revelation hardcodes by item id (generated by Tools/GenesisPort `rev-port --apply` - do not edit).");
        sb.AppendLine("// Included by CTClientObjBase::ApplyCharacterSFX. Raw: PlayRawSFX(sfx resource); else PlayTActionSFX(TSFX id, tint).");
        sb.AppendLine("// { item, effect id, raw, tint }  - items with several rows play all of them.");
        foreach (var (ourId, revId) in auras.OrderBy(a => a.ourId))
        {
            var name = addItems.First(i => i.Id == ourId).Name;
            foreach (var fx in RevAura[revId])
                sb.AppendLine($"\t{{ {ourId,5}, 0x{fx.Id:X8}, {(fx.Raw ? "TRUE " : "FALSE")}, 0x{fx.Color:X8} }},   // {name}");
        }
        log.WriteLine($"aura table: {auras.Count} items, {auras.Sum(a => RevAura[a.revId].Length)} effects");
        return sb.ToString();
    }

    // =========================================================================================== writing
    static readonly Dictionary<Kind, (string pack, string idx)> Out = new()
    {
        [Kind.Obj] = (@"OBJ\Revelation.TOB", "TClientO.IDX"),
        [Kind.Mesh] = (@"Mesh\Revelation.TMH", "TClientM.IDX"),
        [Kind.Ani] = (@"Action\Revelation.TAC", "TClientA.IDX"),
        [Kind.Sfx] = (@"SFX\Revelation.TFX", "TClientX.IDX"),
    };

    void WritePacks(Dictionary<uint, byte[]> playerObjs)
    {
        string D(string rel) => Path.Combine(game, "Data", rel);
        string I(string name) => Path.Combine(game, "Index", name);
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
                if (idAt.TryGetValue((uint)p, out var id) && playerObjs.TryGetValue(id, out var rec)) outp.Write(rec); else outp.Write(tob, p, len);
                p += len;
            }
            oIdx.Entries = oIdx.Entries.Select(e => e.File == f ? e with { Pos = move[e.Pos] } : e).ToList();
            File.WriteAllBytes(D(@"OBJ\Character.TOB"), outp.ToArray());
        }
        foreach (var (k, (pack, idxName)) in Out)
        {
            var ids = pull.Where(x => x.Item1 == k).Select(x => x.Item2).OrderBy(x => x).ToList();
            if (ids.Count == 0) { if (k == Kind.Obj && playerObjs.Count > 0) oIdx.Save(I(idxName)); continue; }
            var idx = G.Idx[k]; int fi = idx.Files.Count; idx.Files.Add(pack);
            using (var fs = File.Create(D(pack)))
                foreach (var id in ids) { idx.Entries.Add(new IdxEntry(id, fi, (uint)fs.Position)); fs.Write(Packs.Stored(R, k, id)); }
            idx.Save(I(idxName));
        }
        // textures: Revelation ships one detail level (2_), used for all three of ours
        var tex = pull.Where(x => x.Item1 == Kind.Tex).Select(x => x.Item2).OrderBy(x => x).ToList();
        if (tex.Count > 0)
            for (int lvl = 0; lvl < 3; lvl++)
            {
                var gi = Idx.Load(I($"{lvl}_TClientS.IDX"));
                string pack = $@"Skin\{lvl}_{Tag}.TTX"; int fi = gi.Files.Count; gi.Files.Add(pack);
                using (var fs = File.Create(D(pack)))
                    foreach (var t in tex) { gi.Entries.Add(new IdxEntry(t, fi, (uint)fs.Position)); fs.Write(Packs.Stored(R, Kind.Tex, t)); }
                gi.Save(I($"{lvl}_TClientS.IDX"));
            }
        var media = pull.Where(x => x.Item1 == Kind.Media).Select(x => x.Item2).ToList();
        if (media.Count > 0)
        {
            var idx = G.Idx[Kind.Media]; var ridx = R.Idx[Kind.Media]; var fileMap = new Dictionary<int, int>();
            foreach (var id in media)
            {
                var e = ridx.ById[id];
                if (!fileMap.TryGetValue(e.File, out var fi))
                {
                    string src = ridx.Files[e.File], rel = Path.Combine(Path.GetDirectoryName(src)!, Tag + "_" + Path.GetFileName(src));
                    File.Copy(Path.Combine(rev, "Data", src), D(rel), true);
                    fileMap[e.File] = fi = idx.Files.Count; idx.Files.Add(rel);
                }
                idx.Entries.Add(new IdxEntry(id, fi, e.Pos));
            }
            idx.Save(I("TClientW.IDX"));
        }
    }

    void WriteTables(List<Import.MountRow> mounts, List<MonRec> monsters)
    {
        TItem.Save(GT("TItem.tcd"), gItems.Concat(addItems).OrderBy(i => i.Id));
        TVisualTcd.Save(GT("TItemVisual.tcd"), gVis.Concat(addVis).OrderBy(kv => kv.Key).Select(kv => kv.Value));
        TMon.Save(GT("TMon.tcd"), TMon.Load(GT("TMon.tcd"), TMon.OurTail).Concat(monsters).OrderBy(m => m.Id));
        TMount.Save(GT("TMount.tcd"), TMount.Load(GT("TMount.tcd"), false).Concat(mounts.Select(m => new Mount(m.Id, m.MonId, m.SaddleMonId, m.Icon, 1f))).OrderBy(m => m.Id), false);
        // TSFX.tcd: WORD count, rows DWORD id, sfx, posId
        var b = File.ReadAllBytes(GT("TSFX.tcd")); int n = BitConverter.ToUInt16(b, 0);
        using var w = new BinaryWriter(File.Create(GT("TSFX.tcd")));
        w.Write((ushort)(n + addTsfx.Count)); w.Write(b, 2, b.Length - 2);
        foreach (var (id, sfx, pos) in addTsfx) { w.Write(id); w.Write(sfx); w.Write(pos); }
    }

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
            expect[$"{k}:{id:X8}"] = k is Kind.Mesh or Kind.Ani ? Packs.Stored(R, k, id) : R.Get(k, id)!.Raw;
        int bad = expect.Count(kv => !after.TryGetValue(kv.Key, out var a) || !a.AsSpan().SequenceEqual(kv.Value)) + Math.Abs(after.Count - expect.Count);
        log.WriteLine($"read-back: {after.Count} records, {bad} unexpected");
        foreach (var (k, (pack, _)) in Out)
            if (k != Kind.Sfx && File.Exists(Path.Combine(game, "Data", pack)))
            {
                var (n, rest) = Packs.WalkFile(k, Path.Combine(game, "Data", pack));
                log.WriteLine($"  {pack}: {n} records, {rest} trailing bytes");
                if (rest != 0) bad++;
            }
        if (bad != 0) throw new InvalidDataException("read-back mismatch - restore Game/ from git");
    }
}
