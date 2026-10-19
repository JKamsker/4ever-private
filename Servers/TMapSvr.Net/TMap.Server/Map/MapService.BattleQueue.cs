using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// The BoW and BR queues on a normal map, batch S6f (C++ CSHandler.cpp:19678–19830, 20667–20750, 20855; SSHandler.cpp:19094–19200,
/// 19482–19650; CSSender.cpp:6611–6792). The battles themselves run on the BoW and BR map servers (<c>BOW_COMPILE_MODE</c>,
/// <c>BR_COMPILE_MODE</c> builds); the world runs the queues and teams (TWorldSvr.Net WorldService.Bow.cs / Br.cs). Here:
/// <list type="bullet">
/// <item>The client's asks are relayed to the world: join or leave the BoW queue, join the BR queue (or flag ready), invite, answer,
/// drop a BR teammate, vote for the BR map or mode, and an operator's teleport into a battle mode.</item>
/// <item>The world's answers go to the player: queue results, the BoW/BR clocks (to everyone), "the battle started without you", the
/// BR invite and the premade team.</item>
/// <item>The world's call to the battle (<c>MW_PREPAREFORBOW_REQ</c> / <c>MW_PREPAREFORBR_REQ</c>): the player's store is closed, it
/// is saved, its loot lock, summons and party are dropped, and it is teleported — to BoW spawn 20000 + team, or to the BR server.</item>
/// </list>
/// <para>What only the battle servers do (cash respawn, BP exchange, the BoW ranking, leaving the battlefield) does nothing here, as in
/// the C++. A tournament character can't queue. The meeting room the C++ checks on the call is not ported (no meeting rooms).</para>
/// </summary>
public sealed partial class MapService
{
    private const ushort BowDefaultSpawnId = 20000;                                           // BOW_DEFAULT_SPAWNID
    private const byte BrServerId = 50;                                                       // BR_SERVER_ID (sent as the channel)

    /// <summary>C++ <c>m_mapTOPERATOR</c> (<c>TOPERATORTABLE</c>): the operators' character ids.</summary>
    private bool IsOperator(uint charId) => _templates.Operators.Contains(charId);

