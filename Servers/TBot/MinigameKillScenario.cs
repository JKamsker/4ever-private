using System.Diagnostics;
using TLogin.Protocol;

namespace TBot;

/// <summary>
/// The second part of <c>--Bot:Scenario=minigame</c>: the mini-games that need kills, on the field by spawn 4200 (map 0).
/// <list type="bullet">
/// <item><b>The soul bar</b>: A is set to level 1 (a bar of 30 exp) and kills level-2 monsters (770) until the bar is full
/// (<c>CS_SOULUP_ACK</c>, and the 4th field of <c>CS_EXP_ACK</c>); it draws its four numbers and the prize, then the number.</item>
/// <item><b>The party loot lottery</b>: A and B form a lottery party. Monster 770's drop of item 9407 is made sure for the test (its
/// <c>TMONITEMCHART</c> row's odds raised, the map restarted to load it). A kills until a corpse holds the item and takes it: B is asked, the item
/// leaves the list. The first time B says no and A gets it at once; the second time B says nothing and the corpse's AI draws after
/// its 15 seconds.</item>
/// </list>
/// The row's odds are put back and the map restarted after; A's level, exp, skill points, both bots' positions and the drawn items are put back.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_MONITEMLIST_REQ = M + 0x0088, CS_MONITEMLIST_ACK = M + 0x0089, CS_MONITEMTAKE_REQ = M + 0x008A,
        CS_MONITEMLOTTERY_REQ = M + 0x008C, CS_MONITEMLOTTERY_ACK = M + 0x008D, CS_GETITEM_ACK = M + 0x0144, CS_PARTYITEMTAKE_ACK = M + 0x0142,
        CS_SOULUP_ACK = M + 0x027A, CS_SOULLOTTERY_TRIGGER_REQ = M + 0x027D, CS_SOULLOTTERY_TRIGGER_ACK = M + 0x027E;
    private const ushort FieldMon = 770, LotteryLoot = 9407;

    private static async Task MinigameKillsAsync(BotConfig cfg, GameDb db, uint idA, uint idB)
    {
        const string Cols = "SELECT bLevel, dwEXP, wSkillPoint, dwHP, dwMP, wMapID, fPosX, fPosY, fPosZ FROM TCHARTABLE WHERE dwCharID=@p0";
        var saveA = await db.RowAsync(Cols, (int)idA);
        var saveB = await db.RowAsync(Cols, (int)idB);
        var maxDl = await db.RowAsync("SELECT ISNULL(MAX(dlID), 0) FROM TITEMTABLE");
        long lastDl = Convert.ToInt64(maxDl![0]);
        var drop = await db.RowAsync("SELECT bItemProb_N1, bItemProb_N2, bItemProb_N3, bItemProb_N4, wWeight FROM TMONITEMCHART WHERE bChartType=5 AND wMonID=@p0 AND wItemID=@p1",
            (int)FieldMon, (int)LotteryLoot);
        try
        {
            await db.ExecAsync("UPDATE TCHARTABLE SET bLevel=1, dwEXP=0 WHERE dwCharID=@p0", (int)idA);
            await db.ExecAsync("UPDATE TCHARTABLE SET wMapID=0, fPosX=3716, fPosY=87.23, fPosZ=548 WHERE dwCharID IN (@p0, @p1)", (int)idA, (int)idB);
            await db.ExecAsync(@"UPDATE TMONITEMCHART SET bItemProb_N1=100, bItemProb_N2=100, bItemProb_N3=100, bItemProb_N4=100, wWeight=1000
                WHERE bChartType=5 AND wMonID=@p0 AND wItemID=@p1", (int)FieldMon, (int)LotteryLoot);
            await RestartMap("a sure drop for monster 770 in");
            await KillGames(cfg);
        }
        finally
        {
            await Task.Delay(2500);                                              // the logout saves land first
            if (drop is not null)
                await db.ExecAsync(@"UPDATE TMONITEMCHART SET bItemProb_N1=@p2, bItemProb_N2=@p3, bItemProb_N3=@p4, bItemProb_N4=@p5, wWeight=@p6
                    WHERE bChartType=5 AND wMonID=@p0 AND wItemID=@p1", (int)FieldMon, (int)LotteryLoot, drop[0], drop[1], drop[2], drop[3], drop[4]);
            await db.ExecAsync("DELETE FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID IN (@p0, @p1) AND dlID > @p2 AND wItemID=@p3",
                (int)idA, (int)idB, lastDl, (int)LotteryLoot);
            foreach (var (id, row) in new[] { (idA, saveA), (idB, saveB) })
                if (row is not null)
                    await db.ExecAsync(@"UPDATE TCHARTABLE SET bLevel=@p1, dwEXP=@p2, wSkillPoint=@p3, dwHP=@p4, dwMP=@p5, wMapID=@p6, fPosX=@p7,
                        fPosY=@p8, fPosZ=@p9 WHERE dwCharID=@p0", (int)id, row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] drop odds, drawn items, A's level and both positions put back");
            await RestartMap("the drop odds back in");
        }
    }

    /// <summary>Restarts the map container and waits until it has announced itself to the world again.</summary>
    private static async Task RestartMap(string why)
    {
        Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] {why} the map; restarting it ...");
        long since = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        await Docker("compose restart mapsvr");
        for (int i = 0; i < 60; i++)
        {
            using var p = Process.Start(new ProcessStartInfo("docker", $"logs --since {since} araz-mapsvr")
                { RedirectStandardOutput = true, RedirectStandardError = true, UseShellExecute = false })!;
            string log = await p.StandardOutput.ReadToEndAsync() + await p.StandardError.ReadToEndAsync();
            await p.WaitForExitAsync();
            if (log.Contains("Announced to world")) { await Task.Delay(2000); return; }
            await Task.Delay(2000);
        }
        Console.WriteLine("the map did not come back in time");
    }

    /// <summary>The living monsters of a kind A was shown: shown (CS_ADDMON_ACK) and not since dead or gone.</summary>
    private static List<uint> LivingMonsters(Bot a, ushort chart)
    {
        var alive = new Dictionary<uint, bool>();
        byte[][] seen;
        lock (a.Received) seen = a.Received.ToArray();
        foreach (var p in seen)
        {
            ushort id = PacketHeader.ReadId(p);
            if (id == CS_ADDMON_ACK && BitConverter.ToUInt16(p, PacketHeader.Size + 4) == chart) alive[BitConverter.ToUInt32(p, PacketHeader.Size)] = true;
            else if ((id == CS_DIE_ACK || id == CS_DELMON_ACK) && alive.ContainsKey(BitConverter.ToUInt32(p, PacketHeader.Size)))
                alive[BitConverter.ToUInt32(p, PacketHeader.Size)] = false;
        }
        return alive.Where(kv => kv.Value).Select(kv => kv.Key).ToList();
    }

    /// <summary>A strikes a living 770 until it dies; returns its id, or 0.</summary>
    private static uint KillFieldMon(Bot a)
    {
        foreach (uint mon in LivingMonsters(a, FieldMon))
        {
            for (int i = 0; i < 300; i++)
            {
                a.Send(FinishSkill(a, a.CharId, 1, BasicMelee, a.Spawn.X, a.Spawn.Z, (mon, 2)));
                if (a.TryWait(CS_DIE_ACK, r => r.ReadUInt32() == mon, 60) is not null) return mon;
                if (i == 30 && a.TryWait(CS_DEFEND_ACK, r => { r.ReadUInt32(); return r.ReadUInt32() == mon; }, 10) is null) break;   // not reachable
            }
        }
        return 0;
    }

    /// <summary>A corpse's loot list: the result and the items (slot, item id).</summary>
    private static (byte Result, List<(byte Slot, ushort Item)> Items) LootList(PacketReader p) => Read(p, r =>
    {
        byte ret = r.ReadByte(); r.ReadByte(); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32();
        var items = new List<(byte, ushort)>();
        if (r.Remaining == 0) return (ret, items);
        for (byte n = r.ReadByte(), i = 0; i < n; i++) { byte slot = r.ReadByte(); items.Add((slot, MiniItem(r).Id)); }
        return (ret, items);
    });

    private static async Task KillGames(BotConfig cfg)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        a.Enter(); b.Enter();
        Thread.Sleep(2000);

        // The soul bar, solo: level-1 A kills until it is full.
        uint lastKill = 0;
        PacketReader? up = null;
        for (int k = 0; k < 8 && up is null; k++)
        {
            lastKill = KillFieldMon(a);
            if (lastKill == 0) break;
            up = a.TryWait(CS_SOULUP_ACK, r => r.ReadUInt32() == a.CharId, 1500);
        }
        Check("soul lottery: kills fill A's soul bar, shown around (CS_SOULUP_ACK)", up is not null && Read(up, r => r.ReadUInt32()) == a.CharId,
            lastKill == 0 ? "no monster 770 to kill" : Describe(up));
        PacketReader? exp = null;
        byte[][] got;
        lock (a.Received) got = a.Received.ToArray();
        var lastExp = got.LastOrDefault(p => PacketHeader.ReadId(p) == CS_EXP_ACK);
        if (lastExp is not null) exp = new PacketReader(lastExp);
        Check("soul lottery: … CS_EXP_ACK's soul field is the full bar (ceiling − floor)", exp is not null
            && Read(exp, r => { r.ReadUInt32(); uint floor = r.ReadUInt32(), ceil = r.ReadUInt32(); return r.ReadUInt32() == ceil - floor; }), Describe(exp));

        a.Send(Req(CS_SOULLOTTERY_REQ, _ => { }));
        var draw = a.TryWait(CS_SOULLOTTERY_ACK);
        var (numbers, prize) = draw is null ? (Array.Empty<uint>(), 0u)
            : Read(draw, r => (new[] { r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32() }, r.ReadUInt32()));
        Check("soul lottery: four different numbers of 1–30 and a prize (CS_SOULLOTTERY_ACK)", numbers.Length == 4 && numbers.Distinct().Count() == 4
            && numbers.All(n => n is >= 1 and <= 30) && prize > 0, Describe(draw));
        a.Discard(CS_MONEY_ACK);
        a.Send(Req(CS_SOULLOTTERY_TRIGGER_REQ, _ => { }));
        var trig = a.TryWait(CS_SOULLOTTERY_TRIGGER_ACK);
        var (drawn, paid, won) = trig is null ? (0u, 0u, false) : Read(trig, r => (r.ReadUInt32(), r.ReadUInt32(), r.ReadByte() != 0));
        Check("soul lottery: the number is drawn and paid: the prize on a match, else the level in silver (CS_SOULLOTTERY_TRIGGER_ACK)", trig is not null
            && won == numbers.Contains(drawn) && (won ? paid == prize : paid % 1000 == 0 && paid > 0), $"draw {drawn}, {paid}, won={won}, numbers {string.Join(",", numbers)}");
        Check("soul lottery: … the money comes (CS_MONEY_ACK)", a.TryWait(CS_MONEY_ACK) is not null, "no CS_MONEY_ACK");

        // The party loot lottery.
        a.Send(Req(CS_PARTYADD_REQ, w => { w.WriteString(b.Spawn.Name); w.WriteByte(0); }));
        b.TryWait(CS_PARTYJOINASK_ACK);
        b.Send(Req(CS_PARTYJOIN_REQ, w => { w.WriteString(a.Spawn.Name); w.WriteByte(0); w.WriteByte(0); }));
        a.TryWait(CS_PARTYJOIN_ACK); b.TryWait(CS_PARTYJOIN_ACK);
        a.Send(Req(CS_CHGPARTYTYPE_REQ, w => w.WriteByte(3)));
        var type = a.TryWait(CS_CHGPARTYTYPE_ACK);
        Check("loot lottery: A and B are a lottery party (CS_CHGPARTYTYPE_ACK)", type is not null && Read(type, r => (r.ReadByte(), r.ReadByte())) == (0, 3), Describe(type));
        b.TryWait(CS_CHGPARTYTYPE_ACK);
        Thread.Sleep(500);

        int drops = 0, kills = 0;
        while (drops < 2 && kills < 12)
        {
            uint mon = KillFieldMon(a);
            if (mon == 0) break;
            kills++;
            a.Discard(CS_MONITEMLIST_ACK);
            a.Send(Req(CS_MONITEMLIST_REQ, w => { w.WriteByte(1); w.WriteUInt32(mon); }));
            var list = a.TryWait(CS_MONITEMLIST_ACK, r => { r.ReadByte(); r.ReadByte(); return r.ReadUInt32() == mon; });
            var (ret, items) = list is null ? ((byte)255, new List<(byte, ushort)>()) : LootList(list);
            var loot = items.FirstOrDefault(i => i.Item2 == LotteryLoot);
            if (ret != 0 || loot.Item2 == 0) { a.Send(Req(CS_MONITEMLIST_REQ, w => { w.WriteByte(0); w.WriteUInt32(mon); })); continue; }
            drops++;
            string nth = drops == 1 ? "1st" : "2nd";

            a.Discard(CS_MONITEMTAKE_ACK); a.Discard(CS_GETITEM_ACK); b.Discard(CS_PARTYITEMTAKE_ACK); b.Discard(CS_MONITEMLOTTERY_ACK);
            a.Send(Req(CS_MONITEMTAKE_REQ, w => { w.WriteUInt32(mon); w.WriteByte(loot.Item1); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
            var ask = b.TryWait(CS_MONITEMLOTTERY_ACK);
            Check($"loot lottery ({nth}): A takes the item, B is asked (CS_MONITEMLOTTERY_ACK)", ask is not null
                && Read(ask, r => (r.ReadUInt32(), MiniItem(r).Id)) == (mon, LotteryLoot), Describe(ask));
            var after = a.TryWait(CS_MONITEMLIST_ACK, r => { r.ReadByte(); r.ReadByte(); return r.ReadUInt32() == mon; });
            Check($"loot lottery ({nth}): … the item leaves the loot list while drawn for", after is not null
                && LootList(after).Items.All(i => i.Item2 != LotteryLoot), Describe(after));
            var started = DateTime.UtcNow;
            if (drops == 1) b.Send(Req(CS_MONITEMLOTTERY_REQ, w => { w.WriteByte(0); w.WriteUInt32(mon); w.WriteUInt16(LotteryLoot); }));
            var gotIt = a.TryWait(CS_GETITEM_ACK, r => MiniItemWithSlot(r) == LotteryLoot, drops == 1 ? 5000 : 25000);
            double secs = (DateTime.UtcNow - started).TotalSeconds;
            Check(drops == 1 ? "loot lottery (1st): B says no, A gets it at once (CS_GETITEM_ACK)"
                    : "loot lottery (2nd): B says nothing, the corpse's AI draws after 15 s and A gets it",
                gotIt is not null && (drops == 1 ? secs < 3 : secs is > 12 and < 20), $"{Describe(gotIt)} after {secs:F1} s");
            Check($"loot lottery ({nth}): … B is told A took it (CS_PARTYITEMTAKE_ACK)",
                b.TryWait(CS_PARTYITEMTAKE_ACK, r => r.ReadUInt32() == a.CharId) is { } told && Read(told, r => { r.ReadUInt32(); return MiniItem(r).Id; }) == LotteryLoot,
                "no CS_PARTYITEMTAKE_ACK");
            a.Send(Req(CS_MONITEMLIST_REQ, w => { w.WriteByte(0); w.WriteUInt32(mon); }));
            a.Discard(CS_MONITEMTAKE_ACK); a.Discard(CS_MONITEMLIST_ACK);
        }
        Check("loot lottery: two drops in at most 12 kills", drops == 2, $"{drops} drop(s) in {kills} kill(s)");
        a.Send(Req(CS_PARTYDEL_REQ, w => w.WriteUInt32(b.CharId)));
        Thread.Sleep(500);
    }

    private static ushort MiniItemWithSlot(PacketReader r) { r.ReadByte(); return r.ReadUInt16(); }
}
