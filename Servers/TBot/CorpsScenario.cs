using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=corps</c> (needs <c>--Bot:Account3/4</c>): four bots make two parties — A with B, C with D — and the chiefs
/// form a corps under A's party. A gives C's squad an order (all four see it), C reports its HP (its party and A, the other chief,
/// see it), A hands command to C's party, and C, the new general, takes A's party out, which ends the corps. Then member recall:
/// D's answer to a recall meant for B is ignored, B's yes brings B to A (A's Evocation Scroll used); C's Scroll of Teleportation
/// takes C to A. The scrolls and B's and C's positions are put back.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_CORPSASK_REQ = M + 0x00AC, CS_CORPSASK_ACK = M + 0x00AD,
        CS_CORPSREPLY_REQ = M + 0x00AE, CS_CORPSLEAVE_REQ = M + 0x00B0, CS_CORPSJOIN_ACK = M + 0x00B1, CS_CORPSCMD_REQ = M + 0x00B2,
        CS_CORPSCMD_ACK = M + 0x00B3, CS_ADDSQUAD_ACK = M + 0x00C0, CS_DELSQUAD_ACK = M + 0x00C1, CS_CORPSHP_REQ = M + 0x00FD,
        CS_CORPSHP_ACK = M + 0x00FE, CS_CHGCORPSCOMMANDER_REQ = M + 0x0103, CS_CHGCORPSCOMMANDER_ACK = M + 0x0104,
        CS_PARTYMEMBERRECALL_REQ = M + 0x0190, CS_PARTYMEMBERRECALL_ACK = M + 0x0191, CS_PARTYMEMBERRECALLANS_REQ = M + 0x01B5,
        CS_PARTYMEMBERRECALLANS_ACK = M + 0x01B6;
    private const ushort EvocationScroll = 7604, TeleportScroll = 7603;   // IK_MEMBERRECALL, IK_MOVETOCHAR
    private const byte EvocationSlot = 44, TeleportSlot = 45;
    private const long EvocationDlId = 900_000_011, TeleportDlId = 900_000_012;

    private static async Task<int> RunCorpsAsync(BotConfig cfg)
    {
        if (cfg.Account3.Length == 0 || cfg.Account4.Length == 0) { Console.Error.WriteLine("the corps scenario needs --Bot:Account3 and --Bot:Account4"); return 2; }
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account), idB = await FirstChar(db, cfg.Account2);
        uint idC = await FirstChar(db, cfg.Account3), idD = await FirstChar(db, cfg.Account4);
        var posA = await db.RowAsync("SELECT wMapID, fPosX, fPosY, fPosZ FROM TCHARTABLE WHERE dwCharID=@p0", (int)idA);
        var saved = new Dictionary<uint, object[]?>();
        foreach (uint id in new[] { idB, idC }) saved[id] = await db.RowAsync("SELECT wMapID, fPosX, fPosY, fPosZ FROM TCHARTABLE WHERE dwCharID=@p0", (int)id);
        try
        {
            // B and C start 20 away from A on its map, so a recall really moves them.
            foreach (uint id in new[] { idB, idC })
                await db.ExecAsync("UPDATE TCHARTABLE SET wMapID=@p1, fPosX=@p2 + 20, fPosY=@p3, fPosZ=@p4 WHERE dwCharID=@p0", (int)id, posA![0], posA[1], posA[2], posA[3]);
            await ClearScrolls(db, idA, idC);
            await InsertScroll(db, EvocationDlId, idA, EvocationSlot, EvocationScroll, 2);
            await InsertScroll(db, TeleportDlId, idC, TeleportSlot, TeleportScroll, 1);
            CorpsLife(cfg);
        }
        finally
        {
            await Task.Delay(2500);                                              // the logout saves land first
            await ClearScrolls(db, idA, idC);
            foreach (var (id, row) in saved)
                if (row is not null)
                    await db.ExecAsync("UPDATE TCHARTABLE SET wMapID=@p1, fPosX=@p2, fPosY=@p3, fPosZ=@p4 WHERE dwCharID=@p0", (int)id, row[0], row[1], row[2], row[3]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] scrolls removed, B's and C's positions restored");
        }
        return PrintResults();
    }

    private static Task ClearScrolls(GameDb db, uint idA, uint idC)
        => db.ExecAsync("DELETE FROM TITEMTABLE WHERE (dwOwnerID IN (@p0, @p1) AND bOwnerType=0 AND wItemID IN (@p2, @p3)) OR dlID IN (@p4, @p5)",
            (int)idA, (int)idC, (int)EvocationScroll, (int)TeleportScroll, EvocationDlId, TeleportDlId);

    private static Task InsertScroll(GameDb db, long dlId, uint owner, byte slot, ushort item, byte count)
        => db.ExecAsync(@"INSERT INTO TITEMTABLE (dlID, bStorageType, dwStorageID, bOwnerType, dwOwnerID, bItemID, wItemID, bLevel,
            bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime, bGradeEffect, bMagic1, bMagic2, bMagic3, bMagic4, bMagic5,
            bMagic6, wValue1, wValue2, wValue3, wValue4, wValue5, wValue6, dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6,
            bGem, wMoggItemID)
            VALUES (@p0, 0, 255, 0, @p1, @p2, @p3, 0, @p4, 0, 0, 0, 0, '1900-01-01', 0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0, 0)",
            dlId, (int)owner, (int)slot, (int)item, (int)count);

    /// <summary>A CS_ADDSQUAD_ACK read whole: (chief, squad, member ids).</summary>
    private static (uint Chief, ushort Squad, List<uint> Members) Squad(PacketReader p) => Read(p, r =>
    {
        uint chief = r.ReadUInt32(); ushort squad = r.ReadUInt16();
        var ids = new List<uint>();
        for (byte n = r.ReadByte(), i = 0; i < n; i++)
        {
            ids.Add(r.ReadUInt32()); r.ReadString(); r.ReadFloat(); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32();
            for (int k = 0; k < 6; k++) r.ReadUInt16();
            for (int k = 0; k < 8; k++) r.ReadByte();
        }
        return (chief, squad, ids);
    });

    private static (ushort Corps, ushort Commander) CorpsJoin(PacketReader p) => Read(p, r => (r.ReadUInt16(), r.ReadUInt16()));

    private static (byte Result, uint Id, ushort Map, float X, float Z) TeleportAck(PacketReader p)
        => Read(p, r => { var x = (r.ReadByte(), r.ReadUInt32()); r.ReadByte(); r.ReadUInt32(); var m = r.ReadUInt16(); float px = r.ReadFloat(); r.ReadFloat(); return (x.Item1, x.Item2, m, px, r.ReadFloat()); });

    /// <summary>The chief invites the member; returns the party id the member was told.</summary>
    private static ushort MakeParty(Bot chief, Bot member, string label)
    {
        chief.Send(Req(CS_PARTYADD_REQ, w => { w.WriteString(member.Spawn.Name); w.WriteByte(0); }));
        member.TryWait(CS_PARTYJOINASK_ACK);
        member.Send(Req(CS_PARTYJOIN_REQ, w => { w.WriteString(chief.Spawn.Name); w.WriteByte(0); w.WriteByte(0); }));
        var join = member.TryWait(CS_PARTYJOIN_ACK);
        var j = join is null ? null : ParseJoin(join);
        chief.TryWait(CS_PARTYJOIN_ACK);
        var squad = member.TryWait(CS_ADDSQUAD_ACK, r => { r.ReadUInt32(); r.ReadUInt16(); return r.ReadByte() == 2; });
        var sq = squad is null ? default : Squad(squad);
        Check($"corps: {label} — the new member is shown its party as a squad of two (CS_ADDSQUAD_ACK)", j is not null && squad is not null
            && sq.Chief == chief.CharId && sq.Squad == j.Value.PartyId && sq.Members.OrderBy(x => x).SequenceEqual(new[] { chief.CharId, member.CharId }.OrderBy(x => x)),
            $"{j} / {Describe(squad)}");
        return j?.PartyId ?? 0;
    }

    private static void CorpsLife(BotConfig cfg)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        using var c = new Bot(Clone(cfg, cfg.Account3, "[C]"));
        using var d = new Bot(Clone(cfg, cfg.Account4, "[D]"));
        a.Enter(); b.Enter(); c.Enter(); d.Enter();
        Thread.Sleep(1500);
        string nameA = a.Spawn.Name, nameB = b.Spawn.Name, nameC = c.Spawn.Name;

        ushort p1 = MakeParty(a, b, "A + B");
        ushort p2 = MakeParty(c, d, "C + D");
        foreach (var bot in new[] { a, b, c, d }) { bot.Discard(CS_ADDSQUAD_ACK); bot.Discard(CS_CORPSJOIN_ACK); }

        // Forming.
        a.Send(Req(CS_CORPSASK_REQ, w => w.WriteString(nameC)));
        var ask = c.TryWait(CS_CORPSASK_ACK);
        Check("corps: C, the other chief, is asked (CS_CORPSASK_ACK)", ask is not null && Read(ask, r => r.ReadString()) == nameA, Describe(ask));
        c.Send(Req(CS_CORPSREPLY_REQ, w => { w.WriteString(nameA); w.WriteByte(0); }));
        var other = a.TryWait(CS_ADDSQUAD_ACK, r => { r.ReadUInt32(); return r.ReadUInt16() == p2; });
        Check("corps: A is shown C's squad (CS_ADDSQUAD_ACK)", other is not null && Squad(other) is var s2 && s2.Chief == c.CharId && s2.Members.Count == 2, Describe(other));
        var joinD = d.TryWait(CS_CORPSJOIN_ACK);
        var cj = joinD is null ? default : CorpsJoin(joinD);
        Check("corps: D is told its corps, under A's party (CS_CORPSJOIN_ACK)", joinD is not null && cj.Corps != 0 && cj.Commander == p1, Describe(joinD));

        // An order to C's squad.
        foreach (var bot in new[] { a, b, c, d }) bot.Discard(CS_CORPSCMD_ACK);
        a.Send(Req(CS_CORPSCMD_REQ, w => { w.WriteUInt16(p2); w.WriteUInt32(c.CharId); w.WriteUInt16(0); w.WriteByte(1); w.WriteUInt32(0); w.WriteByte(0); w.WriteUInt16(10); w.WriteUInt16(20); }));
        var orders = new[] { a, b, c, d }.Select(bot => bot.TryWait(CS_CORPSCMD_ACK)).ToList();
        Check("corps: A's order reaches all four (CS_CORPSCMD_ACK)", orders.All(o => o is not null
            && Read(o, r => (r.ReadUInt16(), r.ReadUInt32(), r.ReadUInt16(), r.ReadByte(), r.ReadUInt32(), r.ReadByte(), r.ReadUInt16(), r.ReadUInt16()))
               == (p2, c.CharId, 0, 1, 0u, 0, 10, 20)), string.Join(" / ", orders.Select(Describe)));

        // A report from a chief.
        c.Send(Req(CS_CORPSHP_REQ, w => { w.WriteUInt16(p2); w.WriteUInt32(c.CharId); w.WriteUInt32(100); w.WriteUInt32(50); }));
        var hpA = a.TryWait(CS_CORPSHP_ACK);
        var hpB = b.TryWait(CS_CORPSHP_ACK, timeoutMs: 800);
        Check("corps: C's HP reaches A, the other chief, not A's member B (CS_CORPSHP_ACK)", hpA is not null && hpB is null
            && Read(hpA, r => (r.ReadUInt16(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32())) == (p2, c.CharId, 100u, 50u), Describe(hpA));
        d.TryWait(CS_CORPSHP_ACK); c.TryWait(CS_CORPSHP_ACK);

        // Command to C's party; C takes A's party out — the corps ends.
        foreach (var bot in new[] { a, b, c, d }) bot.Discard(CS_CORPSJOIN_ACK);
        a.Send(Req(CS_CHGCORPSCOMMANDER_REQ, w => w.WriteUInt16(p2)));
        var chg = a.TryWait(CS_CHGCORPSCOMMANDER_ACK);
        var joinB = b.TryWait(CS_CORPSJOIN_ACK);
        Check("corps: A hands command to C's party (CORPS_CHGCOMMANDER; B is told the new commander)", chg is not null && Read(chg, r => r.ReadByte()) == 8
            && joinB is not null && CorpsJoin(joinB).Commander == p2, $"{Describe(chg)} / {Describe(joinB)}");
        foreach (var bot in new[] { a, b, c, d }) bot.Discard(CS_CORPSJOIN_ACK);
        c.Send(Req(CS_CORPSLEAVE_REQ, w => w.WriteUInt16(p1)));
        var gone = a.TryWait(CS_DELSQUAD_ACK);
        var endA = a.TryWait(CS_CORPSJOIN_ACK);
        var endD = d.TryWait(CS_CORPSJOIN_ACK);
        Check("corps: C, the new general, takes A's party out and the corps ends (DELSQUAD, CORPSJOIN 0)", gone is not null && Read(gone, r => r.ReadUInt16()) == p2
            && endA is not null && CorpsJoin(endA) == (0, 0) && endD is not null && CorpsJoin(endD).Corps == 0, $"{Describe(gone)} / {Describe(endA)} / {Describe(endD)}");

        // Member recall: D's forged answer is ignored; B's yes brings B to A.
        a.Send(Req(CS_PARTYMEMBERRECALL_REQ, w => { w.WriteByte(0xFF); w.WriteByte(EvocationSlot); w.WriteString(nameB); }));
        var asked = b.TryWait(CS_PARTYMEMBERRECALLANS_ACK);
        Check("recall: B is asked to come (CS_PARTYMEMBERRECALLANS_ACK)", asked is not null
            && Read(asked, r => (r.ReadString(), r.ReadByte(), r.ReadByte())) == (nameA, 0xFF, EvocationSlot), Describe(asked));
        d.Send(Req(CS_PARTYMEMBERRECALLANS_REQ, w => { w.WriteByte(0); w.WriteString(nameA); w.WriteByte(0xFF); w.WriteByte(EvocationSlot); }));
        var forged = d.TryWait(CS_TELEPORT_ACK, timeoutMs: 1500);
        Check("recall: D, who was not asked, can't answer for B (fixed C++ bug)", forged is null && a.TryWait(CS_PARTYMEMBERRECALL_ACK, timeoutMs: 300) is null, Describe(forged));
        b.Send(Req(CS_PARTYMEMBERRECALLANS_REQ, w => { w.WriteByte(0); w.WriteString(nameA); w.WriteByte(0xFF); w.WriteByte(EvocationSlot); }));
        var done = a.TryWait(CS_PARTYMEMBERRECALL_ACK);
        var moved = b.TryWait(CS_TELEPORT_ACK);
        var tb = moved is null ? default : TeleportAck(moved);
        Check("recall: B says yes — A's scroll is used and B lands where A stands", done is not null
            && Read(done, r => (r.ReadByte(), r.ReadByte(), r.ReadString())) == (0, 0, nameB) && moved is not null
            && tb.Result == 0 && Math.Abs(tb.X - a.Spawn.X) < 1 && Math.Abs(tb.Z - a.Spawn.Z) < 1, $"{Describe(done)} / {Describe(moved)}");
        var left = a.TryWait(CS_UPDATEITEM_ACK);
        Check("recall: … one Evocation Scroll left", left is not null, Describe(left));

        // Going to someone: no question.
        c.Send(Req(CS_PARTYMEMBERRECALL_REQ, w => { w.WriteByte(0xFF); w.WriteByte(TeleportSlot); w.WriteString(nameA); }));
        var went = c.TryWait(CS_TELEPORT_ACK);
        var tc = went is null ? default : TeleportAck(went);
        var used = c.TryWait(CS_PARTYMEMBERRECALL_ACK);
        Check("teleport scroll: C goes to A at once and the scroll is used up", went is not null && tc.Result == 0 && Math.Abs(tc.X - a.Spawn.X) < 1
            && used is not null && Read(used, r => (r.ReadByte(), r.ReadByte(), r.ReadString())) == (0, 1, nameA)
            && c.TryWait(CS_DELITEM_ACK) is not null, $"{Describe(went)} / {Describe(used)}");
    }
}
