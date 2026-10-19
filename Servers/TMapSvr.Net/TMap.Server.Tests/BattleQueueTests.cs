using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

/// <summary>Batch S6f: the BoW and BR queues on a normal map — the client's asks relayed to the world, the world's answers and clocks
/// to the players, and the call to the battle.</summary>
public class BattleQueueTests
{
    private const uint Id = 7, Key = 70, Gm = 8;

    private static async Task<(MapTestHarness H, ClientSession S, FakeClientChannel C, Character Ch)> Open()
    {
        var t = new TemplateStore();
        t.SpawnPositions[1] = new SpawnPosRow(1, 0, 100, 0, 100, 0);
        t.SpawnPositions[20001] = new SpawnPosRow(20001, 300, 500, 0, 500, 0);              // BoW team 1's spawn, on another server
        t.Operators.Add(Gm);
        var h = new MapTestHarness(t);
        var ch = new Character { CharId = Id, Name = "Ann", Level = 30, MaxHp = 100, Hp = 100 };
        var (s, c) = await h.EnterAsync(Id, 1, Key, name: "Ann", x: 100, z: 100, preSeeded: ch);
        h.World.Clear(); c.Clear();
        return (h, s, c, ch);
    }

    private static (uint CharId, uint Key, PacketReader Body) World(MapTestHarness h, ushort id)
    {
        var r = new PacketReader(h.World.Last(id)!);
        return (r.ReadUInt32(), r.ReadUInt32(), r);
    }