    /// <summary>The client's queue asks, relayed to the world with the character id and key in front (C++ <c>SendMW_*</c>).</summary>
    private void OnCS_BattleQueue(ClientSession s, ushort id, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch) return;
        switch (id)
        {
            case Msg.CS_REGISTERBOW_REQ:
                if (!IsTournamentChar(ch)) SendToWorld(Msg.MW_ADDTOBOWQUEUE_REQ, s);
                break;
            case Msg.CS_CANCELBOWQUEUE_REQ:
                SendToWorld(Msg.MW_CANCELBOWQUEUE_REQ, s);
                break;
            case Msg.CS_REGISTERBR_REQ:
                byte onlyReady = r.ReadByte();
                if (!IsTournamentChar(ch)) SendToWorld(Msg.MW_ADDTOBRQUEUE_REQ, s, w => w.WriteByte(onlyReady));
                break;
            case Msg.CS_BRTEAMMATEADD_REQ:
            {
                string name = r.ReadString();
                SendToWorld(Msg.MW_BRTEAMMATEADD_REQ, s, w => w.WriteString(name));
                break;
            }
            case Msg.CS_BRTEAMMATEDEL_REQ:
            {
                string name = r.ReadString();
                SendToWorld(Msg.MW_BRTEAMMATEDEL_REQ, s, w => w.WriteString(name));
                break;
            }
            case Msg.CS_BRTEAMMATEADDRESULT_ACK:
            {
                byte result = r.ReadByte();
                string name = r.ReadString();
                SendToWorld(Msg.MW_BRTEAMMATEADDRESULT_ACK, s, w => { w.WriteByte(result); w.WriteString(name); });
                break;
            }
            case Msg.CS_VOTEFORBRMAP_REQ:
            {
                string map = r.ReadString();
                byte mode = r.ReadByte();
                SendToWorld(Msg.MW_VOTEFORBRMAP_REQ, s, w => { w.WriteString(map); w.WriteByte(mode); });
                break;
            }
            case Msg.CS_CMTELEPORTBATTLEMODE_REQ:
            {
                byte type = r.ReadByte();
                if (IsOperator(ch.CharId)) SendToWorld(Msg.MW_CMTELEPORTBATTLEMODE_REQ, s, w => w.WriteByte(type));
                break;
            }
            // CS_EXCHANGEBOWBP_REQ, CS_BOWRANKING_REQ, CS_LEAVEBATTLEFIELD_REQ: the battle servers' (nothing on a normal map).
        }
    }

    /// <summary>C++ <c>OnMW_BOWTIMEUPDATE_ACK</c> / <c>OnMW_BRTIMEUPDATE_ACK</c>: the clock, passed to everyone on the map.</summary>
    private void OnMW_BattleClock(ushort clientId, PacketReader r, int bodyLength)
    {
        var w = new PacketWriter(clientId);
        for (int i = 0; i < bodyLength; i++) w.WriteByte(r.ReadByte());
        var msg = w.ToArray();
        foreach (var s in _state.AllInGame()) if (s.IsMain) s.Send(msg);
    }

    /// <summary>C++ <c>OnMW_ADDTOBOWQUEUE_ACK</c> / <c>OnMW_CANCELBOWQUEUE_ACK</c> / <c>OnMW_ADDTOBRQUEUE_ACK</c>: result · charId · key ·
    /// tick, the result and tick to the player.</summary>
    private void OnMW_QueueResult(ushort clientId, PacketReader r)
    {
        byte result = r.ReadByte();
        uint charId = r.ReadUInt32(), key = r.ReadUInt32(), tick = r.ReadUInt32();
        if (FindPlayer(charId, key) is not { } s) return;
        s.Send(new PacketWriter(clientId).WriteByte(result).WriteUInt32(tick));
    }

    /// <summary>C++ <c>OnMW_NOTIFYNONQUEUEDPLAYER_ACK</c>: the battle started without the player.</summary>
    private void OnMW_NOTIFYNONQUEUEDPLAYER_ACK(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        FindPlayer(charId, key)?.Send(new PacketWriter(Msg.CS_NOTIFYNONQUEUEDPLAYER_ACK));
    }

    /// <summary>C++ <c>OnMW_BRTEAMMATEADD_ACK</c>: the BR invite (or its answer) to the player.</summary>
    private void OnMW_BRTEAMMATEADD_ACK(PacketReader r)
    {
        byte result = r.ReadByte();
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        string name = r.ReadString();
        FindPlayer(charId, key)?.Send(new PacketWriter(Msg.CS_BRTEAMMATEADD_ACK).WriteByte(result).WriteString(name));
    }

    /// <summary>C++ <c>OnMW_UPDATEBRTEAM_ACK</c> + <c>SendCS_UPDATEBRTEAM_ACK</c>: the premade team — the chief first (ready), then the
    /// others.</summary>
    private void OnMW_UPDATEBRTEAM_ACK(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        string chief = r.ReadString();
        byte teamReady = r.ReadByte(), count = r.ReadByte();
        if (FindPlayer(charId, key) is not { } s) return;
        var team = new List<(string Name, byte Ready)>();
        if (chief != "") team.Add((chief, 1));
        for (int i = 0; i < count; i++)
        {
            r.ReadUInt32();
            string name = r.ReadString();
            byte ready = r.ReadByte();
            if (name != chief) team.Add((name, ready));
        }
        var w = new PacketWriter(Msg.CS_UPDATEBRTEAM_ACK);
        w.WriteByte(teamReady); w.WriteByte((byte)team.Count);
        foreach (var (name, ready) in team) { w.WriteString(name); w.WriteByte(ready); }
        s.Send(w);
    }

    /// <summary>C++ <c>OnMW_PREPAREFORBOW_REQ</c> / <c>OnMW_PREPAREFORBR_REQ</c>: off to the battle.</summary>
    private void OnMW_PrepareForBattle(PacketReader r, bool br)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte team = br ? (byte)0 : r.ReadByte();
        ushort mapId = br ? r.ReadUInt16() : (ushort)0;
        if (FindPlayer(charId, key) is not { Char: { } ch } s || IsTournamentChar(ch)) return;

        StoreClose(s);
        if (s.IsMain) SaveCharData(s);
        ReleaseLootLock(ch);
        ClearRecalls(s, ch);
        if (ch.GetPartyId() != 0)
        {
            var w = new PacketWriter(Msg.MW_PARTYDEL_ACK);
            w.WriteUInt16(ch.GetPartyId()); w.WriteUInt32(ch.CharId); w.WriteByte(0);
            _world.Send(w);
        }
        if (br) Teleport(s, ch, BrServerId, mapId, UnitSize / 2, 0, UnitSize / 2);         // the BR server places it
        else Teleport(s, ch, (ushort)(BowDefaultSpawnId + team));
    }
}
