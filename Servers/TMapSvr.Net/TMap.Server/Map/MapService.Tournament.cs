using TMap.Data;
using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// The tournament, batch S6e-1 — the windows and signing up (C++ CSHandler.cpp:16564–16960; SSHandler.cpp:17791 (MW_TOURNAMENT_REQ),
/// 17892 (the step part of MW_TOURNAMENTENABLE_REQ), 18124 (MW_TOURNAMENTINFO_REQ), 18197 (MW_TOURNAMENTMATCH_REQ), 18464 (bets);
/// TMapSvr.cpp:11144–11370; CSSender.cpp:5839).
/// <list type="bullet">
/// <item>The world runs the tournament (WorldService.Tournament*.cs): it sends the entries (<c>MW_TOURNAMENTINFO_REQ</c>), each step
/// (<c>MW_TOURNAMENTENABLE_REQ</c>, shown as <c>SM_TOURNAMENT_STEP</c>) and the bracket (<c>MW_TOURNAMENTMATCH_REQ</c>).</item>
/// <item>The windows — sign-up info, party list, bets, schedule — are asked of the world (<c>MW_TOURNAMENT_ACK</c>) and its answers
/// relayed to the client (<c>MW_TOURNAMENT_REQ</c> → <c>CS_TOURNAMENT*_ACK</c>).</item>
/// <item>Signing up (<c>CS_TOURNAMENTAPPLY_REQ</c>) checks the entry's levels, class, fee and permit item — in the first step only the
/// first-grade players of the last month — and takes the fee and permits; the world's refusal gives them back. A party member added
/// is told by mail. A bet won is paid by mail (<c>MW_TOURNAMENTBATPOINT_REQ</c>).</item>
/// <item>During a match, a cheer item heals a fighter on one's tournament map (<c>CS_TOURNAMENTCHEER_REQ</c>).</item>
/// </list>
/// <para>The matches themselves are in MapService.TournamentMatch.cs (batch S6e-2). Fixed C++ bug: a cheer was refused when the cheerer (not the fighter) was at full health.</para>
/// </summary>
public sealed partial class MapService
{
    private const byte TnmtReady = 0, Tnmt1st = 1, TnmtNormal = 2, TnmtParty = 3, TnmtMatch = 4, TnmtEnter = 5, TnmtQFinal = 6,
        TnmtQFEnd = 7, TnmtSFEnter = 8, TnmtSFinal = 9, TnmtSFEnd = 10, TnmtFEnter = 11, TnmtFinal = 12, TnmtEnd = 13;   // TNMTSTEP
    private const byte TnmtSuccess = 0, TnmtDisqualify = 1, TnmtTimeout = 2, TnmtClass = 6, TnmtMoney = 7, TnmtItem = 8,
        TnmtLevel = 9, TnmtFail = 10;                                                           // TOURNAMENT_RESULT
    private const byte TnmtWinNone = 0, TnmtWinWin = 1, TnmtWinLose = 2;                       // TNMTWIN
    private const byte SmTournamentStep = 18;
    private const ushort MapIdTournament = 501, TournamentLoungeSpawn = 10011;
    private const int TournamentMapCount = 4 * 8;                                             // ARENA_COUNT * 8
    private const uint MsgTnmtBattingTitle = 42, MsgTnmtBattingMessage = 43, MsgTnmtPartyAddTitle = 49, MsgTnmtPartyAddMessage = 50;

    private sealed record TournamentEntry(byte Group, byte EntryId, string Name, byte Type, uint Class, uint Fee, uint BackFee,
        ushort PermitItem, byte PermitCount, byte MinLevel, byte MaxLevel, List<(byte ChartType, ushort ItemId, byte Count)> Rewards);

    private sealed class TournamentPlayer
    {
        public byte EntryId, Slot, Country, Level, Class;
        public uint CharId, ChiefId;
        public string Name = "";
        public readonly byte[] Result = new byte[3];                                           // MATCH_QFINAL / SFINAL / FINAL
        public ushort MapId;
    }

