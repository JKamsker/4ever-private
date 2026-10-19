using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// Corps and party member recall, batches S2/S3 — the map's side (C++ CSHandler.cpp:8064–8440, 12664–12830; SSHandler.cpp:
/// 10409, 11614–11944, 12654, 13406–13600). Corps live in the world (WorldService.Corps.cs):
/// <list type="bullet">
/// <item>The client's corps requests go on as they are (<c>MW_*_ACK</c> = the player's id and key, then the client's body; the order
/// <c>CS_CORPSCMD_REQ</c> has its map moved first), and the world's corps news come back as they are
/// (<see cref="RelayToClient"/>). A corps invitation from someone the player blocks for invites is dropped; joining a corps sets the
/// player's commanding party.</item>
/// <item><b>Member recall:</b> an <c>IK_MEMBERRECALL</c> item asks a party member on the same map to come (it answers
/// <c>CS_PARTYMEMBERRECALLANS_REQ</c>); an <c>IK_MOVETOCHAR</c> item takes the player to someone of its country on the same map.
/// Once the world agrees, the item is used and the one moving is teleported through the world (<c>CT_USERMOVE_ACK</c>, which the map
/// now also carries out — the meeting-room move needed it too).</item>
/// </list>
/// <para><b>Fixed C++ bug:</b> the new commander's enemy-list request (<c>CS_REPORTENEMYLIST_ACK</c>) went out empty (<c>&gt;&gt;</c>
/// for <c>&lt;&lt;</c>). <b>Not ported:</b> the store, tournament, lounge, meeting-room and arena gates on recall (none of them exist
/// on this map).</para>
/// </summary>
public sealed partial class MapService
{
    private const byte IkMemberRecall = 38, IkMoveToChar = 39;                                 // IK_*
    private const byte IuSuccess = 0, IuNotFound = 1, IuDealing = 5, IuRiding = 6, IuNotParty = 7, IuTargetNotFound = 9,
        IuTargetBusy = 10, IuTargetDeny = 11, IuWrapping = 14;                                  // TITEMUSE_RESULT
    private const byte TpRecall = 0, TpMoveTo = 1, TpNoneDef = 2;

    /// <summary>A corps request passed on unchanged (C++ <c>SendMW_*_ACK</c> rebuild the same bytes).</summary>
    private void CorpsToWorld(ClientSession s, PacketReader r, ushort mwId)
    {
        byte[] body = r.ReadBytes(r.Remaining);
        if (s.IsMain && s.Char is not null) SendToWorld(mwId, s, w => w.WriteRaw(body));
    }

    /// <summary>C++ <c>OnCS_CORPSCMD_REQ</c> (CSHandler.cpp:8127): the order, its map moved first.</summary>
    private void OnCS_CORPSCMD_REQ(ClientSession s, PacketReader r)
    {
        ushort squad = r.ReadUInt16();
        uint charId = r.ReadUInt32();
        ushort mapId = r.ReadUInt16();
        byte cmd = r.ReadByte();
        uint target = r.ReadUInt32();
        byte targetType = r.ReadByte();
        ushort x = r.ReadUInt16(), z = r.ReadUInt16();
        if (!s.IsMain || s.Char is null) return;
        SendToWorld(Msg.MW_CORPSCMD_ACK, s, w =>
        {
            w.WriteUInt16(mapId); w.WriteUInt16(squad); w.WriteUInt32(charId); w.WriteByte(cmd);
            w.WriteUInt32(target); w.WriteByte(targetType); w.WriteUInt16(x); w.WriteUInt16(z);
        });
    }

