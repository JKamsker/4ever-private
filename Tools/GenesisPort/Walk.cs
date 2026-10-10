using ClassicPort;

/// <summary>Exploration: Genesis merged several packs into 4Genesis.* without fixing the IDX file table.</summary>
public static class Walk
{
    /// <summary>Record start offsets of a .TOB pack, parsed sequentially.</summary>
    public static List<long> TobStarts(string path)
    {
        var b = File.ReadAllBytes(path);
        var r = new BinaryReader(new MemoryStream(b));
        var starts = new List<long>();
        while (r.BaseStream.Position < b.Length)
        {
            starts.Add(r.BaseStream.Position);
            Client.ParseObj(r, new(), out _);
        }
        return starts;
    }
}
