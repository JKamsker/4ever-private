using TWorld.Protocol;

namespace TWorld.Server.World;

/// <summary>
/// Corps — several parties at war under one commanding party and its chief, the general — and party member recall, per the
/// C++ (SSHandler.cpp:6778–7430, 8710–8880; TWorldSvr.cpp:2865–3640, 4512; TCorps.cpp). Corps live in memory, like parties.
/// <list type="bullet">
/// <item><b>Forming:</b> a party chief asks another (same war country, neither in an arena, not both in a corps, at most 7
/// parties); on yes, a new corps is made under the asker's party, or the party without one joins the other's. Each side is shown
/// the other's squads (<c>MW_ADDSQUAD_REQ</c>) and told its corps (<c>MW_CORPSJOIN_REQ</c>).</item>
/// <item><b>Leaving:</b> a chief takes its own party out, the general any party; a corps down to one party ends; a commanding
/// party leaving hands command to the next. A party also leaves when it breaks up; a member leaving is taken off every
/// squad (<c>MW_DELCORPSUNIT_REQ</c>), a member joining is put on them (<c>MW_ADDCORPSUNIT_REQ</c>, and on its own party's
/// squad even without a corps).</item>
/// <item><b>Orders</b> (<c>MW_CORPSCMD_ACK</c>) go to every member of the corps (or of the party). Enemy reports, unit moves and
/// HP (<see cref="RelayCorps"/>) go to the sender's party and, from a chief, to the other chiefs.</item>
/// <item><b>Command</b> passes to another party by the general; the general may move a member between parties.</item>
/// <item><b>Member recall:</b> an item brings a party member on the same map to you (it is asked first), or takes you to someone of
/// your country on the same map.</item>
/// </list>
/// <para><b>Fixed C++ bugs:</b> the general could take a party out of another corps (it was only checked to exist); and anyone
/// on the map could answer a recall meant for someone else and be pulled to the asker, using up its item — the world now remembers
/// who was asked (<see cref="Character.PendingRecall"/>).</para>
/// </summary>
public sealed partial class WorldService
{
    // CORPS_RESULT (NetCode.h:604), MAX_CORPS_PARTY (TWorldType.h:151)
    private const byte CorpsSuccess = 0, CorpsBusy = 2, CorpsNoParty = 3, CorpsNotCommander = 4, CorpsWrongTarget = 5,
        CorpsTargetNoParty = 6, CorpsChgCommander = 8, CorpsMaxParty = 10;
    private const int MaxCorpsParty = 7;
    // TITEMUSE_RESULT, the recall types (NetCode.h:408, 2228)
    private const byte IuSuccess = 0, IuNotFound = 1, IuTargetBusy = 10;
    private const byte TpRecall = 0, TpMoveTo = 1;

    private async Task<bool> DispatchCorpsAsync(ServerSession session, PacketReader r, byte[] packet)
    {
        switch (r.Id)
        {
            case Msg.MW_CORPSASK_ACK: OnCorpsAsk(session, r); break;
            case Msg.MW_CORPSREPLY_ACK: OnCorpsReply(session, r); break;
            case Msg.MW_CORPSLEAVE_ACK: OnCorpsLeave(r); break;
            case Msg.MW_CORPSCMD_ACK: OnCorpsCmd(r); break;
            case Msg.MW_CHGCORPSCOMMANDER_ACK: OnChgCorpsCommander(r); break;
            case Msg.MW_CORPSENEMYLIST_ACK: RelayCorps(r, Msg.MW_CORPSENEMYLIST_REQ); break;
            case Msg.MW_MOVECORPSENEMY_ACK: RelayCorps(r, Msg.MW_MOVECORPSENEMY_REQ); break;
            case Msg.MW_MOVECORPSUNIT_ACK: RelayCorps(r, Msg.MW_MOVECORPSUNIT_REQ); break;
            case Msg.MW_ADDCORPSENEMY_ACK: RelayCorps(r, Msg.MW_ADDCORPSENEMY_REQ); break;
            case Msg.MW_DELCORPSENEMY_ACK: RelayCorps(r, Msg.MW_DELCORPSENEMY_REQ); break;
            case Msg.MW_CORPSHP_ACK: RelayCorps(r, Msg.MW_CORPSHP_REQ); break;
            case Msg.MW_PARTYMOVE_ACK: OnPartyMove(session, r); break;
            case Msg.MW_PARTYMEMBERRECALL_ACK: OnPartyMemberRecall(session, r); break;
            case Msg.MW_PARTYMEMBERRECALLANS_ACK: OnPartyMemberRecallAns(r); break;
            default: return false;
        }
        await Task.CompletedTask;
        return true;
    }

