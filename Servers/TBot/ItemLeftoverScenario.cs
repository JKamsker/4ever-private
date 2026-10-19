using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=itemleftovers</c>: batch S7a live, on the charts' real items. A falls (its HP goes down, B sees it) and is refused
/// when reporting B's fall. A sets off an act item (B sees the cast, one item spent), uses a medal package (its medals go up), turns a
/// refine stone into its item, and opens a special box's rewards window. Before: the three items are put in A's backpack; after: they
/// (and the refined item) are removed, and A's HP and medals put back. Stop-the-clock spends cash, so it is left to the unit tests.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_DROPDAMAGE_REQ = M + 0x0108, CS_DROPDAMAGE_ACK = M + 0x0109, CS_WASTEREFINE_REQ = M + 0x01F9,
        CS_WASTEREFINE_ACK = M + 0x01FA, CS_GETREWARDS_REQ = M + 0x0325, CS_GETREWARDS_ACK = M + 0x0326, CS_UPDATEMEDALS_REQ = M + 0x0350,
        CS_USEMEDALITEM_REQ = M + 0x0351, CS_ACTITEMSUSE_REQ = M + 0x0376;
    private const byte LeftoverBag = 0xFF, ActSlot = 40, MedalSlot = 41, StoneSlot = 42;
    private const long ActDl = 900_000_071, MedalDl = 900_000_072, StoneDl = 900_000_073;

    private static async Task<int> RunItemLeftoversAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account);
        var act = await db.RowAsync(@"SELECT TOP 1 s.wID, s.wItemID FROM TSKILLCHART s JOIN TITEMCHART i ON i.wItemID = s.wItemID
            WHERE i.bType = 25 AND s.wMapID = 65535 ORDER BY s.wID");
        var medal = await db.RowAsync("SELECT TOP 1 wItemID, wUseValue FROM TITEMCHART WHERE bType = 15 AND bKind = 118 ORDER BY wItemID");
        var stone = await db.RowAsync(@"SELECT TOP 1 i.wItemID, i.wUseValue FROM TITEMCHART i JOIN TITEMCHART t ON t.wItemID = i.wUseValue
            WHERE i.bType = 14 ORDER BY i.wItemID");
        var box = await db.RowAsync("SELECT TOP 1 wGroup FROM TSPECIALBOXCHART WHERE bClass = 6 ORDER BY wGroup");
        if (act is null || medal is null || stone is null) { Report("itemleftovers", "the charts have no act item, medal package or refine stone"); return 2; }
        ushort actSkill = Convert.ToUInt16(act[0]), actItem = Convert.ToUInt16(act[1]);
        ushort medalItem = Convert.ToUInt16(medal[0]), medalValue = Convert.ToUInt16(medal[1]);
        ushort stoneItem = Convert.ToUInt16(stone[0]), refined = Convert.ToUInt16(stone[1]);
        Report("itemleftovers", $"act skill {actSkill} (item {actItem}), medal {medalItem} (+{medalValue}), stone {stoneItem} → {refined}, box {box?[0]}");

        var hp = await db.RowAsync("SELECT dwHP FROM TCHARTABLE WHERE dwCharID=@p0", (int)idA);
        var medals = await db.RowAsync("DECLARE @m INT; EXEC TGetMedals @p0, @m OUTPUT; SELECT ISNULL(@m, 0)", (int)idA);
        long Dl(byte slot) => slot == ActSlot ? ActDl : slot == MedalSlot ? MedalDl : StoneDl;
        Task Clear() => db.ExecAsync(@"DELETE FROM TITEMTABLE WHERE dlID IN (@p0, @p1, @p2)
            OR (bOwnerType=0 AND dwOwnerID=@p3 AND bStorageType=0 AND dwStorageID=255 AND (bItemID IN (@p4, @p5, @p6) OR wItemID=@p7))",
            ActDl, MedalDl, StoneDl, (int)idA, (int)ActSlot, (int)MedalSlot, (int)StoneSlot, (int)refined);
        try
        {
            await Clear();
            foreach (var (slot, item, count) in new[] { (ActSlot, actItem, 2), (MedalSlot, medalItem, 2), (StoneSlot, stoneItem, 1) })
                await db.ExecAsync(@"INSERT INTO TITEMTABLE (dlID, bStorageType, dwStorageID, bOwnerType, dwOwnerID, bItemID, wItemID, bLevel,
                    bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime, bGradeEffect, bMagic1, bMagic2, bMagic3, bMagic4, bMagic5,
                    bMagic6, wValue1, wValue2, wValue3, wValue4, wValue5, wValue6, dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6,
                    bGem, wMoggItemID)
                    VALUES (@p0, 0, 255, 0, @p1, @p2, @p3, 0, @p4, 0, 0, 0, 0, '1900-01-01', 0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0, 0)",
                    Dl(slot), (int)idA, (int)slot, (int)item, count);
            await db.ExecAsync("UPDATE TCHARTABLE SET dwHP = 100000 WHERE dwCharID=@p0", (int)idA);         // the char's max, after the load
            ItemLeftovers(cfg, actSkill, medalValue, Convert.ToUInt32(medals?[0] ?? 0), box is null ? null : Convert.ToUInt16(box[0]));
        }
        finally
        {
            await Task.Delay(2500);                                                                 // the logout save lands first
            await Clear();
            if (hp is not null) await db.ExecAsync("UPDATE TCHARTABLE SET dwHP=@p1 WHERE dwCharID=@p0", (int)idA, hp[0]);
            if (medals is not null) await db.ExecAsync("EXEC TSaveMedals @p0, @p1", (int)idA, medals[0]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] items removed, HP and medals put back");
        }
        return PrintResults();
    }

    private static void ItemLeftovers(BotConfig cfg, ushort actSkill, ushort medalValue, uint medals, ushort? box)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        a.Enter(); b.Enter();
        Thread.Sleep(1500);
        uint idA = a.CharId, idB = b.CharId;

        PacketWriter Fall(uint target) => Req(CS_DROPDAMAGE_REQ, w => { w.WriteUInt32(target); w.WriteByte(1); w.WriteUInt32(10); w.WriteByte(1); w.WriteUInt16(0); });
        a.Send(Fall(idA));
        var seen = b.TryWait(CS_DROPDAMAGE_ACK);
        Check("itemleftovers: A's fall is shown around (CS_DROPDAMAGE_ACK)", seen is not null
            && Read(seen, r => (r.ReadUInt32(), r.ReadByte(), r.ReadUInt32())) == (idA, 1, 10u), Describe(seen));

        a.Send(Fall(idB));
        var told = a.TryWait(CS_CHAT_ACK);
        var none = b.TryWait(CS_DROPDAMAGE_ACK, timeoutMs: 1500);
        Check("itemleftovers: reporting B's fall gets the operator's word, and B is not hurt", told is not null && none is null, Describe(told));

        a.Send(Req(CS_ACTITEMSUSE_REQ, w =>
        {
            w.WriteUInt32(idA); w.WriteByte(0); w.WriteByte(1); w.WriteUInt16(0); w.WriteUInt16(actSkill); w.WriteByte(1);
            w.WriteUInt32(0); w.WriteUInt32(0); w.WriteFloat(a.Spawn.X); w.WriteFloat(a.Spawn.Y); w.WriteFloat(a.Spawn.Z); w.WriteUInt32(0); w.WriteByte(0);
        }));
        var cast = b.TryWait(CS_SKILLUSE_ACK);
        Check("itemleftovers: B sees A's act item go off (CS_SKILLUSE_ACK success)", cast is not null && cast.ReadByte() == 0, Describe(cast));

        a.Send(Req(CS_USEMEDALITEM_REQ, w => { w.WriteByte(LeftoverBag); w.WriteByte(MedalSlot); w.WriteByte(1); }));
        var m = a.TryWait(CS_UPDATEMEDALS_REQ);
        Check($"itemleftovers: a medal package adds its {medalValue} medals", m is not null && Read(m, r => r.ReadUInt32()) == medals + medalValue, Describe(m));

        a.Send(Req(CS_WASTEREFINE_REQ, w => { w.WriteByte(LeftoverBag); w.WriteByte(StoneSlot); }));
        var refine = a.TryWait(CS_WASTEREFINE_ACK);
        Check("itemleftovers: a refine stone becomes its item (WASTEREFINE_SUCCESS)", refine is not null && Read(refine, r => r.ReadByte()) == 0, Describe(refine));

        if (box is { } group)
        {
            a.Send(Req(CS_GETREWARDS_REQ, w => { w.WriteByte(115); w.WriteUInt16(group); }));
            var rewards = a.TryWait(CS_GETREWARDS_ACK);
            Check($"itemleftovers: the rewards window lists special box {group}", rewards is not null
                && rewards.ReadUInt16() > 0, Describe(rewards));
        }
    }
}