    private readonly SortedDictionary<byte, TournamentEntry> _tournaments = new();
    private readonly Dictionary<uint, TournamentPlayer> _tnmtPlayers = new();
    private byte _tnmtStep, _tnmtGroup;
    private long _tnmtEndTime;

    /// <summary>C++ <c>GetTournamentStep</c>: no entries, no tournament.</summary>
    private byte TournamentStep => _tournaments.Count == 0 ? TnmtReady : _tnmtStep;

    /// <summary>C++ <c>CanDoTournament</c>: the match list and the betting stay open from their step on; the rest is that step only.</summary>
    private bool CanDoTournament(byte step) => step is TnmtMatch or TnmtEnter ? TournamentStep >= step : TournamentStep == step;

    private static bool IsTournamentMap(ushort mapId) => mapId >= MapIdTournament && mapId < MapIdTournament + TournamentMapCount;

    private static ushort TournamentLoungeMap => MapIdTournament - 1;

    /// <summary>C++ <c>FindTournamentPlayer</c>: in this group, and with <paramref name="active"/> still in (no loss).</summary>
    private TournamentPlayer? FindTournamentPlayer(uint charId, bool active = false)
    {
        if (!_tnmtPlayers.TryGetValue(charId, out var p) || !_tournaments.TryGetValue(p.EntryId, out var e) || e.Group != _tnmtGroup) return null;
        return !active || p.Result.All(r => r != TnmtWinLose) ? p : null;
    }

    /// <summary>C++ <c>GetTournamentMap</c> (TMapSvr.cpp:11173): the map a fighter plays its current match on, 0 when out.</summary>
    private ushort TournamentMapOf(uint charId, byte group)
    {
        if (!_tnmtPlayers.TryGetValue(charId, out var p)) return 0;
        if (group != 0 && (!_tournaments.TryGetValue(p.EntryId, out var e) || e.Group != group)) return 0;
        if (p.Result.Any(r => r == TnmtWinLose)) return 0;
        int baseMap = MapIdTournament + (p.EntryId - 1) * 4;
        if (p.Result[1] == TnmtWinWin && p.Result[2] == TnmtWinNone && _tnmtStep >= TnmtFEnter) return (ushort)baseMap;
        if (p.Result[0] == TnmtWinWin && p.Result[1] == TnmtWinNone && _tnmtStep >= TnmtSFEnter) return (ushort)(baseMap + p.Slot / 4 * 2);
        if (p.Result[0] == TnmtWinNone && _tnmtStep < TnmtSFEnter) return (ushort)(baseMap + p.Slot / 2);
        return 0;
    }

    private void SendTournamentToWorld(ClientSession s, ushort protocol, Action<PacketWriter>? body = null)
        => SendToWorld(Msg.MW_TOURNAMENT_ACK, s, w => { w.WriteUInt16(protocol); body?.Invoke(w); });

    private static void SendTournamentStep(ClientSession s, byte group, byte step, uint period, long next)
    {
        var w = new PacketWriter(Msg.CS_SYSTEMMSG_ACK);
        w.WriteByte(SmTournamentStep); w.WriteByte(group); w.WriteByte(step); w.WriteUInt32(period); w.WriteInt64(next);
        s.Send(w);
    }

    /// <summary>A tournament map or the lounge: the player goes back to its last spawn point (C++ the step reset, SSHandler.cpp:18110).</summary>
    private void SendHomeFromTournament(ClientSession s, Character ch)
    {
        if (!IsTournamentMap(ch.MapId) && ch.MapId != TournamentLoungeMap) return;
        Teleport(s, ch, ch.Persist.LastSpawnId != 0 ? ch.Persist.LastSpawnId : ch.Persist.SpawnId);
    }

    // ================================ the windows ================================