    // ================================ forming and leaving ================================

    /// <summary>C++ <c>CheckCorpsJoin</c> (TWorldSvr.cpp:4512).</summary>
    private byte CheckCorpsJoin(Party origin, Party target)
    {
        if (origin.CorpsId != 0 && target.CorpsId != 0) return CorpsWrongTarget;
        foreach (var p in new[] { origin, target })
        {
            if (p.CorpsId == 0) continue;
            if (_state.FindCorps(p.CorpsId) is not { } c) return CorpsWrongTarget;
            if (c.Parties.Count >= MaxCorpsParty) return CorpsMaxParty;
        }
        return 0;
    }

    private static bool BothChiefs(Character a, Character b)
        => a.Party is { } pa && pa.IsChief(a.CharId) && b.Party is { } pb && pb.IsChief(b.CharId);

    /// <summary>C++ <c>OnMW_CORPSASK_ACK</c> (SSHandler.cpp:6873).</summary>
    private void OnCorpsAsk(ServerSession session, PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        string target = r.ReadString();
        if (_state.FindChar(charId, key) is not { } player) return;
        void Fail(byte result) => session.Send(BuildCorpsReply(charId, player.Key, result, target));
        if (CharByName(target) is not { } tgt) { Fail(CorpsWrongTarget); return; }
        if (!BothChiefs(player, tgt) || GetWarCountry(player) != GetWarCountry(tgt)) { Fail(CorpsNoParty); return; }
        if (player.Party!.Arena != 0 || tgt.Party!.Arena != 0) { Fail(CorpsBusy); return; }
        if (CheckCorpsJoin(player.Party, tgt.Party) is var check and not 0) { Fail(check); return; }

        var w = new PacketWriter(Msg.MW_CORPSASK_REQ);
        w.WriteUInt32(tgt.CharId); w.WriteUInt32(tgt.Key); w.WriteUInt32(player.CharId); w.WriteString(player.Name);
        SendToChar(tgt, w.ToArray());
    }

    /// <summary>C++ <c>OnMW_CORPSREPLY_ACK</c> (SSHandler.cpp:6953): a no goes back to the asker; a yes makes or grows the corps.</summary>
    private void OnCorpsReply(ServerSession session, PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte reply = r.ReadByte();
        string reqName = r.ReadString();
        if (_state.FindChar(charId, key) is not { } player) return;
        if (CharByName(reqName) is not { } req || MapOf(req) is null)
        {
            session.Send(BuildCorpsReply(charId, player.Key, CorpsWrongTarget, reqName));
            return;
        }
        void Both(byte result)
        {
            session.Send(BuildCorpsReply(player.CharId, player.Key, result, reqName));
            SendToChar(req, BuildCorpsReply(req.CharId, req.Key, result, player.Name));
        }
        if (reply != Ask.Yes) { SendToChar(req, BuildCorpsReply(req.CharId, req.Key, reply, player.Name)); return; }
        if (!BothChiefs(player, req) || GetWarCountry(player) != GetWarCountry(req)) { Both(CorpsWrongTarget); return; }
        if (player.Party!.Arena != 0 || req.Party!.Arena != 0) { Both(CorpsBusy); return; }
        if (CheckCorpsJoin(player.Party, req.Party) is var check and not 0) { Both(check); return; }

        if (player.Party.CorpsId == 0 && req.Party.CorpsId == 0)
        {
            var corps = new Corps { Id = _state.PartyIds.Alloc(), GeneralId = req.Party.ChiefId, Commander = req.Party.Id };
            _state.CorpsMap[corps.Id] = corps;
            NotifyCorpsJoin(corps, player.Party);
            NotifyCorpsJoin(corps, req.Party);
        }
        else if (player.Party.CorpsId != 0)
        {
            if (_state.FindCorps(player.Party.CorpsId) is { } c) NotifyCorpsJoin(c, req.Party);
        }
        else if (_state.FindCorps(req.Party.CorpsId) is { } c)
            NotifyCorpsJoin(c, player.Party);
    }

