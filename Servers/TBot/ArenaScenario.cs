using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=arena</c> (needs <c>--Bot:Account3/4</c>): a 3-against-3 arena fight, two a side. The four bots stand by the arena
/// (map 0, NPC 26156); A and B make a party, C and D another. A signs its party up (50 000 copper), then C: the arena is ready, and a
/// minute later the battle starts. A hit before that does nothing (the arena is a peace zone until then). A and B bring C and D down:
/// C's side is sent out, A's side stays, and A gets the fee by mail. The info window shows A's side, the ranking answers, and A gives
/// up the waiting arena — its side is sent out and the fee comes back by mail. Positions, money, HP and the letters are put back.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_ARENA_REQ = M + 0x0250, CS_ARENA_ACK = M + 0x0251, CS_ARENATEAM_ACK = M + 0x0252, CS_ARENAINFO_REQ = M + 0x039D,
        CS_ARENAINFO_ACK = M + 0x039E, CS_ARENARANKING_REQ = M + 0x0540, CS_ARENARANKING_ACK = M + 0x0541;
    private const ushort ArenaNpc = 26156;

    private static (ushort Arena, byte Team, List<uint> Fighters) ArenaTeam(PacketReader p) => Read(p, r =>
    {
        ushort arena = r.ReadUInt16(); byte team = r.ReadByte();
        var ids = new List<uint>();
        for (byte n = r.ReadByte(), i = 0; i < n; i++) ids.Add(r.ReadUInt32());
        return (arena, team, ids);
    });

    private static void JoinParty(Bot chief, Bot member)
    {
        chief.Send(Req(CS_PARTYADD_REQ, w => { w.WriteString(member.Spawn.Name); w.WriteByte(0); }));
        member.TryWait(CS_PARTYJOINASK_ACK);
        member.Send(Req(CS_PARTYJOIN_REQ, w => { w.WriteString(chief.Spawn.Name); w.WriteByte(0); w.WriteByte(0); }));
        chief.TryWait(CS_PARTYJOIN_ACK); member.TryWait(CS_PARTYJOIN_ACK);
    }

    private static async Task<int> RunArenaAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        var ids = new List<uint>();
        foreach (var acc in new[] { cfg.Account, cfg.Account2, cfg.Account3, cfg.Account4 }) ids.Add(await FirstChar(db, acc));
        const string Cols = "SELECT wMapID, fPosX, fPosY, fPosZ, dwGold, dwSilver, dwCooper, dwHP, dwMP FROM TCHARTABLE WHERE dwCharID=@p0";
        var saved = new List<object[]?>();
        foreach (var id in ids) saved.Add(await db.RowAsync(Cols, (int)id));
        var maxPost = await db.RowAsync("SELECT ISNULL(MAX(dwPostID), 0) FROM TPOSTTABLE");
        int lastPost = Convert.ToInt32(maxPost![0]);
        try
        {
            for (int i = 0; i < 4; i++)
                await db.ExecAsync("UPDATE TCHARTABLE SET wMapID=0, fPosX=@p1, fPosY=86.1, fPosZ=345, dwGold=1, dwSilver=0, dwCooper=0 WHERE dwCharID=@p0",
                    (int)ids[i], 3805 + i);
            await ArenaFight(cfg, db, ids[0], lastPost);
        }
        finally
        {
            await Task.Delay(2500);
            await db.ExecAsync("DELETE FROM TPOSTTABLE WHERE dwCharID IN (@p0, @p1, @p2, @p3) AND dwPostID > @p4", (int)ids[0], (int)ids[1], (int)ids[2], (int)ids[3], lastPost);
            for (int i = 0; i < 4; i++)
                if (saved[i] is { } s)
                    await db.ExecAsync(@"UPDATE TCHARTABLE SET wMapID=@p1, fPosX=@p2, fPosY=@p3, fPosZ=@p4, dwGold=@p5, dwSilver=@p6, dwCooper=@p7, dwHP=@p8,
                        dwMP=@p9 WHERE dwCharID=@p0", (int)ids[i], s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] positions, money, HP and letters put back");
        }
        return PrintResults();
    }

    private static async Task ArenaFight(BotConfig cfg, GameDb db, uint idA, int lastPost)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        using var c = new Bot(Clone(cfg, cfg.Account3, "[C]"));
        using var d = new Bot(Clone(cfg, cfg.Account4, "[D]"));
        a.Enter(); b.Enter(); c.Enter(); d.Enter();
        Thread.Sleep(1500);
        JoinParty(a, b);
        JoinParty(c, d);
        Thread.Sleep(500);

        // Sign-ups.
        a.Discard(CS_ARENATEAM_ACK); a.Discard(CS_MONEY_ACK);
        a.Send(Req(CS_ARENA_REQ, w => { w.WriteByte(0); w.WriteUInt16(ArenaNpc); w.WriteUInt16(0); }));
        var teamA = a.TryWait(CS_ARENATEAM_ACK);
        var paid = a.TryWait(CS_MONEY_ACK);
        Check("arena: A's party is signed up as side 1, both members (CS_ARENATEAM_ACK)", teamA is not null
            && ArenaTeam(teamA) is var ta && ta.Arena == 1 && ta.Team == 1 && ta.Fighters.OrderBy(x => x).SequenceEqual(new[] { a.CharId, b.CharId }.OrderBy(x => x)),
            Describe(teamA));
        Check("arena: … the chief pays the fee (50 000)", paid is not null && MoneyOf(paid) == 1_000_000 - 50_000, Describe(paid));

        a.Discard(CS_SYSTEMMSG_ACK);
        c.Send(Req(CS_ARENA_REQ, w => { w.WriteByte(0); w.WriteUInt16(ArenaNpc); w.WriteUInt16(0); }));
        var teamC = c.TryWait(CS_ARENATEAM_ACK, r => { r.ReadUInt16(); return r.ReadByte() == 2; });
        var ready = a.TryWait(CS_SYSTEMMSG_ACK, r => r.ReadByte() == 28);
        Check("arena: C's party takes side 2 (CS_ARENATEAM_ACK)", teamC is not null && ArenaTeam(teamC).Fighters.Count == 2, Describe(teamC));
        Check("arena: … the arena is ready, 60 s to the battle (SM_ARENA_COUNTDOWN)", ready is not null
            && Read(ready, r => (r.ReadByte(), r.ReadUInt16(), r.ReadUInt32(), r.ReadByte(), r.ReadByte())) is var rd && rd.Item3 == 60 && rd.Item4 == 3,
            Describe(ready));

        // Before the battle: a peace zone.
        a.Discard(CS_DEFEND_ACK);
        a.Send(FinishSkill(a, a.CharId, 1, BasicMelee, a.Spawn.X, a.Spawn.Z, (c.CharId, 1)));
        var early = a.TryWait(CS_DEFEND_ACK, r => { r.ReadUInt32(); return r.ReadUInt32() == c.CharId; }, 1500);
        Check("arena: no hit lands before the battle (a peace zone)", early is null, Describe(early));

        var battle = a.TryWait(CS_SYSTEMMSG_ACK, r => r.ReadByte() == 28 && r.ReadUInt16() == 1 && r.ReadUInt32() == 240, 70_000);
        Check("arena: a minute later the battle starts, 4 minutes (SM_ARENA_COUNTDOWN)", battle is not null, Describe(battle));

        // A and B bring C and D down.
        c.Discard(CS_TELEPORT_ACK); d.Discard(CS_TELEPORT_ACK); a.Discard(CS_ARENATEAM_ACK); a.Discard(CS_POSTRECV_ACK);
        foreach (var target in new[] { c, d })
        {
            PacketReader? died = null;
            for (int i = 0; i < 600 && died is null; i++)
            {
                a.Send(FinishSkill(a, a.CharId, 1, BasicMelee, a.Spawn.X, a.Spawn.Z, (target.CharId, 1)));
                b.Send(FinishSkill(b, b.CharId, 1, BasicMelee, b.Spawn.X, b.Spawn.Z, (target.CharId, 1)));
                died = a.TryWait(CS_DIE_ACK, r => r.ReadUInt32() == target.CharId, 60);
            }
            Check($"arena: {(target == c ? "C" : "D")} is brought down", died is not null, "still standing");
        }
        var outC = a.TryWait(CS_ARENATEAM_ACK, r => { r.ReadUInt16(); return r.ReadByte() == 0; }, 3000);
        Check("arena: C's side is out (CS_ARENATEAM_ACK side 0) and sent outside (CS_TELEPORT_ACK)", outC is not null
            && ArenaTeam(outC).Fighters.Count == 2 && c.TryWait(CS_TELEPORT_ACK, timeoutMs: 2000) is not null, Describe(outC));
        Check("arena: … A, the winner, gets the fee by mail (CS_POSTRECV_ACK)", a.TryWait(CS_POSTRECV_ACK, timeoutMs: 3000) is not null, "no letter");

        // The windows, then A gives up.
        a.Send(Req(CS_ARENAINFO_REQ, w => w.WriteByte(1)));
        var info = a.TryWait(CS_ARENAINFO_ACK);
        Check("arena: the info window shows A's side holding the arena (CS_ARENAINFO_ACK)", info is not null && Read(info, r =>
        {
            uint first = r.ReadUInt32(), second = r.ReadUInt32();
            string name = r.ReadString(); r.ReadByte(); r.ReadByte(); r.ReadUInt32(); byte wins = r.ReadByte();
            byte queued = r.ReadByte();
            return (first, second, name, wins, queued);
        }) == (1u, 0u, a.Spawn.Name, (byte)1, (byte)0), Describe(info));
        a.Send(Req(CS_ARENARANKING_REQ, w => w.WriteByte(1)));
        var rank = a.TryWait(CS_ARENARANKING_ACK);
        Check("arena: the ranking window answers (CS_ARENARANKING_ACK)", rank is not null && Read(rank, r =>
        {
            byte n1 = r.ReadByte(), max1 = r.ReadByte(), n2 = r.ReadByte(), max2 = r.ReadByte();
            for (int i = 0; i < n1 + n2; i++) { r.ReadString(); r.ReadByte(); r.ReadByte(); }
            return (n1, max1);
        }) is var rk && rk.n1 == 0, Describe(rank));

        a.Discard(CS_ARENATEAM_ACK); a.Discard(CS_POSTRECV_ACK);
        a.Send(Req(CS_ARENA_REQ, w => { w.WriteByte(1); w.WriteUInt16(ArenaNpc); w.WriteUInt16(1); }));
        var outA = a.TryWait(CS_ARENATEAM_ACK, r => { r.ReadUInt16(); return r.ReadByte() == 0; });
        Check("arena: A gives up the waiting arena: its side is out (CS_ARENATEAM_ACK side 0)", outA is not null && ArenaTeam(outA).Fighters.Contains(a.CharId),
            Describe(outA));
        Check("arena: … and the fee comes back by mail (CS_POSTRECV_ACK)", a.TryWait(CS_POSTRECV_ACK, timeoutMs: 3000) is not null, "no letter");
        a.Send(Req(CS_PARTYDEL_REQ, w => w.WriteUInt32(b.CharId)));
        c.Send(Req(CS_PARTYDEL_REQ, w => w.WriteUInt32(d.CharId)));
        Thread.Sleep(500);
        foreach (var bot in new[] { a, b, c, d }) bot.Dispose();

        await Task.Delay(2500);
        var letters = await Letters(db, idA, lastPost);
        Check("arena: A's letters are the win and the refund, 50 silver each (TPOSTTABLE)", letters.Count == 2 && letters.All(l => l.Money == 50_000),
            string.Join("; ", letters));
    }
}
