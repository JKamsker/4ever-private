using TMap.Data;
using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// Events, batch S6a (C++ SSHandler.cpp:13287, 15857–16030; TMapSvr.cpp:11095–11142 and the <c>m_wEventValue</c> readers).
/// <list type="bullet">
/// <item>The world starts and ends events (<c>MW_EVENTUPDATE_REQ</c>, and on connect every running one). An event's value is a
/// percentage: exp (<see cref="EventExpAdd"/>), item / magic / money drops on its map (0xFF = all), and the crafting odds — magic,
/// rare magic, refine, option transfer, upgrade and the gamble (<c>CalcProb</c>).</item>
/// <item>A monster-spawn event spawns its spawn points at the start (start action 0) and, at its end, kills their monsters or
/// fires an AI trigger; a monster-regen event places new one-monster spawns, killed and removed at its end.</item>
/// <item>An event message (<c>MW_EVENTMSG_REQ</c>) is a world chat line from the operator; the lucky-ticket draw
/// (<c>MW_EVENTQUARTER_REQ</c>) is shown to everyone, and a player whose lucky number came out gets the quarter's prize by mail
/// (<c>TEventQuarter</c>).</item>
/// </list>
/// <para>Fixed C++ bugs: a second monster-spawn or regen event dropped the first one (<c>AddEvent</c>'s condition was always
/// true); an event id equal to <c>EVENT_COUNT</c> wrote past the value table. Not ported: the cash-sale event (no cash shop).</para>
/// </summary>
public sealed partial class MapService
{
    internal const byte EventExpAdd = 1, EventCashSale = 2, EventItemDrop = 3, EventItemMagicDrop = 4, EventRefine = 5, EventTrans = 6,
        EventItemUpgrade = 7, EventMagicUpgrade = 8, EventRareMagicUpgrade = 9, EventGambleOption = 10, EventMoneyDrop = 11,
        EventMonSpawn = 12, EventMonRegen = 13, EventCount = 16;                                // EVENT_TYPE
    private const byte AtAiComplete = 0, AtDead = 9, SmEventQuarter = 9;
    private const uint MsgNameOperator = 6;

    private sealed class MapEvent
    {
        public uint Index;
        public byte Id;
        public ushort Value, MapId;
        public byte MonStartAction, MonEndAction;
        public readonly List<ushort> SpawnIds = new();
        public readonly List<(ushort MonId, uint Delay, ushort MapId, float X, float Y, float Z)> MonRegen = new();
    }

    private readonly ushort[] _eventValue = new ushort[EventCount];
    private readonly Dictionary<uint, MapEvent> _events = new();

    /// <summary>C++ <c>m_wEventValue[id]</c>: the running value of an event kind, as a percentage.</summary>
    internal ushort EventValue(byte id) => id < EventCount ? _eventValue[id] : (ushort)0;

    /// <summary>C++ <c>GetEventInfo(id)</c> + its map check: the event's value when one of that kind runs on the map, else 0.</summary>
    private ushort EventValueOnMap(byte id, ushort mapId)
        => _events.Values.FirstOrDefault(e => e.Id == id) is { } e && (e.MapId == mapId || e.MapId == 0xFF) ? EventValue(id) : (ushort)0;

    /// <summary>C++ <c>EVENTINFO::WrapPacketOut</c>: what the map keeps of an event (the rest is the world's).</summary>
    private static MapEvent ReadEvent(PacketReader r)
    {
        var e = new MapEvent { Index = r.ReadUInt32(), Id = r.ReadByte() };
        r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadByte();                                 // state, group, server type, server id
        r.ReadInt64(); r.ReadInt64();                                                           // start, end
        e.Value = r.ReadUInt16();
        e.MapId = r.ReadUInt16();
        r.ReadUInt32(); r.ReadUInt32(); r.ReadString(); r.ReadString(); r.ReadString(); r.ReadByte(); r.ReadString();
        for (ushort n = r.ReadUInt16(), i = 0; i < n; i++) { r.ReadUInt16(); r.ReadByte(); }  // cash sale items
        e.MonStartAction = r.ReadByte();
        e.MonEndAction = r.ReadByte();
        for (ushort n = r.ReadUInt16(), i = 0; i < n; i++) e.SpawnIds.Add(r.ReadUInt16());
        for (ushort n = r.ReadUInt16(), i = 0; i < n; i++)
            e.MonRegen.Add((r.ReadUInt16(), r.ReadUInt32(), r.ReadUInt16(), r.ReadFloat(), r.ReadFloat(), r.ReadFloat()));
        for (ushort n = r.ReadUInt16(), i = 0; i < n; i++) { r.ReadUInt16(); r.ReadByte(); r.ReadUInt16(); }   // lottery items
        return e;
    }