    /// <summary>C++ <c>OnMW_CORPSLEAVE_ACK</c> (SSHandler.cpp:7091): a chief takes its party out, the general any party of its
    /// corps (fixed: the C++ took any party at all).</summary>
    private void OnCorpsLeave(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        ushort squadId = r.ReadUInt16();
        if (_state.FindParty(squadId) is not { } squad) return;
        if (_state.FindChar(charId, key) is not { Party: { } own } || !own.IsChief(charId)) return;
        if (_state.FindCorps(own.CorpsId) is not { } corps) return;
        if (squadId != own.Id && corps.GeneralId != charId) return;
        if (!corps.Parties.ContainsKey(squadId)) return;
        NotifyCorpsLeave(corps, squad);
    }

    /// <summary>C++ <c>NotifyCorpsJoin</c> (TWorldSvr.cpp:3438): the party and every party of the corps are shown each other's
    /// squads, then it enters.</summary>
    private void NotifyCorpsJoin(Corps corps, Party party)
    {
        foreach (var cp in corps.Parties.Values)
        {
            foreach (var m in cp.Members) SendToChar(m, BuildAddSquad(m, party));
            foreach (var m in party.Members) SendToChar(m, BuildAddSquad(m, cp));
        }
        corps.Enter(party);
        CorpsJoin(party, corps.Commander);
    }

    /// <summary>C++ <c>NotifyCorpsLeave</c> (TWorldSvr.cpp:3492): the corps that is left, or null when it ended.</summary>
    private Corps? NotifyCorpsLeave(Corps? corps, Party party)
    {
        if (corps is null) return null;
        foreach (var other in corps.Parties.Values.Where(p => p.Id != party.Id).ToList())
        {
            DelSquad(other, party.Id);
            DelSquad(party, other.Id);
        }
        if (corps.Leave(party))
        {
            NotifyCorpsLeave(corps, corps.Parties.Values.First());
            _state.CorpsMap.Remove(corps.Id);
            _state.PartyIds.Free(corps.Id);
            corps = null;
        }
        else if (corps.NextCommander(party.Id) is not null)
        {
            foreach (var p in corps.Parties.Values) CorpsJoin(p, corps.Commander);
        }
        CorpsJoin(party, 0);
        return corps;
    }

    /// <summary>C++ <c>CorpsJoin</c>: every member is told its corps and commander, and its party fields are pushed.</summary>
    private void CorpsJoin(Party party, ushort commander)
    {
        foreach (var m in party.Members)
        {
            SendToChar(m, BuildCorpsJoin(m, party.CorpsId, commander));
            PartyAttr(m);
        }
    }

    /// <summary>C++ <c>AddPartyMember</c> (TWorldSvr.cpp:3252), after the member is added: its corps, and the squads.</summary>
    private void OnPartyMemberAdded(Party party, Character ch)
    {
        if (MapOf(ch) is null) return;
        var corps = party.CorpsId != 0 ? _state.FindCorps(party.CorpsId) : null;
        if (corps is not null) SendToChar(ch, BuildCorpsJoin(ch, corps.Id, corps.Commander));
        if (corps is not null) foreach (var p in corps.Parties.Values) NotifyAddCorpsUnit(p, ch);
        else NotifyAddCorpsUnit(party, ch);
    }

    /// <summary>C++ <c>NotifyAddCorpsUnit(party, char)</c>: the joiner is shown the squad, its members the joiner.</summary>
    private void NotifyAddCorpsUnit(Party party, Character ch)
    {
        if (MapOf(ch) is null) return;
        SendToChar(ch, BuildAddSquad(ch, party));
        foreach (var m in party.Members)
        {
            if (m == ch) continue;
            var c = party.Command;
            var w = new PacketWriter(Msg.MW_ADDCORPSUNIT_REQ);
            w.WriteUInt32(m.CharId); w.WriteUInt32(m.Key);
            w.WriteUInt16(ch.Party!.Id); w.WriteUInt32(ch.CharId); w.WriteString(ch.Name); w.WriteFloat(1.0f);
            w.WriteUInt32(c.TgObjId); w.WriteUInt32(ch.MaxHP); w.WriteUInt32(ch.HP); w.WriteUInt16(c.TgPosX); w.WriteUInt16(c.TgPosZ);
            w.WriteUInt16(ch.MapId); w.WriteUInt16(0); w.WriteUInt16(0); w.WriteUInt16(0);          // pos, MOVE_NONE
            w.WriteByte(c.TgType); w.WriteByte(ch.Level); w.WriteByte(ch.Class); w.WriteByte(ch.Race); w.WriteByte(ch.Sex);
            w.WriteByte(ch.Face); w.WriteByte(ch.Hair); w.WriteByte(c.Command);
            SendToChar(m, w.ToArray());
        }
    }