    /// <summary>C++ <c>OnMW_CORPSASK_REQ</c> (SSHandler.cpp:11634): asked into a corps — unless the asker is blocked for invites.</summary>
    private void OnMW_CORPSASK_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        uint askId = r.ReadUInt32();
        string askName = r.ReadString();
        if (FindPlayer(charId, key) is not { Char: { } ch } s || !CheckProtected(ch, askId, ProtectedInvite)) return;
        s.Send(new PacketWriter(Msg.CS_CORPSASK_ACK).WriteString(askName));
    }

    /// <summary>C++ <c>OnMW_CORPSJOIN_REQ</c> (SSHandler.cpp:11681): the corps joined (or left, 0) and its commanding party.</summary>
    private void OnMW_CORPSJOIN_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        ushort corps = r.ReadUInt16(), commander = r.ReadUInt16();
        if (FindPlayer(charId, key) is not { Char: { } ch } s) return;
        ch.CommanderId = commander;
        s.Send(new PacketWriter(Msg.CS_CORPSJOIN_ACK).WriteUInt16(corps).WriteUInt16(ch.GetCommanderId()));
    }

    // ================================ member recall ================================

    private static void SendCS_PARTYMEMBERRECALL_ACK(ClientSession s, byte result, byte type, string name)
        => s.Send(new PacketWriter(Msg.CS_PARTYMEMBERRECALL_ACK).WriteByte(result).WriteByte(type).WriteString(name));

    /// <summary>C++ <c>SendMW_PARTYMEMBERRECALLANS_ACK</c> (SSSender.cpp:4301) — no id and key: the world finds the user by name.</summary>
    private void SendMW_PARTYMEMBERRECALLANS_ACK(byte result, string user, string target, byte type, byte inven, byte item,
        byte channel = 0, ushort mapId = 0, float x = 0, float y = 0, float z = 0)
    {
        var w = new PacketWriter(Msg.MW_PARTYMEMBERRECALLANS_ACK);
        w.WriteByte(result); w.WriteString(user); w.WriteString(target); w.WriteByte(type); w.WriteByte(inven); w.WriteByte(item);
        w.WriteByte(channel); w.WriteUInt16(mapId); w.WriteFloat(x); w.WriteFloat(y); w.WriteFloat(z);
        _world.Send(w);
    }

    /// <summary>C++ <c>OnCS_PARTYMEMBERRECALL_REQ</c> (CSHandler.cpp:12664): an item bringing a party member, or taking the player
    /// to someone.</summary>
    private void OnCS_PARTYMEMBERRECALL_REQ(ClientSession s, PacketReader r)
    {
        byte inven = r.ReadByte(), slot = r.ReadByte();
        string name = r.ReadString();
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        void Fail(byte result, byte type = TpNoneDef) => SendCS_PARTYMEMBERRECALL_ACK(s, result, type, name);
        if (ch.ArenaId != 0 || IsTournamentMap(ch.MapId) || ch.MapId == TournamentLoungeMap) { Fail(IuTargetBusy); return; }   // not from an arena or the tournament
        if (ch.Riding != 0) { Fail(IuRiding); return; }
        if (s.Deal.Status >= DealStart) { Fail(IuDealing); return; }
        if (ch.FindInven(inven)?.FindItem(slot) is not { Count: > 0 } item || item.Template?.Type != ItUse) { Fail(IuNotFound); return; }
        if (!item.CanUse) { Fail(IuWrapping); return; }

        string origin, target;
        switch (item.Template.Kind)
        {
            case IkMemberRecall:
                if (ch.Name == name) { Fail(IuTargetNotFound, TpRecall); return; }
                if (ch.PartyId == 0) { Fail(IuNotParty, TpRecall); return; }
                (origin, target) = (ch.Name, name);
                break;
            case IkMoveToChar:
                (origin, target) = (name, ch.Name);
                break;
            default:
                Fail(IuNotFound);
                return;
        }
        SendToWorld(Msg.MW_PARTYMEMBERRECALL_ACK, s, w => { w.WriteByte(inven); w.WriteByte(slot); w.WriteString(origin); w.WriteString(target); });
    }

    /// <summary>C++ <c>OnMW_PARTYMEMBERRECALLANS_REQ</c> (SSHandler.cpp:13406): the member to bring is asked; someone to go to answers
    /// with where it stands.</summary>
    private void OnMW_PARTYMEMBERRECALLANS_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        string user = r.ReadString();
        byte type = r.ReadByte(), inven = r.ReadByte(), item = r.ReadByte();
        if (FindPlayer(charId, key) is not { Char: { } ch } s) { SendMW_PARTYMEMBERRECALLANS_ACK(IuNotFound, user, "", type, inven, item); return; }
        if (!s.IsMain || ch.Hp == 0 || s.Deal.Status >= DealStart || ch.ArenaId != 0)
        {
            SendMW_PARTYMEMBERRECALLANS_ACK(IuTargetBusy, user, ch.Name, type, inven, item);
            return;
        }
        if (type == TpRecall) s.Send(new PacketWriter(Msg.CS_PARTYMEMBERRECALLANS_ACK).WriteString(user).WriteByte(inven).WriteByte(item));
        else if (type == TpMoveTo)
            SendMW_PARTYMEMBERRECALLANS_ACK(IuSuccess, user, ch.Name, type, inven, item, s.Channel, ch.MapId, ch.PosX, ch.PosY, ch.PosZ);
    }

    /// <summary>C++ <c>OnCS_PARTYMEMBERRECALLANS_REQ</c> (CSHandler.cpp:12790): the member's answer, with where it stands.</summary>
    private void OnCS_PARTYMEMBERRECALLANS_REQ(ClientSession s, PacketReader r)
    {
        byte answer = r.ReadByte();
        string inviter = r.ReadString();
        byte inven = r.ReadByte(), item = r.ReadByte();
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        byte result = answer == 0 ? IuSuccess : answer == AskNo ? IuTargetDeny : IuTargetBusy;
        SendMW_PARTYMEMBERRECALLANS_ACK(result, inviter, ch.Name, TpRecall, inven, item, s.Channel, ch.MapId, ch.PosX, ch.PosY, ch.PosZ);
    }

    /// <summary>C++ <c>OnMW_PARTYMEMBERRECALL_REQ</c> (SSHandler.cpp:13482): the world's verdict; on success the item is used and the
    /// one moving goes, through the world (<c>CT_USERMOVE_ACK</c>).</summary>
    private void OnMW_PARTYMEMBERRECALL_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte result = r.ReadByte();
        string name = r.ReadString();
        byte type = r.ReadByte(), inven = r.ReadByte(), slot = r.ReadByte(), channel = r.ReadByte();
        ushort mapId = r.ReadUInt16();
        float x = r.ReadFloat(), y = r.ReadFloat(), z = r.ReadFloat();
        if (FindPlayer(charId, key) is not { Char: { } ch } s || s.Deal.Status >= DealStart) return;
        if (ch.ArenaId != 0) { SendCS_PARTYMEMBERRECALL_ACK(s, IuNotFound, type, name); return; }   // the arena (SSHandler.cpp:13545)
        if (result != IuSuccess) { SendCS_PARTYMEMBERRECALL_ACK(s, result, type, name); return; }
        var inv = ch.FindInven(inven);
        if (inv?.FindItem(slot) is not { } item || item.Template?.Type != ItUse) { SendCS_PARTYMEMBERRECALL_ACK(s, IuNotFound, type, name); return; }
        switch (type)
        {
            case TpRecall when item.Template.Kind == IkMemberRecall:
                SendCT_USERMOVE_ACK(name, s.Channel, ch.MapId, ch.PosX, ch.PosY, ch.PosZ);
                break;
            case TpMoveTo when item.Template.Kind == IkMoveToChar:
                SendCT_USERMOVE_ACK(ch.Name, channel, mapId, x, y, z);
                break;
            case TpRecall or TpMoveTo:
                SendCS_PARTYMEMBERRECALL_ACK(s, IuNotFound, type, name);
                return;
            default:
                if (item.Template.Kind != IkMoveToChar) { SendCS_PARTYMEMBERRECALL_ACK(s, IuNotFound, TpNoneDef, name); return; }
                break;
        }
        item.Count -= 1;                                                                        // C++ UseItem(…, 1)
        if (item.Count == 0) { inv.Items.Remove(item); SendCS_DELITEM_ACK(s, inven, item); }
        else SendCS_UPDATEITEM_ACK(s, inven, item);
        SendCS_PARTYMEMBERRECALL_ACK(s, result, type, name);
    }

    /// <summary>C++ <c>SendCT_USERMOVE_ACK</c> (SSSender.cpp:4156): the world moves the named player.</summary>
    private void SendCT_USERMOVE_ACK(string name, byte channel, ushort mapId, float x, float y, float z)
    {
        var w = new PacketWriter(Msg.CT_USERMOVE_ACK);
        w.WriteString(name); w.WriteByte(channel); w.WriteUInt16(mapId); w.WriteFloat(x); w.WriteFloat(y); w.WriteFloat(z); w.WriteUInt16(0);
        _world.Send(w);
    }

    /// <summary>C++ <c>OnCT_USERMOVE_ACK</c> (SSHandler.cpp:12654): the world moves this player (no place: to its spawn point).</summary>
    private void OnCT_USERMOVE_ACK(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte channel = r.ReadByte();
        ushort mapId = r.ReadUInt16();
        float x = r.ReadFloat(), y = r.ReadFloat(), z = r.ReadFloat();
        ushort partyId = r.ReadUInt16();
        if (FindPlayer(charId, key) is not { IsMain: true, Char: { } ch } s) return;
        if (partyId != 0) ch.PartyId = partyId;
        if (x == 0 && y == 0 && z == 0) Teleport(s, ch, ch.Persist.SpawnId);
        else Teleport(s, ch, channel, mapId, x, y, z);
    }
}
