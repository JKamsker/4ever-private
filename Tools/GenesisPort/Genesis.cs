// Genesis-specific loading: the newest player objects (orphans in 4Genesis.TOB), TPet.tcd, resource closures.
using ClassicPort;

public static class Genesis
{
    public static readonly uint[] Players = [0xF56A4, 0xF56A5, 0xF56A6, 0xF56A7, 0xF56A8, 0xF56A9];

    /// <summary>The 6 player objects as Genesis really ships them. Its index points them at OBJ\IWW.TOB, which it
    /// never distributes; 4Genesis.TOB is append-only and got the 6 objects re-appended (unindexed) on every
    /// costume update, so the newest copies are the last 6 large unindexed records, in F56A4..F56A9 order.</summary>
    public static Dictionary<uint, byte[]> PlayerObjects(Client n)
    {
        string path = Path.Combine(n.Root, "Data", "OBJ", "4Genesis.TOB");
        var b = File.ReadAllBytes(path);
        var starts = Walk.TobStarts(path);
        int f = n.Idx[Kind.Obj].Files.IndexOf(@"OBJ\4Genesis.TOB");
        var indexed = n.Idx[Kind.Obj].Entries.Where(e => e.File == f).Select(e => (long)e.Pos).ToHashSet();
        var recs = Enumerable.Range(0, starts.Count).Where(i => !indexed.Contains(starts[i]))
            .Select(i => b[(int)starts[i]..(int)(i + 1 < starts.Count ? starts[i + 1] : b.Length)])
            .Where(r => r.Length > 100_000).TakeLast(6).ToList();
        if (recs.Count != 6) throw new InvalidDataException("expected 6 player objects in 4Genesis.TOB");
        return Players.Zip(recs).ToDictionary(x => x.First, x => x.Second);
    }

    /// <summary>TPet.tcd (Genesis InitTPET @0x73ae40): WORD count, 17-byte rows
    /// WORD key=(petId&lt;&lt;8)|race, BYTE, BYTE, WORD speed, WORD, BYTE, DWORD, WORD icon, WORD monId.</summary>
    public record Pet(ushort Id, byte Race, byte[] Raw)
    {
        public ushort Icon => BitConverter.ToUInt16(Raw, 13);
        public ushort MonId => BitConverter.ToUInt16(Raw, 15);
    }

    public static List<Pet> LoadPets(string path)
    {
        var b = File.ReadAllBytes(path);
        int n = BitConverter.ToUInt16(b, 0);
        if (2 + 17 * n != b.Length) throw new InvalidDataException(path);
        return Enumerable.Range(0, n).Select(i => b[(2 + 17 * i)..(2 + 17 * (i + 1))])
            .Select(r => new Pet((ushort)(BitConverter.ToUInt16(r, 0) >> 8), r[0], r)).ToList();
    }
}

/// <summary>Walks a Genesis resource graph and sorts every reachable resource into: already ours (same bytes),
/// new (id free in ours), or COLLIDING (id taken in ours by different content -> must be remapped).</summary>
public class Closure
{
    public readonly HashSet<(Kind, uint)> Same = new(), New = new(), Collide = new(), Missing = new();
    readonly Client G, N;
    public Closure(Client g, Client n) { G = g; N = n; }

    public void Walk(IEnumerable<(Kind, uint)> roots)
    {
        var todo = new Stack<(Kind, uint)>(roots);
        var seen = new HashSet<(Kind, uint)>();
        while (todo.Count > 0)
        {
            var (k, id) = todo.Pop();
            if (id == 0 || !seen.Add((k, id))) continue;
            if (k == Kind.Media) { (G.Has(k, id) ? Same : New).Add((k, id)); continue; }
            Rec? rec;
            try { rec = N.Get(k, id); } catch (FileNotFoundException) { rec = null; }
            if (rec == null) { Missing.Add((k, id)); continue; }
            if (G.Has(k, id))
            {
                if (G.Get(k, id)!.Raw.AsSpan().SequenceEqual(rec.Raw)) { Same.Add((k, id)); continue; }
                Collide.Add((k, id));
            }
            else New.Add((k, id));
            foreach (var r in rec.Refs) todo.Push(r);
        }
    }

    public string Summary() =>
        string.Join("  ", new[] { ("same", Same), ("new", New), ("COLLIDE", Collide), ("missing", Missing) }
            .Select(x => $"{x.Item1} {x.Item2.Count} [{string.Join(",", x.Item2.GroupBy(k => k.Item1).Select(g => $"{g.Key}:{g.Count()}"))}]"));
}
