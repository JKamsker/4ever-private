using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=drops</c>: monster drops with the real chart. A stands by spawn 4200 (map 0) and kills up to 20 level-2
/// monsters (770), whose drop rows are almost all ranged (no item: one of the row's group, drop levels and id range). Every
/// dropped item must fit one of the monster's rows; A takes the first one, which must be saved after logout. The taken item and
/// A's position are put back.
/// </summary>
public static partial class Scenarios
{
    /// <summary>An item block with its slot: the slot, item id and number of magic options.</summary>
    private static (byte Slot, ushort Id, int Magic) DropItem(PacketReader r)
    {
        byte slot = r.ReadByte();
        ushort id = r.ReadUInt16(); r.ReadByte(); r.ReadByte(); r.ReadUInt16(); r.ReadUInt16(); r.ReadByte();
        r.ReadUInt32(); r.ReadUInt32(); r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadInt64();
        r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadUInt16(); r.ReadUInt16(); r.ReadByte();
        byte m = r.ReadByte();
        for (int k = 0; k < m; k++) { r.ReadByte(); r.ReadUInt16(); }
        return (slot, id, m);
    }

    private static async Task<int> RunDropsAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account);
        var pos = await db.RowAsync("SELECT wMapID, fPosX, fPosY, fPosZ FROM TCHARTABLE WHERE dwCharID=@p0", (int)idA);
        var maxDl = await db.RowAsync("SELECT ISNULL(MAX(dlID), 0) FROM TITEMTABLE");
        long lastDl = Convert.ToInt64(maxDl![0]);
        try
        {
            await db.ExecAsync("UPDATE TCHARTABLE SET wMapID=0, fPosX=3716, fPosY=87.23, fPosZ=548 WHERE dwCharID=@p0", (int)idA);
            await Drops(cfg, db, idA, lastDl);
        }
        finally
        {
            await Task.Delay(2500);
            await db.ExecAsync("DELETE FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND dlID > @p1", (int)idA, lastDl);
            if (pos is not null)
                await db.ExecAsync("UPDATE TCHARTABLE SET wMapID=@p1, fPosX=@p2, fPosY=@p3, fPosZ=@p4 WHERE dwCharID=@p0", (int)idA, pos[0], pos[1], pos[2], pos[3]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] taken item removed, A's position put back");
        }
        return PrintResults();
    }

    private static async Task Drops(BotConfig cfg, GameDb db, uint idA, long lastDl)
    {
        var rows = new List<(byte Chart, int Item, int Min, int Max, int LvMin, int LvMax)>();
        for (int k = 0; ; k++)
        {
            var row = await db.RowAsync(@"SELECT bChartType, wItemID, wItemIDMin, wItemIDMax, bLevelMin, bLevelMax FROM TMONITEMCHART WHERE wMonID=@p0
                ORDER BY bChartType OFFSET @p1 ROWS FETCH NEXT 1 ROWS ONLY", (int)FieldMon, k);
            if (row is null) break;
            rows.Add((Convert.ToByte(row[0]), Convert.ToInt32(row[1]), Convert.ToInt32(row[2]), Convert.ToInt32(row[3]), Convert.ToInt32(row[4]), Convert.ToInt32(row[5])));
        }

        var dropped = new List<(ushort Id, int Magic)>();
        int kills = 0;
        ushort taken = 0;
        using (var a = new Bot(Clone(cfg, cfg.Account, "[A]")))
        {
            a.Enter();
            Thread.Sleep(2000);
            while (kills < 20)
            {
                uint mon = KillFieldMon(a);
                if (mon == 0) break;
                kills++;
                a.Discard(CS_MONITEMLIST_ACK);
                a.Send(Req(CS_MONITEMLIST_REQ, w => { w.WriteByte(1); w.WriteUInt32(mon); }));
                var list = a.TryWait(CS_MONITEMLIST_ACK, r => { r.ReadByte(); r.ReadByte(); return r.ReadUInt32() == mon; });
                var items = list is null ? new List<(byte Slot, ushort Id, int Magic)>() : Read(list, r =>
                {
                    var l = new List<(byte Slot, ushort Id, int Magic)>();
                    r.ReadByte(); r.ReadByte(); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32();
                    if (r.Remaining == 0) return l;
                    for (byte n = r.ReadByte(), i = 0; i < n; i++) l.Add(DropItem(r));
                    return l;
                });
                dropped.AddRange(items.Select(i => (i.Id, i.Magic)));
                if (taken == 0 && items.Count > 0)
                {
                    a.Discard(CS_GETITEM_ACK);
                    a.Send(Req(CS_MONITEMTAKE_REQ, w => { w.WriteUInt32(mon); w.WriteByte(items[0].Slot); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
                    var got = a.TryWait(CS_GETITEM_ACK);
                    if (got is not null && Read(got, r => DropItem(r).Id) == items[0].Id) taken = items[0].Id;
                    Check("drops: A takes a dropped item (CS_GETITEM_ACK)", taken != 0, Describe(got));
                }
                a.Send(Req(CS_MONITEMLIST_REQ, w => { w.WriteByte(0); w.WriteUInt32(mon); }));
            }
        }
        Report("drops", $"{dropped.Count} item(s) from {kills} kill(s): {string.Join(", ", dropped.Select(d => d.Magic > 0 ? $"{d.Id}+{d.Magic}" : $"{d.Id}"))}");
        Check("drops: monster 770 drops items again (ranged rows)", dropped.Count > 0, $"{kills} kill(s)");

        bool fits = true;
        var bad = new List<ushort>();
        foreach (var (id, _) in dropped.DistinctBy(d => d.Id))
        {
            var t = await db.RowAsync("SELECT bGroupID, bDropLevel, bInitState FROM TITEMCHART WHERE wItemID=@p0", (int)id);
            byte group = t is null ? (byte)0 : Convert.ToByte(t[0]);
            int level = t is null ? -1 : Convert.ToInt32(t[1]);
            bool ok = rows.Any(r => r.Item == id || (r.Item == 0 && r.Chart == group && id >= r.Min && id <= r.Max && level >= r.LvMin && level <= r.LvMax
                && t is not null && Convert.ToInt32(t[2]) != 0));
            if (!ok) { fits = false; bad.Add(id); }
        }
        Check("drops: every item fits one of the monster's rows (group, drop level, id range)", fits, bad.Count == 0 ? "all fit" : string.Join(",", bad));

        await Task.Delay(2500);
        var saved = await db.RowAsync("SELECT wItemID FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND dlID > @p1 AND wItemID=@p2",
            (int)idA, lastDl, (int)taken);
        Check("drops: the taken item is saved after logout", taken != 0 && saved is not null, taken == 0 ? "nothing taken" : saved is null ? "no row" : "saved");
    }
}