    /// <summary>C++ <c>NotifyDelCorpsUnit</c> (TWorldSvr.cpp:3330): a member leaving its party is taken off every squad it shows, and
    /// sees them go.</summary>
    private void NotifyDelCorpsUnit(Corps? corps, Character ch)
    {
        if (ch.Party is not { } own) return;
        if (corps is not null) foreach (var p in corps.Parties.Values.ToList()) NotifyDelCorpsUnit(p, own, ch);
        else NotifyDelCorpsUnit(own, own, ch);
    }

    private void NotifyDelCorpsUnit(Party party, Party own, Character ch)
    {
        if (MapOf(ch) is null) return;
        foreach (var m in party.Members)
        {
            SendToChar(m, BuildDelCorpsUnit(m, own.Id, ch.CharId));
            if (m != ch) SendToChar(ch, BuildDelCorpsUnit(ch, party.Id, m.CharId));
        }
        SendToChar(ch, BuildDelSquad(ch, party.Id));
        SendToChar(ch, BuildCorpsJoin(ch, 0, 0));
    }

    private void DelSquad(Party to, ushort gone)
    {
        foreach (var m in to.Members) SendToChar(m, BuildDelSquad(m, gone));
    }

    /// <summary>C++ <c>ChgSquadChief</c> (TWorldSvr.cpp:3579): every squad showing the party learns its new chief.</summary>
    private void ChgSquadChief(Corps? corps, Party party)
    {
        foreach (var p in corps is null ? new[] { party } : corps.Parties.Values.ToArray())
            foreach (var m in p.Members)
            {
                var w = new PacketWriter(Msg.MW_CHGSQUADCHIEF_REQ);
                w.WriteUInt32(m.CharId); w.WriteUInt32(m.Key); w.WriteUInt16(party.Id); w.WriteUInt32(party.ChiefId);
                SendToChar(m, w.ToArray());
            }
    }

    /// <summary>C++ <c>ReportEnemyList</c> (TWorldSvr.cpp:3608): the first other chief is asked to tell the new chief what enemies it
    /// knows (the C++ did not check that chief is there).</summary>
    private void ReportEnemyList(Corps? corps, ushort partyId, uint chiefId)
    {
        if (corps is null) return;
        foreach (var p in corps.Parties.Values)
        {
            if (p.Id == partyId || p.Chief is not { } chief || MapOf(chief) is null) continue;
            var w = new PacketWriter(Msg.MW_REPORTENEMYLIST_REQ);
            w.WriteUInt32(chief.CharId); w.WriteUInt32(chief.Key); w.WriteUInt16(partyId); w.WriteUInt32(chiefId);
            SendToChar(chief, w.ToArray());
            return;
        }
    }

    // ================================ orders and reports ================================

