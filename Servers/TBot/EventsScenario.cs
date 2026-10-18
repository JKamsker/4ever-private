using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=events</c>: the operator's events, pushed to the world as the control tool does (<see cref="WorldTool"/>).
/// A (set to level 5) stands by spawn 4200 (map 0) and kills a monster 770 for its plain exp; a 100% exp event doubles the next
/// kill's exp, and the exp is plain again once it ends. A monster-regen event places a 770 next to A, and its end kills it. An event
/// message reaches A as a world chat line, and the lucky-ticket draw is shown. A's level, exp and position are put back.
/// </summary>
public static partial class Scenarios
{
    private const byte EventExpAdd = 1, EventMonRegen = 13;

    private static async Task<int> RunEventsAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account);
        var save = await db.RowAsync("SELECT bLevel, dwEXP, wSkillPoint, wMapID, fPosX, fPosY, fPosZ FROM TCHARTABLE WHERE dwCharID=@p0", (int)idA);
        var exp5 = await db.RowAsync("SELECT dwEXP FROM TLEVELCHART WHERE bLevel=4");
        try
        {
            await db.ExecAsync("UPDATE TCHARTABLE SET bLevel=5, dwEXP=@p1, wMapID=0, fPosX=3716, fPosY=87.23, fPosZ=548 WHERE dwCharID=@p0",
                (int)idA, exp5![0]);
            Events(cfg);
        }
        finally
        {
            WorldTool.EventUpdate(EventExpAdd, 0, 900_001);
            WorldTool.EventUpdate(EventMonRegen, 0, 900_002);
            await Task.Delay(2500);
            if (save is not null)
                await db.ExecAsync("UPDATE TCHARTABLE SET bLevel=@p1, dwEXP=@p2, wSkillPoint=@p3, wMapID=@p4, fPosX=@p5, fPosY=@p6, fPosZ=@p7 WHERE dwCharID=@p0",
                    (int)idA, save[0], save[1], save[2], save[3], save[4], save[5], save[6]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] events ended, A's level, exp and position put back");
        }
        return PrintResults();
    }

    /// <summary>The exp one kill of a 770 gives A (from the CS_EXP_ACK totals around it), or −1.</summary>
    private static long KillForExp(Bot a)
    {
        byte[][] seen;
        lock (a.Received) seen = a.Received.ToArray();
        var before = seen.LastOrDefault(p => PacketHeader.ReadId(p) == CS_EXP_ACK);
        long start = before is null ? -1 : BitConverter.ToUInt32(before, PacketHeader.Size);
        a.Discard(CS_EXP_ACK);
        if (KillFieldMon(a) == 0) return -1;
        var after = a.TryWait(CS_EXP_ACK, timeoutMs: 2000);
        if (after is null) return -1;
        long end = after.ReadUInt32();
        return start < 0 ? -1 : end - start;
    }

    private static void Events(BotConfig cfg)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        a.Enter();
        Thread.Sleep(2000);

        // Exp: plain, doubled under the event, plain again after it.
        KillForExp(a);                                                 // the first kill sets the CS_EXP_ACK baseline
        long plain = KillForExp(a);
        WorldTool.EventUpdate(EventExpAdd, 100, 900_001);
        long doubled = KillForExp(a);
        WorldTool.EventUpdate(EventExpAdd, 0, 900_001);
        long after = KillForExp(a);
        Check("events: a 100% exp event doubles a kill's exp", plain > 0 && Math.Abs(doubled - 2 * plain) <= 2, $"plain {plain}, under the event {doubled}");
        Check("events: … and the exp is plain again once it ends", plain > 0 && Math.Abs(after - plain) <= 1, $"after {after}");

        // A monster-regen event places a 770 by A, and its end kills it.
        var known = new HashSet<uint>(LivingMonsters(a, FieldMon));
        WorldTool.EventUpdate(EventMonRegen, 1, 900_002, 0, new[] { (FieldMon, 0u, (ushort)0, a.Spawn.X + 2, a.Spawn.Y, a.Spawn.Z + 2) });
        Thread.Sleep(1000);
        var placed = LivingMonsters(a, FieldMon).Where(m => !known.Contains(m)).ToList();
        Check("events: a monster-regen event places a monster by A (CS_ADDMON_ACK)", placed.Count >= 1, $"{placed.Count} new 770(s)");
        a.Discard(CS_DIE_ACK); a.Discard(CS_DELMON_ACK);
        WorldTool.EventUpdate(EventMonRegen, 0, 900_002);
        var gone = placed.Count == 0 ? null
            : a.TryWait(CS_DIE_ACK, r => placed.Contains(r.ReadUInt32()), 3000) ?? a.TryWait(CS_DELMON_ACK, r => placed.Contains(r.ReadUInt32()), 1000);
        Check("events: … and its end takes it away (CS_DIE_ACK)", gone is not null, Describe(gone));

        // The event message and the lucky-ticket draw.
        a.Discard(CS_CHAT_ACK);
        WorldTool.EventMsg("TBot: double exp tonight!");
        var chat = a.TryWait(CS_CHAT_ACK, r => r.ReadByte() == 3);
        Check("events: an event message is a world chat line from the operator (CS_CHAT_ACK)", chat is not null
            && Read(chat, r => { r.ReadByte(); uint from = r.ReadUInt32(); string name = r.ReadString(); return (from, name.Length > 0, r.ReadString().EndsWith("double exp tonight!")); })
               == (0u, true, true), Describe(chat));
        a.Discard(CS_SYSTEMMSG_ACK);
        WorldTool.EventQuarter(1, 21, 30, "TBot cake");
        var draw = a.TryWait(CS_SYSTEMMSG_ACK, r => r.ReadByte() == 9);
        Check("events: the lucky-ticket draw is shown (SM_EVENT_QUARTER)", draw is not null
            && Read(draw, r => (r.ReadByte(), r.ReadByte(), r.ReadByte(), r.ReadString(), r.ReadByte() < 100)) == (9, 21, 30, "TBot cake", true), Describe(draw));
    }
}
