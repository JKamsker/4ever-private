// Checks that every item / mount ported from 4Revelation shows the same icon as in 4Revelation: our item is
// matched to the Revelation item with the same name (mounts: same monster name), and both
// icons are cut out of their atlases and compared pixel by pixel.
using ClassicPort;

public static class RevIconMatch
{
    public static int Run(string game, string rev, ushort firstItem, ushort lastItem, ushort firstMountItem, ushort lastMountItem, ushort firstMount)
    {
        var ours = new IconCut(game); var theirs = new IconCut(rev);
        static Dictionary<ushort, ushort> Vis(string p)
        {
            var v = File.ReadAllBytes(p); var d = new Dictionary<ushort, ushort>();
            for (int i = 0, n = BitConverter.ToUInt16(v, 0); i < n; i++) d.TryAdd(BitConverter.ToUInt16(v, 2 + 63 * i), BitConverter.ToUInt16(v, 2 + 63 * i + 34));
            return d;
        }
        var gVis = Vis(Path.Combine(game, "Tcd", "TItemVisual.tcd")); var rVis = Vis(Path.Combine(rev, "Tcd", "TItemVisual.tcd"));
        static ushort Visual(ItemRec it) => BitConverter.ToUInt16(it.Raw, it.TailOfs + 63);
        var rItems = TItem.Load(Path.Combine(rev, "Tcd", "TItem.tcd"));

        int same = 0, diff = 0, unmatched = 0;
        bool Same(ushort a, ushort b)
        {
            var x = ours.Cut(a); var y = theirs.Cut(b);
            return x is { } p && y is { } q && p.w == q.w && p.h == q.h && p.rgba.AsSpan().SequenceEqual(q.rgba);
        }
        void Check(string who, ushort ourIcon, IEnumerable<ushort> revIcons)
        {
            var cands = revIcons.Distinct().ToList();
            if (cands.Count == 0) { unmatched++; Console.WriteLine($"  ? {who}: no Revelation counterpart"); return; }
            if (cands.Any(r => Same(ourIcon, r))) { same++; return; }
            diff++; Console.WriteLine($"  DIFF {who}: our icon {ourIcon} vs Revelation {string.Join("/", cands)}");
        }

        foreach (var it in TItem.Load(Path.Combine(game, "Tcd", "TItem.tcd"))
                     .Where(i => (i.Id >= firstItem && i.Id <= lastItem) || (i.Id >= firstMountItem && i.Id <= lastMountItem)))
        {
            if (!gVis.TryGetValue(Visual(it), out var icon)) { Console.WriteLine($"  ? item {it.Id} {it.Name}: no visual"); unmatched++; continue; }
            var named = rItems.Where(r => r.Name == it.Name).ToList();
            string seen = string.Join(", ", named.Select(r => $"rev {r.Id} visual {Visual(r)}" + (rVis.TryGetValue(Visual(r), out var ri) ? $" icon {ri}" : " (no visual row)")));
            Check($"item {it.Id} {it.Name} (our icon {icon}; {seen})", icon, named
                .Select(r => rVis.TryGetValue(Visual(r), out var ri) ? ri : (ushort)0).Where(ri => ri != 0));
        }

        var gMon = TMon.Load(Path.Combine(game, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var rMon = TMon.Load(Path.Combine(rev, "Tcd", "TMon.tcd"), TMon.OurTail).ToDictionary(m => m.Id);
        var rMounts = TMount.Load(Path.Combine(rev, "Tcd", "TMount.tcd"), false);
        foreach (var m in TMount.Load(Path.Combine(game, "Tcd", "TMount.tcd"), false).Where(m => m.Id >= firstMount))
        {
            string name = gMon[m.MonId].Name;
            Check($"mount {m.Id} {name}", m.Icon, rMounts.Where(r => rMon.TryGetValue(r.MonId, out var rm) && rm.Name == name).Select(r => r.Icon));
        }
        Console.WriteLine($"Revelation icons: {same} identical, {diff} different, {unmatched} without a counterpart");
        return diff;
    }
}