    /// <summary>C++ <c>OnMW_CORPSCMD_ACK</c> (SSHandler.cpp:7125): the order is kept by the squad's chief's party and sent to every
    /// member of the corps (or of the sender's party).</summary>
    private void OnCorpsCmd(PacketReader r)
    {
        uint general = r.ReadUInt32(), key = r.ReadUInt32();
        ushort mapId = r.ReadUInt16(), squadId = r.ReadUInt16();
        uint charId = r.ReadUInt32();
        byte cmd = r.ReadByte();
        uint targetId = r.ReadUInt32();
        byte targetType = r.ReadByte();
        ushort posX = r.ReadUInt16(), posZ = r.ReadUInt16();
        if (_state.FindChar(general, key) is not { Party: { } party }) return;
        var order = new CorpsCommand(cmd, targetType, targetId, posX, posZ);
        if (_state.Characters.TryGetValue(charId, out var ch) && ch.Party is { } cp && cp.IsChief(charId)) cp.Command = order;

        var parties = _state.FindCorps(party.CorpsId) is { } corps ? corps.Parties.Values.ToList() : new List<Party> { party };
        foreach (var p in parties)
            foreach (var m in p.Members.ToList())
            {
                if (m.CharId == charId) m.Command = order;
                var w = new PacketWriter(Msg.MW_CORPSCMD_REQ);
                w.WriteUInt32(m.CharId); w.WriteUInt32(m.Key); w.WriteUInt16(squadId); w.WriteUInt32(charId); w.WriteUInt16(mapId);
                w.WriteByte(cmd); w.WriteUInt32(targetId); w.WriteByte(targetType); w.WriteUInt16(posX); w.WriteUInt16(posZ);
                SendToChar(m, w.ToArray());
            }
    }

