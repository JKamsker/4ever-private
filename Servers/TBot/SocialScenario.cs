using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=social</c>: friends, the messenger and the protected (block) list, end to end with the live world. A asks B
/// to be friends, puts B in a group, renames and deletes it; they talk in the messenger, B leaves and is brought back by A's next
/// line; then B blocks A — the friendship breaks, A's whisper, trade, duel and party invite are refused, the list comes back on B's
/// next login and A no longer sees B come online — and B takes A off the list again. The bots' friend, group and protected rows are
/// copied aside before the run and put back after.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_CHAT_REQ = M + 0x0074, CS_CHAT_ACK = M + 0x0075, CS_DEALITEMASK_REQ = M + 0x010C, CS_DEALITEMASK_ACK = M + 0x010D,
        CS_DEALITEMEND_ACK = M + 0x0113, CS_FRIENDASK_REQ = M + 0x008E, CS_FRIENDASK_ACK = M + 0x008F, CS_FRIENDREPLY_REQ = M + 0x0090,
        CS_FRIENDADD_ACK = M + 0x0091, CS_FRIENDERASE_ACK = M + 0x0093, CS_FRIENDLIST_ACK = M + 0x0094,
        CS_FRIENDCONNECTION_ACK = M + 0x0095, CS_FRIENDGROUPMAKE_REQ = M + 0x00C2, CS_FRIENDGROUPMAKE_ACK = M + 0x00C3,
        CS_FRIENDGROUPDELETE_REQ = M + 0x00C4, CS_FRIENDGROUPDELETE_ACK = M + 0x00C5, CS_FRIENDGROUPCHANGE_REQ = M + 0x00C6,
        CS_FRIENDGROUPCHANGE_ACK = M + 0x00C7, CS_FRIENDGROUPNAME_REQ = M + 0x00C8, CS_FRIENDGROUPNAME_ACK = M + 0x00C9,
        CS_TMSSEND_REQ = M + 0x00CA, CS_TMSRECV_ACK = M + 0x00CB, CS_TMSINVITE_REQ = M + 0x00CC, CS_TMSINVITE_ACK = M + 0x00CD,
        CS_TMSOUT_REQ = M + 0x00D0, CS_TMSOUT_ACK = M + 0x00D1, CS_PROTECTEDLIST_ACK = M + 0x00F6, CS_PROTECTEDADD_REQ = M + 0x00F7,
        CS_PROTECTEDADD_ACK = M + 0x00F8, CS_PROTECTEDERASE_REQ = M + 0x00F9, CS_PROTECTEDERASE_ACK = M + 0x00FA,
        CS_PROTECTEDOPTION_REQ = M + 0x01F2, CS_FRIENDLIST_REQ = M + 0x01F3, CS_SOULMATESEARCH_REQ = M + 0x019C,
        CS_SOULMATESEARCH_ACK = M + 0x019D, CS_SOULMATEEND_REQ = M + 0x01A0, CS_SOULMATEEND_ACK = M + 0x01A1,
        CS_SOULMATEREGREADY_REQ = M + 0x01A2, CS_SOULMATEREGREADY_ACK = M + 0x01A3;
    private const uint SoulSearchFee19 = 2002;                                // TLEVELCHART dwSearchCost at level 19
    private const byte BlockAll = 1 | 2 | 4 | 8;                              // PROTECTED_CHAT | INVITE | DEAL | DUEL
    private static readonly string[] SocialTables = { "TFRIENDTABLE", "TFRIENDGROUPTABLE", "TPROTECTEDTABLE", "TSOULMATETABLE" };

    private static async Task<int> RunSocialAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account), idB = await FirstChar(db, cfg.Account2);
        string Where(string t) => t switch
        {
            "TFRIENDTABLE" => "(dwCharID IN (@p0, @p1) OR dwFriendID IN (@p0, @p1))",
            "TSOULMATETABLE" => "(dwCharID IN (@p0, @p1) OR dwTarget IN (@p0, @p1))",
            _ => "dwCharID IN (@p0, @p1)",
        };
        var money = await db.RowAsync("SELECT dwGold, dwSilver, dwCooper FROM TCHARTABLE WHERE dwCharID=@p0", (int)idA);
        await db.ExecAsync("UPDATE TCHARTABLE SET dwGold=0, dwSilver=10, dwCooper=0 WHERE dwCharID=@p0", (int)idA);
        foreach (var t in SocialTables)                                     // kept if a crashed run left one: it holds the original
        {
            await db.ExecAsync($"IF OBJECT_ID('TBOT_BAK_{t}') IS NULL SELECT * INTO TBOT_BAK_{t} FROM {t} WHERE {Where(t)}", (int)idA, (int)idB);
            await db.ExecAsync($"DELETE FROM {t} WHERE {Where(t)}", (int)idA, (int)idB);
        }
        try
        {
            await SocialLife(cfg, db, idA, idB);
        }
        finally
        {
            await Task.Delay(2500);                                              // the logout saves land first
            foreach (var t in SocialTables)
            {
                await db.ExecAsync($"DELETE FROM {t} WHERE {Where(t)}", (int)idA, (int)idB);
                await db.ExecAsync($"INSERT INTO {t} SELECT * FROM TBOT_BAK_{t}; DROP TABLE TBOT_BAK_{t}");
            }
            if (money is not null)
                await db.ExecAsync("UPDATE TCHARTABLE SET dwGold=@p1, dwSilver=@p2, dwCooper=@p3 WHERE dwCharID=@p0", (int)idA, money[0], money[1], money[2]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] friend, group, protected and soulmate rows and A's money restored");
        }
        return PrintResults();
    }

    /// <summary>A CS_FRIENDLIST_ACK read: the groups, then the friends (id, group, online).</summary>
    private static (List<(byte Id, string Name)> Groups, List<(uint Id, byte Group, bool On)> Friends) FriendList(PacketReader p) => Read(p, r =>
    {
        if (r.ReadUInt32() != 0) { r.ReadString(); r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadUInt32(); }   // the soulmate
        var groups = new List<(byte, string)>();
        for (byte n = r.ReadByte(), i = 0; i < n; i++) groups.Add((r.ReadByte(), r.ReadString()));
        var friends = new List<(uint, byte, bool)>();
        for (byte n = r.ReadByte(), i = 0; i < n; i++)
        {
            uint id = r.ReadUInt32(); r.ReadString(); r.ReadByte(); byte g = r.ReadByte(); r.ReadByte(); bool on = r.ReadByte() != 0; r.ReadUInt32();
            friends.Add((id, g, on));
        }
        return (groups, friends);
    });

    private static (List<(byte Id, string Name)> Groups, List<(uint Id, byte Group, bool On)> Friends)? AskFriendList(Bot a)
    {
        a.Discard(CS_FRIENDLIST_ACK);
        a.Send(Req(CS_FRIENDLIST_REQ, _ => { }));
        var p = a.TryWait(CS_FRIENDLIST_ACK);
        return p is null ? null : FriendList(p);
    }

    /// <summary>A CS_FRIENDADD_ACK read whole: (result, friend id, name).</summary>
    private static (byte Result, uint Id, string Name) FriendAdd(PacketReader p)
        => Read(p, r => { var x = (r.ReadByte(), r.ReadUInt32(), r.ReadString()); r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadUInt32(); return x; });

    /// <summary>A CS_TMSINVITE_ACK read whole: (inviter, conversation, member count).</summary>
    private static (uint Inviter, uint Tms, byte Count) TmsInvite(PacketReader p) => Read(p, r =>
    {
        var x = (r.ReadUInt32(), r.ReadUInt32(), r.ReadByte());
        for (int i = 0; i < x.Item3; i++) { r.ReadUInt32(); r.ReadString(); r.ReadByte(); r.ReadByte(); }
        return x;
    });

    /// <summary>A searches (B, the only one of its country online within 10 levels, is found and saved), can't choose B again,
    /// ends the bond and must then wait a day.</summary>
    private static async Task Soulmates(Bot a, GameDb db, uint idA, uint idB, string nameB)
    {
        a.Send(Req(CS_SOULMATESEARCH_REQ, w => { w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        var found = a.TryWait(CS_SOULMATESEARCH_ACK);
        var f = found is null ? default : Read(found, r => (r.ReadByte(), r.ReadUInt32(), r.ReadString(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32()));
        long left = (long)f.Item5 * 1_000_000 + f.Item6 * 1000L + f.Item7;
        Check("soulmates: A's search finds B and costs the level's fee (CS_SOULMATESEARCH_ACK)", found is not null
            && (f.Item1, f.Item2, f.Item3) == (0, idB, nameB) && left == 10_000 - SoulSearchFee19, Describe(found));
        await Task.Delay(500);
        var row = await db.RowAsync("SELECT dwTarget, dwTime FROM TSOULMATETABLE WHERE dwCharID=@p0", (int)idA);
        Check("soulmates: the bond found is saved in TSOULMATETABLE (fixed: only one chosen by name was)", row is not null
            && Convert.ToUInt32(row[0]) == idB && Convert.ToInt64(row[1]) == 0, row is null ? "no row" : $"target={row[0]} time={row[1]}");
        a.Send(Req(CS_SOULMATEREGREADY_REQ, w => { w.WriteString(nameB); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        var again = a.TryWait(CS_SOULMATEREGREADY_ACK);
        Check("soulmates: B can't be chosen again (SOULMATE_ALREADY)", again is not null
            && Read(again, r => (r.ReadByte(), r.ReadString(), r.ReadByte(), r.ReadByte())).Item1 == 5, Describe(again));
        a.Send(Req(CS_SOULMATEEND_REQ, _ => { }));
        var end = a.TryWait(CS_SOULMATEEND_ACK);
        Check("soulmates: A ends the bond (CS_SOULMATEEND_ACK)", end is not null && Read(end, r => r.ReadByte()) == 0, Describe(end));
        a.Send(Req(CS_SOULMATESEARCH_REQ, w => { w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        var wait = a.TryWait(CS_SOULMATESEARCH_ACK);
        Check("soulmates: … and must wait a day before searching again (SOULMATE_SILENCE)", wait is not null
            && Read(wait, r => (r.ReadByte(), r.ReadUInt32(), r.ReadString(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32())).Item1 == 2, Describe(wait));
        await Task.Delay(500);
        var ended = await db.RowAsync("SELECT dwTime FROM TSOULMATETABLE WHERE dwCharID=@p0", (int)idA);
        Check("soulmates: the end is saved (dwTime set)", ended is not null && Convert.ToInt64(ended[0]) != 0, ended is null ? "no row" : $"time={ended[0]}");
    }

    private static PacketWriter Whisper(string from, string to, string talk)
        => Req(CS_CHAT_REQ, w => { w.WriteString(from); w.WriteByte(0 /* CHAT_WHISPER */); w.WriteUInt32(0); w.WriteString(to); w.WriteString(talk); });

    private static async Task SocialLife(BotConfig cfg, GameDb db, uint idA, uint idB)
    {
        var cfgA = Clone(cfg, cfg.Account, "[A]");
        var cfgB = Clone(cfg, cfg.Account2, "[B]");
        using var a = new Bot(cfgA);
        var b = new Bot(cfgB);
        try
        {
            a.Enter(); b.Enter();
            Thread.Sleep(1500);
            string nameA = a.Spawn.Name, nameB = b.Spawn.Name;
            var empty = b.TryWait(CS_PROTECTEDLIST_ACK, timeoutMs: 1000);
            Check("protected: B's empty list arrives on entering (CS_PROTECTEDLIST_ACK)", empty is not null && Read(empty, r => r.ReadByte()) == 0, Describe(empty));

            // Friends: ask, yes.
            a.Send(Req(CS_FRIENDASK_REQ, w => w.WriteString(nameB)));
            var ask = b.TryWait(CS_FRIENDASK_ACK);
            Check("friends: B is asked (CS_FRIENDASK_ACK)", ask is not null && Read(ask, r => r.ReadString()) == nameA, Describe(ask));
            b.Send(Req(CS_FRIENDREPLY_REQ, w => { w.WriteString(nameA); w.WriteByte(0); }));     // ASK_YES
            var addA = a.TryWait(CS_FRIENDADD_ACK);
            var addB = b.TryWait(CS_FRIENDADD_ACK);
            Check("friends: both are told (CS_FRIENDADD_ACK success, the other's id)",
                addA is not null && addB is not null && FriendAdd(addA) is var fa && (fa.Result, fa.Id) == (0, idB) && FriendAdd(addB) is var fb && (fb.Result, fb.Id) == (0, idA),
                $"{Describe(addA)} / {Describe(addB)}");
            await Task.Delay(500);
            var rows = await db.RowAsync("SELECT COUNT(*) FROM TFRIENDTABLE WHERE (dwCharID=@p0 AND dwFriendID=@p1) OR (dwCharID=@p1 AND dwFriendID=@p0)", (int)idA, (int)idB);
            Check("friends: saved both ways in TFRIENDTABLE", rows is not null && Convert.ToInt32(rows[0]) == 2, rows is null ? "no row" : $"{rows[0]} rows");

            // Groups.
            a.Send(Req(CS_FRIENDGROUPMAKE_REQ, w => { w.WriteByte(1); w.WriteString("Pals"); }));
            var make = a.TryWait(CS_FRIENDGROUPMAKE_ACK);
            Check("friend groups: A makes 'Pals'", make is not null && Read(make, r => (r.ReadByte(), r.ReadByte(), r.ReadString())) == (0, 1, "Pals"), Describe(make));
            a.Send(Req(CS_FRIENDGROUPCHANGE_REQ, w => { w.WriteUInt32(idB); w.WriteByte(1); }));
            var change = a.TryWait(CS_FRIENDGROUPCHANGE_ACK);
            Check("friend groups: B is put in it", change is not null && Read(change, r => (r.ReadByte(), r.ReadByte(), r.ReadUInt32())) == (0, 1, idB), Describe(change));
            a.Send(Req(CS_FRIENDGROUPNAME_REQ, w => { w.WriteByte(1); w.WriteString("Mates"); }));
            var rename = a.TryWait(CS_FRIENDGROUPNAME_ACK);
            Check("friend groups: renamed 'Mates'", rename is not null && Read(rename, r => (r.ReadByte(), r.ReadByte(), r.ReadString())) == (0, 1, "Mates"), Describe(rename));
            var list = AskFriendList(a);
            Check("friends: A's list shows the group and B online in it", list is { } l && l.Groups.SequenceEqual(new[] { ((byte)1, "Mates") })
                && l.Friends.SequenceEqual(new[] { (idB, (byte)1, true) }), list is null ? "no CS_FRIENDLIST_ACK" : $"{string.Join(",", list.Value.Groups)} / {string.Join(",", list.Value.Friends)}");
            a.Send(Req(CS_FRIENDGROUPDELETE_REQ, w => w.WriteByte(1)));
            var refuse = a.TryWait(CS_FRIENDGROUPDELETE_ACK);
            Check("friend groups: a group with a friend in it is not deleted (FRIEND_REFUSE)", refuse is not null && Read(refuse, r => (r.ReadByte(), r.ReadByte())) == (1, 1), Describe(refuse));
            a.Send(Req(CS_FRIENDGROUPCHANGE_REQ, w => { w.WriteUInt32(idB); w.WriteByte(0); }));
            a.TryWait(CS_FRIENDGROUPCHANGE_ACK);
            a.Send(Req(CS_FRIENDGROUPDELETE_REQ, w => w.WriteByte(1)));
            var del = a.TryWait(CS_FRIENDGROUPDELETE_ACK);
            Check("friend groups: emptied, it is deleted", del is not null && Read(del, r => (r.ReadByte(), r.ReadByte())) == (0, 1), Describe(del));

            // The messenger.
            a.Send(Req(CS_TMSINVITE_REQ, w => { w.WriteUInt32(0); w.WriteByte(1); w.WriteUInt32(idB); }));
            var invA = a.TryWait(CS_TMSINVITE_ACK);
            var invB = b.TryWait(CS_TMSINVITE_ACK);
            uint tms = invA is null ? 0 : TmsInvite(invA).Tms;
            Check("messenger: A opens a conversation with B, both see the two of them (CS_TMSINVITE_ACK)", tms != 0 && invB is not null
                && TmsInvite(invB) == (idA, tms, 2), $"{Describe(invA)} / {Describe(invB)}");
            a.Send(Req(CS_TMSSEND_REQ, w => { w.WriteUInt32(tms); w.WriteString("hello"); }));
            var recvB = b.TryWait(CS_TMSRECV_ACK);
            Check("messenger: B gets A's line", recvB is not null && Read(recvB, r => (r.ReadUInt32(), r.ReadString(), r.ReadString())) == (tms, nameA, "hello"), Describe(recvB));
            a.TryWait(CS_TMSRECV_ACK);
            b.Send(Req(CS_TMSOUT_REQ, w => w.WriteUInt32(tms)));
            var outA = a.TryWait(CS_TMSOUT_ACK);
            Check("messenger: A is told B left", outA is not null && Read(outA, r => (r.ReadUInt32(), r.ReadString())) == (tms, nameB), Describe(outA));
            b.TryWait(CS_TMSOUT_ACK);
            a.Send(Req(CS_TMSSEND_REQ, w => { w.WriteUInt32(tms); w.WriteString("come back"); }));
            var backA = a.TryWait(CS_TMSINVITE_ACK);
            var backB = b.TryWait(CS_TMSINVITE_ACK);
            Check("messenger: A's next line brings B back — both see the new roster (fixed C++ bug: only B did)", backA is not null && backB is not null
                && TmsInvite(backA).Count == 2 && TmsInvite(backB).Count == 2, $"{Describe(backA)} / {Describe(backB)}");
            var again = b.TryWait(CS_TMSRECV_ACK);
            Check("messenger: … and B gets the line", again is not null && Read(again, r => (r.ReadUInt32(), r.ReadString(), r.ReadString())).Item3 == "come back", Describe(again));
            a.Send(Req(CS_TMSOUT_REQ, w => w.WriteUInt32(tms)));
            b.Send(Req(CS_TMSOUT_REQ, w => w.WriteUInt32(tms)));
            Thread.Sleep(500);

            // B blocks A.
            a.Discard(CS_FRIENDCONNECTION_ACK);
            b.Send(Req(CS_PROTECTEDADD_REQ, w => w.WriteString(nameA)));
            var added = b.TryWait(CS_PROTECTEDADD_ACK);
            Check("protected: B blocks A (CS_PROTECTEDADD_ACK success, chat refused)", added is not null
                && Read(added, r => (r.ReadByte(), r.ReadUInt32(), r.ReadString(), r.ReadByte())) == (0, idA, nameA, 1), Describe(added));
            var gone = a.TryWait(CS_FRIENDCONNECTION_ACK);
            Check("protected: A sees B go offline (CS_FRIENDCONNECTION_ACK disconnection)", gone is not null
                && Read(gone, r => (r.ReadByte(), r.ReadString(), r.ReadUInt32())) == (1, nameB, 0u), Describe(gone));
            var broken = b.TryWait(CS_FRIENDERASE_ACK);
            Check("protected: the friendship breaks on B's side (CS_FRIENDERASE_ACK)", broken is not null && Read(broken, r => (r.ReadByte(), r.ReadUInt32())) == (0, idA), Describe(broken));
            b.Discard(CS_CHAT_ACK); a.Discard(CS_CHAT_ACK);
            a.Send(Whisper(nameA, nameB, "psst"));
            var echo = a.TryWait(CS_CHAT_ACK);
            var heard = b.TryWait(CS_CHAT_ACK, timeoutMs: 1500);
            Check("protected: A's whisper never reaches B (A still sees its own)", echo is not null && heard is null, $"{Describe(echo)} / {Describe(heard)}");
            b.Send(Req(CS_PROTECTEDOPTION_REQ, w => { w.WriteUInt32(idA); w.WriteByte(BlockAll); }));
            await Task.Delay(800);
            var opt = await db.RowAsync("SELECT bOption FROM TPROTECTEDTABLE WHERE dwCharID=@p0 AND dwProtected=@p1", (int)idB, (int)idA);
            Check("protected: B blocks everything from A, saved in TPROTECTEDTABLE", opt is not null && Convert.ToInt32(opt[0]) == BlockAll, opt is null ? "no row" : $"option={opt[0]}");
            a.Send(Req(CS_DEALITEMASK_REQ, w => w.WriteString(nameB)));
            var deal = a.TryWait(CS_DEALITEMEND_ACK);
            Check("protected: A's trade is refused (DEALITEM_DENY)", deal is not null && Read(deal, r => (r.ReadByte(), r.ReadString())) == (1, nameB) && b.TryWait(CS_DEALITEMASK_ACK, timeoutMs: 500) is null, Describe(deal));
            a.Send(Req(CS_DUELINVITE_REQ, w => w.WriteUInt32(idB)));
            var duel = a.TryWait(CS_DUELSTART_ACK);
            Check("protected: A's duel is refused (DUEL_REFUSE)", duel is not null && Read(duel, r => (r.ReadByte(), r.ReadUInt32(), r.ReadUInt32())) == (1, idA, idB) && b.TryWait(CS_DUELINVITE_ACK, timeoutMs: 500) is null, Describe(duel));
            a.Send(Req(CS_PARTYADD_REQ, w => { w.WriteString(nameB); w.WriteByte(0); }));
            var party = a.TryWait(CS_PARTYADD_ACK);
            Check("protected: A's party invite is answered no (ASK_NO)", party is not null && Read(party, r => (r.ReadString(), r.ReadString(), r.ReadByte())).Item3 == 1
                && b.TryWait(CS_PARTYJOINASK_ACK, timeoutMs: 500) is null, Describe(party));
            b.Send(Req(CS_FRIENDASK_REQ, w => w.WriteString(nameA)));
            var self = b.TryWait(CS_FRIENDADD_ACK);
            Check("protected: B can't ask A, whom it blocks, to be friends (FRIEND_REFUSE)", self is not null && FriendAdd(self) == (1, 0u, nameA)
                && a.TryWait(CS_FRIENDASK_ACK, timeoutMs: 500) is null, Describe(self));

            // B logs in again: the list comes back, and A — who still has B — is not told.
            b.Dispose();
            Thread.Sleep(2500);
            a.Discard(CS_FRIENDCONNECTION_ACK);
            b = new Bot(cfgB);
            b.Enter();
            var back = b.TryWait(CS_PROTECTEDLIST_ACK, timeoutMs: 3000);
            Check("protected: B's list is read again on entering (A, everything blocked)", back is not null
                && Read(back, r => (r.ReadByte(), r.ReadUInt32(), r.ReadString(), r.ReadByte())) == (1, idA, nameA, BlockAll), Describe(back));
            var told = a.TryWait(CS_FRIENDCONNECTION_ACK, timeoutMs: 2000);
            Check("protected: A is not told that B, who blocks it, came online", told is null, Describe(told));
            var seen = AskFriendList(a);
            Check("protected: … and A's list shows B offline", seen is { } s && s.Friends.SequenceEqual(new[] { (idB, (byte)0, false) }),
                seen is null ? "no CS_FRIENDLIST_ACK" : string.Join(",", seen.Value.Friends));

            // B takes A off the list.
            b.Send(Req(CS_PROTECTEDERASE_REQ, w => w.WriteString(nameA)));
            var erased = b.TryWait(CS_PROTECTEDERASE_ACK);
            Check("protected: B takes A off its list", erased is not null && Read(erased, r => (r.ReadByte(), r.ReadString())) == (0, nameA), Describe(erased));
            var on = a.TryWait(CS_FRIENDCONNECTION_ACK);
            Check("protected: A sees B online again", on is not null && Read(on, r => (r.ReadByte(), r.ReadString(), r.ReadUInt32())) is var o && (o.Item1, o.Item2) == (0, nameB), Describe(on));
            await Task.Delay(500);
            var left = await db.RowAsync("SELECT COUNT(*) FROM TPROTECTEDTABLE WHERE dwCharID=@p0", (int)idB);
            Check("protected: the row is gone from TPROTECTEDTABLE", left is not null && Convert.ToInt32(left[0]) == 0, left is null ? "no row" : $"{left[0]} rows");
            b.Discard(CS_CHAT_ACK);
            a.Send(Whisper(nameA, nameB, "hi again"));
            var whisper = b.TryWait(CS_CHAT_ACK);
            Check("protected: A's whisper reaches B again", whisper is not null && Read(whisper, r => { r.ReadByte(); r.ReadUInt32(); r.ReadString(); return r.ReadString(); }) == "hi again", Describe(whisper));

            await Soulmates(a, db, idA, idB, nameB);
        }
        finally
        {
            b.Dispose();
        }
    }
}