    /// <summary>C++ <c>OnMW_EVENTUPDATE_REQ</c> (SSHandler.cpp:15857): an event starts (value ≠ 0) or ends.</summary>
    private void OnMW_EVENTUPDATE_REQ(PacketReader r)
    {
        byte id = r.ReadByte();
        ushort value = r.ReadUInt16();
        if (id >= EventCount) return;                                                          // fixed: the C++ let EVENT_COUNT through
        _eventValue[id] = value;
        var ev = ReadEvent(r);

        if (ev.Id == EventMonSpawn) MonSpawnEvent(ev.SpawnIds, value != 0 ? ev.MonStartAction : ev.MonEndAction, remove: false);
        if (ev.Id == EventMonRegen)
        {
            if (value != 0)
            {
                foreach (var (monId, delay, mapId, x, y, z) in ev.MonRegen)
                    if (RegenDynamicMonster(mapId, TcontryN, monId, x, y, z, 1, delay) is var spawnId and not 0) ev.SpawnIds.Add(spawnId);
                MonSpawnEvent(ev.SpawnIds, AtAiComplete, remove: false);
            }
            else if (_events.TryGetValue(ev.Index, out var running)) MonSpawnEvent(running.SpawnIds, AtDead, remove: true);
        }

        if (value == 0) _events.Remove(ev.Index);
        else
        {
            if (ev.Id != EventMonSpawn && ev.Id != EventMonRegen)                               // fixed: the C++ '||' was always true
                foreach (var old in _events.Values.Where(e => e.Id == ev.Id).ToList()) _events.Remove(old.Index);
            _events[ev.Index] = ev;
        }
        _log.LogInformation("Event {Id} (#{Index}) {State} at {Value}%.", ev.Id, ev.Index, value != 0 ? "on" : "off", value);
    }

    /// <summary>The spawn and regen events' monsters, on every channel: action 0 spawns the spawn points, <c>AT_DEAD</c> kills their
    /// monsters (and removes a regen event's spawns), any other trigger is fired on them.</summary>
    private void MonSpawnEvent(IEnumerable<ushort> spawnIds, byte action, bool remove)
    {
        var channels = _opt.Channels is { Length: > 0 } ? _opt.Channels : new byte[] { 1 };
        long nowMs = _tickSeconds * 1000L;
        foreach (ushort spawnId in spawnIds.ToList())
        {
            if (SpawnById(spawnId) is null) continue;
            foreach (byte channel in channels)
            {
                if (action == AtAiComplete) { AddTimelimitedMon(spawnId, channel, 0 /* RT_ETERNAL */, nowMs, TcontryN); continue; }
                foreach (var sp in _spawns.Where(p => p.Def.Spawn.Id == spawnId && p.Channel == channel).ToList())
                    foreach (var slot in sp.Slots)
                    {
                        if (slot.Live is not { Hp: > 0, Dead: false } mon) continue;
                        if (action == AtDead) { mon.Hp = 0; OnMonsterDeath(mon); }
                        else OnAiEvent(mon, (AiTrigger)action);
                    }
                if (remove && action == AtDead) DelMonSpawn(spawnId, channel);
            }
        }
    }

    /// <summary>C++ <c>OnMW_EVENTMSG_REQ</c> (SSHandler.cpp:16021): a world chat line from the operator, to every player.</summary>
    private void OnMW_EVENTMSG_REQ(PacketReader r)
    {
        r.ReadByte(); r.ReadByte();                                                             // event id, message type
        string msg = r.ReadString();
        if (msg.Length > 1024) msg = msg[..1024];
        var chat = BuildCS_CHAT_ACK(ChatGroup.World, 0, SvrMsg(MsgNameOperator), NetString(msg));
        foreach (var p in _state.AllInGame()) if (p.IsMain) p.Send(chat);
    }

    /// <summary>C++ <c>OnMW_EVENTQUARTER_REQ</c> (SSHandler.cpp:13287): the lucky-ticket draw for everyone; the players whose lucky
    /// number came out get the quarter's prize by mail (<c>TEventQuarter</c>).</summary>
    private void OnMW_EVENTQUARTER_REQ(PacketReader r)
    {
        byte day = r.ReadByte(), hour = r.ReadByte(), minute = r.ReadByte(), select = r.ReadByte();
        string present = r.ReadString();
        var msg = new PacketWriter(Msg.CS_SYSTEMMSG_ACK).WriteByte(SmEventQuarter).WriteByte(hour).WriteByte(minute).WriteString(present)
            .WriteByte(select).ToArray();
        foreach (var p in _state.AllInGame())
        {
            if (!p.IsMain || p.Char is not { } ch) continue;
            p.Send(msg);
            if (ch.LuckyNumber == select && _gameDb is { } db)
            {
                string name = ch.Name;
                _ = EnqueueDbWrite(() => db.EventQuarterAsync(name, day, hour, minute));
                _log.LogInformation("Lucky number {Number}: {Name} wins the {Hour}:{Minute:D2} prize.", select, name, hour, minute);
            }
        }
    }
}