    /// <summary>C++ <c>BroadcastCorps</c> (TWorldSvr.cpp:3284) with <c>RelayCorpsMsg</c>: the client's report goes, re-addressed, to
    /// the sender's party and — from a chief in a corps — to the other parties' chiefs.</summary>
    private void RelayCorps(PacketReader r, ushort reqId)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte[] body = r.ReadRemaining().ToArray();
        if (_state.FindChar(charId, key) is not { Party: { } party } ch) return;
        void To(Character m)
        {
            var w = new PacketWriter(reqId);
            w.WriteUInt32(m.CharId); w.WriteUInt32(m.Key); w.WriteRaw(body);
            SendToChar(m, w.ToArray());
        }
        if (party.CorpsId != 0 && party.ChiefId == ch.CharId && _state.FindCorps(party.CorpsId) is { } corps)
            foreach (var p in corps.Parties.Values)
                if (p != party && p.Chief is { } chief) To(chief);
        foreach (var m in party.Members.ToList()) To(m);
    }

    /// <summary>C++ <c>OnMW_CHGCORPSCOMMANDER_ACK</c> (SSHandler.cpp:6778): the general hands command to another party of the corps.</summary>
    private void OnChgCorpsCommander(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        ushort partyId = r.ReadUInt16();
        if (_state.FindChar(charId, key) is not { } ch) return;
        void Tell(byte result) => SendToChar(ch, BuildChgCorpsCommander(ch.CharId, ch.Key, result));
        if (ch.Party is not { } own || _state.FindCorps(own.CorpsId) is not { } corps) { Tell(CorpsNoParty); return; }
        if (own.Id != corps.Commander || ch.CharId != corps.GeneralId) { Tell(CorpsNotCommander); return; }
        if (partyId == corps.Commander) { Tell(CorpsWrongTarget); return; }
        if (!corps.Parties.TryGetValue(partyId, out var target)) { Tell(CorpsTargetNoParty); return; }

        corps.Commander = target.Id;
        corps.GeneralId = target.ChiefId;
        Tell(CorpsChgCommander);
        foreach (var p in corps.Parties.Values) CorpsJoin(p, corps.Commander);
    }

    /// <summary>C++ <c>OnMW_PARTYMOVE_ACK</c> (SSHandler.cpp:7303): the general swaps two members between their parties, or moves
    /// one into a party (the map checked it is the general).</summary>
    private void OnPartyMove(ServerSession session, PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        string target = r.ReadString(), destName = r.ReadString();
        ushort destPartyId = r.ReadUInt16();
        void Tell(byte result) => session.Send(BuildPartyMove(charId, key, result));
        if (_state.FindChar(charId, key) is null) { Tell(CorpsNotCommander); return; }
        if (CharByName(target) is not { Party: { } tgParty } tgt) { Tell(CorpsNotCommander); return; }

        if (destName.Length != 0)
        {
            if (CharByName(destName) is not { Party: { } destParty } dest || destParty.Size <= 1 || tgParty.Size <= 1) { Tell(CorpsWrongTarget); return; }
            LeaveParty(dest, 1, delete: false);
            LeaveParty(tgt, 1, delete: false);
            if (destParty.Members.FirstOrDefault() is not { } destChief || tgParty.Members.FirstOrDefault() is not { } tgChief) { Tell(CorpsWrongTarget); return; }
            JoinParty(destParty, destChief.CharId, tgt);
            JoinParty(tgParty, tgChief.CharId, dest);
        }
        else
        {
            if (_state.FindParty(destPartyId) is not { Size: > 0 } destParty || tgParty.Id == destPartyId) { Tell(CorpsNotCommander); return; }
            LeaveParty(tgt, 1, delete: true);
            if (destParty.Members.FirstOrDefault() is not { } chief) { Tell(CorpsWrongTarget); return; }
            JoinParty(destParty, chief.CharId, tgt);
        }
        Tell(CorpsSuccess);
    }

    // ================================ member recall ================================

    /// <summary>C++ <c>OnMW_PARTYMEMBERRECALL_ACK</c> (SSHandler.cpp:8710): bringing a party member on the same map (it is asked),
    /// or going to someone of one's country on the same map (no question).</summary>
    private void OnPartyMemberRecall(ServerSession session, PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte inven = r.ReadByte(), item = r.ReadByte();
        string origin = r.ReadString(), target = r.ReadString();
        if (_state.FindChar(charId, key) is not { } ch) return;

        if (origin == ch.Name)
        {
            if (CharByName(target) is { Party: { } tp } tgt && tgt.MapId == ch.MapId && tp.Id == (ch.Party?.Id ?? 0) && MapOf(tgt) is { })
            {
                tgt.PendingRecall = (origin, inven, item);
                SendToChar(tgt, BuildRecallAns(tgt, origin, TpRecall, inven, item));
            }
            else session.Send(BuildRecall(ch, IuTargetBusy, target, TpRecall));
        }
        else
        {
            if (CharByName(origin) is { } org && org.MapId == ch.MapId && GetWarCountry(org) == GetWarCountry(ch) && MapOf(org) is { })
                SendToChar(org, BuildRecallAns(org, target, TpMoveTo, inven, item));
            else session.Send(BuildRecall(ch, IuTargetBusy, origin, TpMoveTo));
        }
    }

    /// <summary>C++ <c>OnMW_PARTYMEMBERRECALLANS_ACK</c> (SSHandler.cpp:8799): the answer, with where the one answering stands, goes
    /// back to the user of the item. A recall answer is only taken from the one that was asked (fixed C++ bug).</summary>
    private void OnPartyMemberRecallAns(PacketReader r)
    {
        byte result = r.ReadByte();
        string user = r.ReadString(), target = r.ReadString();
        byte type = r.ReadByte(), inven = r.ReadByte(), item = r.ReadByte(), channel = r.ReadByte();
        ushort mapId = r.ReadUInt16();
        float x = r.ReadFloat(), y = r.ReadFloat(), z = r.ReadFloat();

        if (type == TpRecall && result != IuNotFound)
        {
            if (CharByName(target) is not { } answering || answering.PendingRecall != (user, inven, item)) return;
            answering.PendingRecall = null;
        }
        if (CharByName(user) is not { } u || MapOf(u) is null) return;
        if (u.MapId != mapId) result = IuTargetBusy;
        SendToChar(u, BuildRecall(u, result, target, type, inven, item, channel, mapId, x, y, z));
    }

    // ================================ packets ================================

    private static byte[] BuildCorpsReply(uint charId, uint key, byte result, string name)
    { var w = new PacketWriter(Msg.MW_CORPSREPLY_REQ); w.WriteUInt32(charId); w.WriteUInt32(key); w.WriteByte(result); w.WriteString(name); return w.ToArray(); }

    private static byte[] BuildCorpsJoin(Character to, ushort corpsId, ushort commander)
    { var w = new PacketWriter(Msg.MW_CORPSJOIN_REQ); w.WriteUInt32(to.CharId); w.WriteUInt32(to.Key); w.WriteUInt16(corpsId); w.WriteUInt16(commander); return w.ToArray(); }

    private static byte[] BuildDelSquad(Character to, ushort squad)
    { var w = new PacketWriter(Msg.MW_DELSQUAD_REQ); w.WriteUInt32(to.CharId); w.WriteUInt32(to.Key); w.WriteUInt16(squad); return w.ToArray(); }

    private static byte[] BuildDelCorpsUnit(Character to, ushort squad, uint target)
    { var w = new PacketWriter(Msg.MW_DELCORPSUNIT_REQ); w.WriteUInt32(to.CharId); w.WriteUInt32(to.Key); w.WriteUInt16(squad); w.WriteUInt32(target); return w.ToArray(); }

    /// <summary>C++ <c>SendMW_ADDSQUAD_REQ</c> (SSSender.cpp:1968): a party as a squad. The direction is a WORD (MOVE_NONE), as the
    /// client reads it (OnCS_ADDSQUAD_ACK).</summary>
    private static byte[] BuildAddSquad(Character to, Party squad)
    {
        var w = new PacketWriter(Msg.MW_ADDSQUAD_REQ);
        w.WriteUInt32(to.CharId); w.WriteUInt32(to.Key);
        w.WriteUInt32(squad.ChiefId); w.WriteUInt16(squad.Id); w.WriteByte(squad.Size);
        foreach (var m in squad.Members)
        {
            var c = m.Command;
            w.WriteUInt32(m.CharId); w.WriteString(m.Name); w.WriteFloat(1.0f); w.WriteUInt32(c.TgObjId);
            w.WriteUInt32(m.MaxHP); w.WriteUInt32(m.HP); w.WriteUInt16(c.TgPosX); w.WriteUInt16(c.TgPosZ);
            w.WriteUInt16(m.MapId); w.WriteUInt16((ushort)m.PosX); w.WriteUInt16((ushort)m.PosZ); w.WriteUInt16(0);   // MOVE_NONE
            w.WriteByte(c.TgType); w.WriteByte(m.Level); w.WriteByte(m.Class); w.WriteByte(m.Race); w.WriteByte(m.Sex);
            w.WriteByte(m.Face); w.WriteByte(m.Hair); w.WriteByte(c.Command);
        }
        return w.ToArray();
    }

    /// <summary>PartyAttr — sync a character's party id/type/chief/commander to its map.</summary>
    private void PartyAttr(Character ch)
    {
        var p = ch.Party;
        var w = new PacketWriter(Msg.MW_PARTYATTR_REQ);
        w.WriteUInt32(ch.CharId); w.WriteUInt32(ch.Key);
        w.WriteUInt16(p?.Id ?? 0); w.WriteByte(p?.ObtainType ?? 0); w.WriteUInt32(p?.ChiefId ?? 0); w.WriteUInt16(p is null ? (ushort)0 : CommanderOf(p));
        SendToChar(ch, w.ToArray());
    }

    private static byte[] BuildRecallAns(Character to, string user, byte type, byte inven, byte item)
    {
        var w = new PacketWriter(Msg.MW_PARTYMEMBERRECALLANS_REQ);
        w.WriteUInt32(to.CharId); w.WriteUInt32(to.Key); w.WriteString(user); w.WriteByte(type); w.WriteByte(inven); w.WriteByte(item);
        return w.ToArray();
    }

    private static byte[] BuildRecall(Character to, byte result, string name, byte type, byte inven = 0, byte item = 0,
        byte channel = 0, ushort mapId = 0, float x = 0, float y = 0, float z = 0)
    {
        var w = new PacketWriter(Msg.MW_PARTYMEMBERRECALL_REQ);
        w.WriteUInt32(to.CharId); w.WriteUInt32(to.Key); w.WriteByte(result); w.WriteString(name); w.WriteByte(type);
        w.WriteByte(inven); w.WriteByte(item); w.WriteByte(channel); w.WriteUInt16(mapId); w.WriteFloat(x); w.WriteFloat(y); w.WriteFloat(z);
        return w.ToArray();
    }

    private static byte[] BuildChgCorpsCommander(uint charId, uint key, byte ret)
    { var w = new PacketWriter(Msg.MW_CHGCORPSCOMMANDER_REQ); w.WriteUInt32(charId); w.WriteUInt32(key); w.WriteByte(ret); return w.ToArray(); }

    private static byte[] BuildPartyMove(uint charId, uint key, byte ret)
    { var w = new PacketWriter(Msg.MW_PARTYMOVE_REQ); w.WriteUInt32(charId); w.WriteUInt32(key); w.WriteByte(ret); return w.ToArray(); }
}
