using System.Net.Sockets;
using TLogin.Protocol;

namespace TBot;

/// <summary>
/// A server-plane tool for the scenarios: it talks to the world server directly (plaintext, as the control server does) to force a
/// war phase — C++ <c>SM_BATTLESTATUS_REQ</c> (<c>bType, bStatus, dwStart, dwSecond</c>), which the world sends on to every map as
/// its <c>MW_*ENABLE_REQ</c>. The world's own schedule is left alone.
/// </summary>
internal static class WorldTool
{
    public const ushort SM_BATTLESTATUS_REQ = 0x1581 + 0x001C;
    public const byte BtLocal = 0, BtCastle = 1, BtMission = 3, BtSkyGarden = 4;
    public const byte BsNormal = 0, BsBattle = 1, BsPeace = 4;

    public static string Host { get; set; } = "127.0.0.1";
    public static int Port { get; set; } = 3816;

    public const ushort CT_CASTLEGUILDCHG_REQ = 0x9301 + 0x005B;
    public const ushort CT_CHARMSG_ACK = 0x9301 + 0x003D, CT_HELPMESSAGE_REQ = 0x9301 + 0x0073;

    /// <summary>C++ <c>CT_CHARMSG_ACK</c> (the operator's tool): a line for one player, from the operator.</summary>
    public static void CharMsg(string name, string msg)
    {
        var w = new PacketWriter(CT_CHARMSG_ACK);
        w.WriteString(name); w.WriteString(msg);
        Send(w);
    }

    /// <summary>C++ <c>CT_HELPMESSAGE_REQ</c> (the operator's tool): a help message from <paramref name="start"/> to <paramref name="end"/>
    /// (unix seconds); the world saves it and tells every map.</summary>
    public static void HelpMessage(byte id, long start, long end, string msg)
    {
        var w = new PacketWriter(CT_HELPMESSAGE_REQ);
        w.WriteByte(id); w.WriteInt64(start); w.WriteInt64(end); w.WriteString(msg);
        Send(w);
    }

    /// <summary>C++ <c>CT_CASTLEGUILDCHG_REQ</c> (the operator's tool): a castle's defending and attacking guilds and its next war
    /// (unix seconds) — the world tells every map.</summary>
    public static void CastleGuildChg(ushort castle, uint defGuild, uint atkGuild, long nextWar)
    {
        using var tcp = new TcpClient(Host, Port);
        var w = new PacketWriter(CT_CASTLEGUILDCHG_REQ);
        w.WriteUInt16(castle); w.WriteUInt32(defGuild); w.WriteUInt32(atkGuild); w.WriteUInt32(0); w.WriteInt64(nextWar);
        tcp.GetStream().Write(w.ToArray());
        tcp.GetStream().Flush();
        Thread.Sleep(300);
    }

    public const ushort CT_EVENTUPDATE_REQ = 0x9301 + 0x005F, CT_EVENTMSG_REQ = 0x9301 + 0x0065, SM_EVENTQUARTER_REQ = 0x1581 + 0x0017;

    private static void Send(PacketWriter w)
    {
        using var tcp = new TcpClient(Host, Port);
        tcp.GetStream().Write(w.ToArray());
        tcp.GetStream().Flush();
        Thread.Sleep(300);                                              // let the world read it before the socket closes
    }

    /// <summary>C++ <c>CT_EVENTUPDATE_REQ</c> (the operator's tool): an event starts (value ≠ 0) or ends — the world tells every map.
    /// The body is C++ <c>EVENTINFO::WrapPacketIn</c>; <paramref name="regen"/> places one-monster spawns (a regen event).</summary>
    public static void EventUpdate(byte id, ushort value, uint index, ushort mapId = 0xFF,
        IReadOnlyList<(ushort Mon, uint Delay, ushort Map, float X, float Y, float Z)>? regen = null)
    {
        var w = new PacketWriter(CT_EVENTUPDATE_REQ);
        w.WriteByte(id); w.WriteUInt16(value);
        w.WriteUInt32(index); w.WriteByte(id); w.WriteByte(1); w.WriteByte(0); w.WriteByte(0); w.WriteByte(0);
        w.WriteInt64(0); w.WriteInt64(0); w.WriteUInt16(value); w.WriteUInt16(mapId); w.WriteUInt32(0); w.WriteUInt32(0);
        w.WriteString(""); w.WriteString(""); w.WriteString("TBot event"); w.WriteByte(0); w.WriteString("");
        w.WriteUInt16(0);                                               // cash sale items
        w.WriteByte(0); w.WriteByte(9);                                 // monster start action (spawn) / end action (AT_DEAD)
        w.WriteUInt16(0);                                               // spawn ids
        w.WriteUInt16((ushort)(regen?.Count ?? 0));
        foreach (var g in regen ?? Array.Empty<(ushort, uint, ushort, float, float, float)>())
        { w.WriteUInt16(g.Mon); w.WriteUInt32(g.Delay); w.WriteUInt16(g.Map); w.WriteFloat(g.X); w.WriteFloat(g.Y); w.WriteFloat(g.Z); }
        w.WriteUInt16(0);                                               // lottery items
        Send(w);
    }

    /// <summary>C++ <c>CT_EVENTMSG_REQ</c>: an event message for every player.</summary>
    public static void EventMsg(string msg)
    {
        var w = new PacketWriter(CT_EVENTMSG_REQ);
        w.WriteByte(1); w.WriteByte(0); w.WriteString(msg);
        Send(w);
    }

    /// <summary>C++ <c>SM_EVENTQUARTER_REQ</c> (the world's own lucky-ticket timer): draw a lucky number for a quarter.</summary>
    public static void EventQuarter(byte day, byte hour, byte minute, string present)
    {
        var w = new PacketWriter(SM_EVENTQUARTER_REQ);
        w.WriteByte(day); w.WriteByte(hour); w.WriteByte(minute); w.WriteString(present);
        Send(w);
    }

    public static void BattleStatus(byte type, byte status, uint start, uint second)
    {
        using var tcp = new TcpClient(Host, Port);
        var w = new PacketWriter(SM_BATTLESTATUS_REQ);
        w.WriteByte(type); w.WriteByte(status); w.WriteUInt32(start); w.WriteUInt32(second);
        tcp.GetStream().Write(w.ToArray());
        tcp.GetStream().Flush();
        Thread.Sleep(300);                                              // let the world read it before the socket closes
    }
}
