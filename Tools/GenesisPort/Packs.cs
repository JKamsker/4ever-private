// Exact record lengths for the sequentially-loaded packs (.TOB/.TMH/.TAC): the engine walks these files record by
// record (CTachyonRes::LoadOBJ / LoadMESH / LoadANI), so a copied record must be exactly one record. Genesis packs are
// append-only (unindexed orphans between indexed records), so "slice up to the next indexed record" is not enough.
using ClassicPort;

public static class Packs
{
    const int MeshVertex = 40, WMeshVertex = 48;      // MESHVERTEX (2 UV sets) / WMESHVERTEX (WEIGHT_COUNT 3), T3D.h

    static int I32(byte[] b, int p) => BitConverter.ToInt32(b, p);
    static readonly Dictionary<string, byte[]> cache = new();

    /// <summary>Length of the record at p (LoadMESH field order).</summary>
    public static int MeshLength(byte[] b, int p0)
    {
        int meshes = I32(b, p0), nodes = I32(b, p0 + 4), level = I32(b, p0 + 9);
        int p = p0 + 29 + 64 * nodes;
        int verts = I32(b, p); p += 4 + verts * (nodes > 0 ? WMeshVertex : MeshVertex);
        for (int i = 0; i < meshes; i++)
            for (int j = 0; j < level; j++)
            {
                int n = I32(b, p); p += 4;
                for (int k = 0; k < n; k++) { int c = I32(b, p); p += 4 + 2 + 2 * c; }
            }
        return p + 4 * Math.Max(level - 1, 0) - p0;
    }

    /// <summary>Length of the record at p (LoadANI field order).</summary>
    public static int AniLength(byte[] b, int p0)
    {
        int p = p0;
        int n = I32(b, p); p += 4 + 5 * n;
        n = I32(b, p); p += 4 + 44 * n;
        p += 4 + 4 + 1;
        int size = I32(b, p); p += 4 + size;
        return p - p0;
    }

    public static int ObjLength(byte[] b, int p0)
    {
        var r = new BinaryReader(new MemoryStream(b, p0, b.Length - p0));
        Client.ParseObj(r, new(), out int len);
        return len;
    }

    public static int Length(Kind k, byte[] b, int p) => k switch
    {
        Kind.Mesh => MeshLength(b, p),
        Kind.Ani => AniLength(b, p),
        Kind.Obj => ObjLength(b, p),
        _ => throw new ArgumentException($"{k} is not a sequential pack"),
    };

    /// <summary>The exact record bytes of a resource (Obj/Mesh/Ani: one record; Tex/Sfx: the stored zlib chunk).</summary>
    public static byte[] Stored(Client c, Kind k, uint id)
    {
        var idx = c.Idx[k];
        var e = idx.ById[id];
        string path = Path.Combine(c.Root, "Data", idx.Files[e.File]);
        if (!cache.TryGetValue(path, out var file)) cache[path] = file = File.ReadAllBytes(path);
        int p = (int)e.Pos;
        int len = k is Kind.Tex or Kind.Sfx ? 8 + I32(file, p + 4) : Length(k, file, p);
        return file[p..(p + len)];
    }

    /// <summary>walk: every record of a pack parses back to back to the end of the file.</summary>
    public static (int records, long rest) WalkFile(Kind k, string path)
    {
        var b = File.ReadAllBytes(path);
        int p = 0, n = 0;
        while (p < b.Length) { p += Length(k, b, p); n++; }
        return (n, b.Length - (long)p);
    }
}