    /// <summary>C++ <c>OnCS_TOURNAMENTAPPLYINFO_REQ</c>: the sign-up window, the party window or the match list, by step.</summary>
    private void OnCS_TOURNAMENTAPPLYINFO_REQ(ClientSession s)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        if (_tnmtStep <= TnmtNormal) SendTournamentToWorld(s, Msg.MW_TOURNAMENTAPPLYINFO_ACK);
        else if (CanDoTournament(TnmtParty)) SendTournamentToWorld(s, Msg.MW_TOURNAMENTJOINLIST_ACK);
        else if (CanDoTournament(TnmtMatch)) SendTournamentToWorld(s, Msg.MW_TOURNAMENTMATCHLIST_ACK);
    }

    private void OnCS_TOURNAMENTPARTYLIST_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch) || !CanDoTournament(TnmtParty)) return;
        uint chief = r.ReadUInt32();
        SendTournamentToWorld(s, Msg.MW_TOURNAMENTPARTYLIST_ACK, w => w.WriteUInt32(chief));
    }

    private void OnCS_TOURNAMENTPARTYADD_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch) || !CanDoTournament(TnmtParty)) return;
        string name = r.ReadString();
        SendTournamentToWorld(s, Msg.MW_TOURNAMENTPARTYADD_ACK, w => w.WriteString(name));
    }

    private void OnCS_TOURNAMENTPARTYDEL_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch) || !CanDoTournament(TnmtParty)) return;
        uint target = r.ReadUInt32();
        SendTournamentToWorld(s, Msg.MW_TOURNAMENTPARTYDEL_ACK, w => w.WriteUInt32(target));
    }

    private void OnCS_TOURNAMENTEVENTLIST_REQ(ClientSession s)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch) || !CanDoTournament(TnmtEnter)) return;
        SendTournamentToWorld(s, Msg.MW_TOURNAMENTEVENTLIST_ACK);
    }

    private void OnCS_TOURNAMENTEVENTINFO_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch) || TournamentStep != TnmtEnter) return;
        byte entry = r.ReadByte();
        SendTournamentToWorld(s, Msg.MW_TOURNAMENTEVENTINFO_ACK, w => w.WriteByte(entry));
    }

    private void OnCS_TOURNAMENTEVENTJOIN_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch) || TournamentStep != TnmtEnter) return;
        byte entry = r.ReadByte();
        uint target = r.ReadUInt32();
        SendTournamentToWorld(s, Msg.MW_TOURNAMENTEVENTJOIN_ACK, w => { w.WriteByte(entry); w.WriteUInt32(target); });
    }

    private void OnCS_TOURNAMENTSCHEDULE_REQ(ClientSession s)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        SendTournamentToWorld(s, Msg.MW_TOURNAMENTSCHEDULE_ACK);
    }

    // ================================ signing up ================================

    private static int ItemCount(Character ch, ushort itemId) => ch.Invens.Sum(b => b.Items.Where(i => i.TemplateId == itemId).Sum(i => i.Count));

    /// <summary>C++ <c>CTPlayer::UseItem(wItemID, bCount)</c>: takes <paramref name="count"/> of an item from wherever it is.</summary>
    private void UseItemById(ClientSession s, Character ch, ushort itemId, int count)
    {
        foreach (var bag in ch.Invens.ToList())
            foreach (var item in bag.Items.Where(i => i.TemplateId == itemId).ToList())
            {
                if (count <= 0) return;
                byte n = (byte)Math.Min(count, item.Count);
                count -= n;
                UseItemStack(s, bag.InvenId, bag, item, n);
            }
    }

    /// <summary>C++ <c>CanApplyTournament</c> (TMapSvr.cpp:11330).</summary>
    private byte CanApplyTournament(Character ch, TournamentEntry e)
    {
        if ((e.Class & (1u << ch.Class)) == 0) return TnmtClass;
        if (!ch.UseMoney(e.Fee, commit: false)) return TnmtMoney;
        if (e.PermitItem != 0 && ItemCount(ch, e.PermitItem) < e.PermitCount) return TnmtItem;
        if (e.MaxLevel < ch.Level) return TnmtDisqualify;
        return TnmtSuccess;
    }

    /// <summary>C++ <c>OnCS_TOURNAMENTAPPLY_REQ</c> (CSHandler.cpp:16586).</summary>
    private void OnCS_TOURNAMENTAPPLY_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch) || ch.Country > TcontryB) return;
        byte entryId = r.ReadByte();
        string hwid = r.ReadString();
        byte result = TnmtTimeout;
        _tournaments.TryGetValue(entryId, out var e);
        void Apply() => SendTournamentToWorld(s, Msg.MW_TOURNAMENTAPPLY_ACK, w => { w.WriteByte(entryId); w.WriteString(hwid); w.WriteUInt32(s.ClientIp); });
        if (e is null) result = TnmtFail;
        else if (e.MaxLevel < ch.Level || e.MinLevel > ch.Level) result = TnmtLevel;
        else if (CanDoTournament(Tnmt1st))
        {
            for (int i = 1; i < FirstGroupCount && ch.Country < CountryCount; i++)
                if (_firstGrade[ch.Country, i].CharId == ch.CharId)
                {
                    result = CanApplyTournament(ch, e);
                    if (result == TnmtSuccess) Apply();
                    break;
                }
            if (result == TnmtTimeout) result = TnmtDisqualify;
        }
        else if (CanDoTournament(TnmtNormal))
        {
            result = CanApplyTournament(ch, e);
            if (result == TnmtSuccess) Apply();
        }

        if (result == TnmtSuccess && e is not null)
        {
            ch.UseMoney(e.Fee, commit: true);
            if (e.PermitItem != 0) UseItemById(s, ch, e.PermitItem, e.PermitCount);
        }
        else s.Send(new PacketWriter(Msg.CS_TOURNAMENTAPPLY_ACK).WriteByte(result).WriteByte(entryId));
    }

    // ================================ the world's answers ================================

    /// <summary>C++ <c>OnMW_TOURNAMENT_REQ</c> (SSHandler.cpp:17791) + <c>SendCS_TOURNAMENT_ACK</c>: a refused sign-up gives the fee and
    /// permits back; a party member added is told by mail; the answer goes to the client under its CS id.</summary>
    private void OnMW_TOURNAMENT_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        ushort protocol = r.ReadUInt16();
        if (FindPlayer(charId, key) is not { Char: { } ch } s) return;
        byte[] body = r.ReadBytes(r.Remaining);
        var br = new PacketReader(new PacketWriter(0).WriteRaw(body).ToArray());
        if (protocol == Msg.MW_TOURNAMENTAPPLY_REQ)
        {
            byte result = br.ReadByte(), entryId = br.ReadByte();
            if (_tournaments.TryGetValue(entryId, out var e))
            {
                if (result != 0)
                {
                    ch.EarnMoney(e.Fee);
                    if (e.PermitItem != 0 && e.PermitCount != 0 && _templates.Item(e.PermitItem) is { } tpl)
                    {
                        var permit = new Item { TemplateId = e.PermitItem, Count = e.PermitCount, Template = tpl, DuraMax = tpl.DuraMax, DuraCur = tpl.DuraMax };
                        LinkItemAttr(permit);
                        if (CanPush(ch, new[] { permit })) PushTItem(s, new[] { permit });
                    }
                }
                if (e.Fee != 0) SendCS_MONEY_ACK(s, ch);
            }
        }
        else if (protocol == Msg.MW_TOURNAMENTPARTYADD_REQ)
        {
            byte result = br.ReadByte();
            string target = br.ReadString();
            uint targetId = br.ReadUInt32();
            if (result == TnmtSuccess && targetId != 0)
                _ = MailOperator(targetId, target, SvrMsg(MsgTnmtPartyAddTitle), NetString(SvrMsg(MsgTnmtPartyAddMessage)),
                    Array.Empty<(ItemTemplate, byte)>(), 0, 0, 0, PostNormal);
        }

        ushort? id = protocol switch
        {
            Msg.MW_TOURNAMENTSCHEDULE_REQ => Msg.CS_TOURNAMENTSCHEDULE_ACK,
            Msg.MW_TOURNAMENTAPPLYINFO_REQ => Msg.CS_TOURNAMENTAPPLYINFO_ACK,
            Msg.MW_TOURNAMENTAPPLY_REQ => Msg.CS_TOURNAMENTAPPLY_ACK,
            Msg.MW_TOURNAMENTJOINLIST_REQ => Msg.CS_TOURNAMENTJOINLIST_ACK,
            Msg.MW_TOURNAMENTPARTYLIST_REQ => Msg.CS_TOURNAMENTPARTYLIST_ACK,
            Msg.MW_TOURNAMENTPARTYADD_REQ => Msg.CS_TOURNAMENTPARTYADD_ACK,
            Msg.MW_TOURNAMENTMATCHLIST_REQ => Msg.CS_TOURNAMENTMATCHLIST_ACK,
            Msg.MW_TOURNAMENTEVENTLIST_REQ => Msg.CS_TOURNAMENTEVENTLIST_ACK,
            Msg.MW_TOURNAMENTEVENTINFO_REQ => Msg.CS_TOURNAMENTEVENTINFO_ACK,
            Msg.MW_TOURNAMENTEVENTJOIN_REQ => Msg.CS_TOURNAMENTEVENTJOIN_ACK,
            _ => null,
        };
        if (id is { } cs) s.Send(new PacketWriter(cs).WriteRaw(body));
    }

    /// <summary>C++ <c>OnMW_TOURNAMENTINFO_REQ</c> (SSHandler.cpp:18124): the entries and where the tournament stands; everyone is told,
    /// and whoever is in a tournament map or the lounge goes home.</summary>
    private void OnMW_TOURNAMENTINFO_REQ(PacketReader r)
    {
        ClearTournamentPlayers();
        _tournaments.Clear();
        FirstGroupCount = r.ReadByte();
        byte group = r.ReadByte(), step = r.ReadByte(), count = r.ReadByte();
        for (int i = 0; i < count; i++)
        {
            byte g = r.ReadByte(), entryId = r.ReadByte();
            string name = r.ReadString();
            byte type = r.ReadByte();
            uint cls = r.ReadUInt32(), fee = r.ReadUInt32(), backFee = r.ReadUInt32();
            ushort permit = r.ReadUInt16();
            byte permitCount = r.ReadByte(), minLevel = r.ReadByte(), maxLevel = r.ReadByte();
            var rewards = new List<(byte, ushort, byte)>();
            for (byte n = r.ReadByte(), j = 0; j < n; j++) rewards.Add((r.ReadByte(), r.ReadUInt16(), r.ReadByte()));
            _tournaments[entryId] = new TournamentEntry(g, entryId, name, type, cls, fee, backFee, permit, permitCount, minLevel, maxLevel, rewards);
        }
        _tnmtStep = step;
        _tnmtGroup = group;
        _tnmtEndTime = 0;
        foreach (var s in _state.AllInGame().ToList())
        {
            if (!s.IsMain || s.Char is not { } ch) continue;
            SendTournamentStep(s, group, step, 0, 0);
            SendHomeFromTournament(s, ch);
        }
    }

    /// <summary>C++ <c>OnMW_TOURNAMENTMATCH_REQ</c> (SSHandler.cpp:18197): the bracket — every fighter, its entry, slot, chief and
    /// results so far.</summary>
    private void OnMW_TOURNAMENTMATCH_REQ(PacketReader r)
    {
        ClearTournamentPlayers();
        byte count = r.ReadByte();
        for (int i = 0; i < count; i++)
        {
            var p = new TournamentPlayer
            {
                EntryId = r.ReadByte(), Slot = r.ReadByte(), CharId = r.ReadUInt32(), Country = r.ReadByte(), Name = r.ReadString(),
                Level = r.ReadByte(), Class = r.ReadByte(), ChiefId = r.ReadUInt32(),
            };
            p.Result[0] = r.ReadByte(); p.Result[1] = r.ReadByte(); p.Result[2] = r.ReadByte();
            if (!_tournaments.ContainsKey(p.EntryId)) continue;
            _tnmtPlayers[p.CharId] = p;
            if (TournamentMapOf(p.CharId, 0) is var map and not 0) AddTnmtPlayer(MatchOf(map), p);   // CTMap::AddTNMTPlayer
        }
    }

    /// <summary>C++ <c>OnMW_TOURNAMENTENABLE_REQ</c>'s step part (SSHandler.cpp:17892): everyone is told the step; going back to an
    /// earlier step sends whoever is in a tournament map or the lounge home. (The matches' part is batch S6e-2.)</summary>
    private void OnMW_TOURNAMENTENABLE_REQ(PacketReader r)
    {
        byte group = r.ReadByte(), step = r.ReadByte();
        uint period = r.ReadUInt32();
        long next = r.ReadInt64();
        byte current = _tnmtStep;
        foreach (var s in _state.AllInGame().ToList()) if (s.IsMain) SendTournamentStep(s, group, step, period, next);
        TournamentMatchesOnStep(group, step, period);                                         // MapService.TournamentMatch.cs
        _tnmtGroup = group;
        _tnmtStep = step;
        _tnmtEndTime = UnixNow() + period;
        if (current > step)
            foreach (var s in _state.AllInGame().ToList()) if (s.IsMain && s.Char is { } ch) SendHomeFromTournament(s, ch);
    }

    /// <summary>C++ <c>OnMW_TOURNAMENTBATPOINT_REQ</c> (SSHandler.cpp:18464): a bet won, paid by the operator's mail.</summary>
    private void OnMW_TOURNAMENTBATPOINT_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32();
        string name = r.ReadString();
        uint amount = r.ReadUInt32();
        var (gold, silver, cooper) = SplitMoney(amount);
        _ = MailOperator(charId, name, SvrMsg(MsgTnmtBattingTitle), NetString(SvrMsg(MsgTnmtBattingMessage)),
            Array.Empty<(ItemTemplate, byte)>(), gold, silver, cooper);
    }

    // ================================ cheering ================================

    /// <summary>C++ <c>OnCS_TOURNAMENTCHEER_REQ</c> (CSHandler.cpp:16817): during a match, a cheer item heals a fighter on one's map.</summary>
    private void OnCS_TOURNAMENTCHEER_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        byte inven = r.ReadByte(), slot = r.ReadByte();
        uint targetId = r.ReadUInt32();
        if (_tnmtStep is not (TnmtQFinal or TnmtSFinal or TnmtFinal)) return;
        var bag = ch.FindInven(inven);
        if (bag?.FindItem(slot) is not { Count: > 0, Template: { } tpl } item) return;
        if (FindTournamentPlayer(targetId, active: true) is null || TournamentMapOf(targetId, _tnmtGroup) != ch.MapId) return;
        if (_state.FindByChar(targetId) is not { IsMain: true, Char: { } target } ts || target.MapId != ch.MapId || target.Hp == 0) return;
        uint maxHp = MaxHpFor(target);
        if (maxHp <= target.Hp) return;                                                       // fixed: the C++ looked at the cheerer
        target.Hp = Math.Min(maxHp, target.Hp + tpl.UseValue);
        var cheer = new PacketWriter(Msg.CS_TOURNAMENTCHEER_ACK).WriteUInt32(ch.CharId).WriteUInt32(targetId).ToArray();
        BroadcastHpMp(ts, target);
        foreach (var p in _state.InView(ts).ToList()) p.Send(cheer);
        UseItemStack(s, inven, bag, item, 1);
    }
}