    [Fact]
    public async Task QueueAsks_AreRelayedToTheWorld()
    {
        var (h, s, _, _) = await Open();
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_REGISTERBOW_REQ).ToArray());
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_CANCELBOWQUEUE_REQ).ToArray());
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_REGISTERBR_REQ).WriteByte(1).ToArray());
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_BRTEAMMATEADD_REQ).WriteString("Bob").ToArray());
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_BRTEAMMATEADDRESULT_ACK).WriteByte(0).WriteString("Bob").ToArray());
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_VOTEFORBRMAP_REQ).WriteString("Desert").WriteByte(2).ToArray());

        Assert.Equal((Id, Key), (World(h, Msg.MW_ADDTOBOWQUEUE_REQ).CharId, World(h, Msg.MW_ADDTOBOWQUEUE_REQ).Key));
        Assert.True(h.World.Has(Msg.MW_CANCELBOWQUEUE_REQ));
        Assert.Equal(1, World(h, Msg.MW_ADDTOBRQUEUE_REQ).Body.ReadByte());
        Assert.Equal("Bob", World(h, Msg.MW_BRTEAMMATEADD_REQ).Body.ReadString());
        var res = World(h, Msg.MW_BRTEAMMATEADDRESULT_ACK).Body;
        Assert.Equal((0, "Bob"), (res.ReadByte(), res.ReadString()));
        var vote = World(h, Msg.MW_VOTEFORBRMAP_REQ).Body;
        Assert.Equal(("Desert", 2), (vote.ReadString(), vote.ReadByte()));
    }

    [Fact]
    public async Task TheBattleModeTeleport_IsForOperatorsOnly()
    {
        var (h, s, _, ch) = await Open();
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_CMTELEPORTBATTLEMODE_REQ).WriteByte(0).ToArray());
        Assert.False(h.World.Has(Msg.MW_CMTELEPORTBATTLEMODE_REQ));
        ch.CharId = Gm;
        s.CharId = Gm;
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_CMTELEPORTBATTLEMODE_REQ).WriteByte(0).ToArray());
        Assert.True(h.World.Has(Msg.MW_CMTELEPORTBATTLEMODE_REQ));
    }

    [Fact]
    public async Task TheWorldsAnswers_ReachThePlayer()
    {
        var (h, _, c, _) = await Open();
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_ADDTOBOWQUEUE_ACK).WriteByte(3).WriteUInt32(Id).WriteUInt32(Key).WriteUInt32(90).ToArray());
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_ADDTOBRQUEUE_ACK).WriteByte(1).WriteUInt32(Id).WriteUInt32(Key).WriteUInt32(45).ToArray());
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_NOTIFYNONQUEUEDPLAYER_ACK).WriteUInt32(Id).WriteUInt32(Key).ToArray());
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_BRTEAMMATEADD_ACK).WriteByte(0).WriteUInt32(Id).WriteUInt32(Key).WriteString("Bob").ToArray());
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_BOWTIMEUPDATE_ACK).WriteByte(2).WriteUInt32(600).WriteByte(4).WriteByte(5).ToArray());

        var bow = new PacketReader(c.Last(Msg.CS_REGISTERBOW_ACK)!);
        Assert.Equal((3, 90u), (bow.ReadByte(), bow.ReadUInt32()));
        var br = new PacketReader(c.Last(Msg.CS_ADDTOBRQUEUE_ACK)!);
        Assert.Equal((1, 45u), (br.ReadByte(), br.ReadUInt32()));
        Assert.True(c.Has(Msg.CS_NOTIFYNONQUEUEDPLAYER_ACK));
        var invite = new PacketReader(c.Last(Msg.CS_BRTEAMMATEADD_ACK)!);
        Assert.Equal((0, "Bob"), (invite.ReadByte(), invite.ReadString()));
        var clock = new PacketReader(c.Last(Msg.CS_BOWTIMEUPDATE_ACK)!);
        Assert.Equal((2, 600u, 4, 5), (clock.ReadByte(), clock.ReadUInt32(), clock.ReadByte(), clock.ReadByte()));
    }

    [Fact]
    public async Task ThePremadeTeam_ListsTheChiefFirst_Once()
    {
        var (h, _, c, _) = await Open();
        var w = new PacketWriter(Msg.MW_UPDATEBRTEAM_ACK);
        w.WriteUInt32(Id); w.WriteUInt32(Key); w.WriteString("Ann"); w.WriteByte(0); w.WriteByte(2);
        w.WriteUInt32(Id); w.WriteString("Ann"); w.WriteByte(1);
        w.WriteUInt32(9); w.WriteString("Bob"); w.WriteByte(0);
        await h.Service.DispatchWorldAsync(w.ToArray());

        var r = new PacketReader(c.Last(Msg.CS_UPDATEBRTEAM_ACK)!);
        Assert.Equal((0, 2), (r.ReadByte(), r.ReadByte()));
        Assert.Equal(("Ann", 1), (r.ReadString(), r.ReadByte()));
        Assert.Equal(("Bob", 0), (r.ReadString(), r.ReadByte()));
    }

    [Fact]
    public async Task TheCallToBow_LeavesTheParty_AndTeleportsToTheTeamsSpawn()
    {
        var (h, _, c, ch) = await Open();
        ch.PartyId = 12; ch.PartyType = 0;
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_PREPAREFORBOW_REQ).WriteUInt32(Id).WriteUInt32(Key).WriteByte(1).ToArray());

        var del = new PacketReader(h.World.Last(Msg.MW_PARTYDEL_ACK)!);
        Assert.Equal(((ushort)12, Id, 0), (del.ReadUInt16(), del.ReadUInt32(), del.ReadByte()));
        var tp = new PacketReader(h.World.Last(Msg.MW_BEGINTELEPORT_ACK)!);
        tp.ReadUInt32(); tp.ReadUInt32(); tp.ReadByte(); tp.ReadByte();
        Assert.Equal((ushort)300, tp.ReadUInt16());
        Assert.True(c.Has(Msg.CS_BEGINTELEPORT_ACK));
    }

    [Fact]
    public async Task TheCallToBr_GoesToTheBrServer()
    {
        var (h, _, _, _) = await Open();
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_PREPAREFORBR_REQ).WriteUInt32(Id).WriteUInt32(Key).WriteUInt16(400).ToArray());

        var tp = new PacketReader(h.World.Last(Msg.MW_BEGINTELEPORT_ACK)!);
        tp.ReadUInt32(); tp.ReadUInt32(); tp.ReadByte();
        Assert.Equal((50, (ushort)400, 512f), (tp.ReadByte(), tp.ReadUInt16(), tp.ReadFloat()));
    }
}
