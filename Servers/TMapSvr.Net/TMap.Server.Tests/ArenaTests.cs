using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

/// <summary>Batch S6d: the arena — signing up, the ready and battle clocks, the result and its mail, giving up, leaving, the queue,
/// and the info and ranking windows.</summary>
public class ArenaTests
{
    private const ushort Npc = 26156, InPos = 17000, OutPos = 17001;
    private const uint Fee = 50_000;

    private sealed record Fighter(ClientSession S, FakeClientChannel C, Character Ch);
    private sealed record Setup(MapTestHarness H, FakePostStore Post, Fighter A, Fighter B, Fighter C, Fighter D, Fighter E, Fighter F);

    private static TemplateStore Store()
    {
        var t = new TemplateStore();
        t.Arenas.Add(new ArenaRow(1, 3, Fee, InPos, OutPos));
        t.SpawnPositions[InPos] = new SpawnPosRow(InPos, 0, 100, 0, 100, 0);
        t.SpawnPositions[OutPos] = new SpawnPosRow(OutPos, 0, 130, 0, 100, 0);
        var npc = new NpcDef(Npc, 24, 3, 0, 0, 0, 0, 0, 0, 105, 0, 105);
        npc.ItemIds.Add(1);
        t.Npcs[Npc] = npc;
        t.SvrMsgs[47] = "Arena win"; t.SvrMsgs[48] = "Well fought";
        return t;
    }

    private static async Task<Setup> Open()
    {
        var h = new MapTestHarness(Store());
        h.Service.InitNpcs();
        var post = new FakePostStore();
        h.Service.PostStore = post;
        async Task<Fighter> Pc(uint id, string name, ushort party, uint chief)
        {
            var ch = new Character { CharId = id, Name = name, Level = 30, Country = 1, MaxHp = 100, Hp = 100, Gold = 1 };
            var (s, c) = await h.EnterAsync(id, id, 100 + id, name: name, x: 110, z: 110, preSeeded: ch);
            ch.Name = name; ch.Gold = 1; ch.PartyId = party; ch.PartyChiefId = chief; ch.PartyType = 0;
            return new Fighter(s, c, ch);
        }
        var a = await Pc(1, "Ann", 10, 1); var b = await Pc(2, "Bob", 10, 1);
        var c = await Pc(3, "Cid", 20, 3); var d = await Pc(4, "Dee", 20, 3);
        var e = await Pc(5, "Eve", 30, 5); var f = await Pc(6, "Fay", 30, 5);
        h.World.Clear();
        return new Setup(h, post, a, b, c, d, e, f);
    }

    private static byte[] Req(byte command, ushort arena = 0) => new PacketWriter(Msg.CS_ARENA_REQ).WriteByte(command).WriteUInt16(Npc).WriteUInt16(arena).ToArray();
    private static byte Ack(FakeClientChannel c) => new PacketReader(c.Last(Msg.CS_ARENA_ACK)!).ReadByte();
    private static async Task Ticks(MapTestHarness h, int n) { for (int i = 0; i < n; i++) await h.Service.OnTimerAsync(); }

    [Fact]
    public async Task Chief_SignsUp_TheParty_IsSentInside()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.A.S, Req(0));
        Assert.Equal((1, 1, 1, 1), (x.A.Ch.ArenaId, x.A.Ch.ArenaTeam, x.B.Ch.ArenaId, x.B.Ch.ArenaTeam));
        Assert.Equal(1_000_000 - Fee, x.A.Ch.MoneyTotal);
        Assert.Equal((100f, 100f), (x.A.Ch.PosX, x.A.Ch.PosZ));
        var team = new PacketReader(x.C.C.Last(Msg.CS_ARENATEAM_ACK)!);                         // everyone on the map sees the team
        Assert.Equal((1, 1, 2), (team.ReadUInt16(), team.ReadByte(), team.ReadByte()));
        var w = new PacketReader(x.H.World.Last(Msg.MW_ARENAJOIN_ACK)!);
        Assert.Equal((1u, 1), (w.ReadUInt32(), (int)(w.ReadUInt32() - 100)));
        Assert.Equal((1, 2u), (w.ReadByte(), w.ReadUInt32()));
    }

    [Fact]
    public async Task Refusals()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.B.S, Req(0));
        Assert.Equal(4, Ack(x.B.C));                                                            // ARENA_CHIEF
        x.B.Ch.PartyId = 11; x.B.Ch.PartyChiefId = 2;                                          // Ann alone in her party
        await x.H.Service.DispatchClientAsync(x.A.S, Req(0));
        Assert.Equal(6, Ack(x.A.C));                                                            // ARENA_MINPARTY
        x.C.Ch.Gold = 0; x.C.Ch.Silver = 1;
        await x.H.Service.DispatchClientAsync(x.C.S, Req(0));
        Assert.Equal(2, Ack(x.C.C));                                                            // ARENA_MONEY
        Assert.Equal(0, x.C.Ch.ArenaId);
    }

    [Fact]
    public async Task TwoTeams_Ready_ThenBattle_ThenTwoTies_GoToTheFirstSide()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.A.S, Req(0));
        await x.H.Service.DispatchClientAsync(x.C.S, Req(0));
        Assert.Equal(2, x.C.Ch.ArenaTeam);
        var cd = new PacketReader(x.A.C.Last(Msg.CS_SYSTEMMSG_ACK)!);
        Assert.Equal((28, 1, 60u, 3), (cd.ReadByte(), cd.ReadUInt16(), cd.ReadUInt32(), cd.ReadByte()));   // ready, 60 s

        await Ticks(x.H, 60);                                                                   // the battle starts
        cd = new PacketReader(x.A.C.Last(Msg.CS_SYSTEMMSG_ACK)!);
        Assert.Equal((28, 1, 240u, 1), (cd.ReadByte(), cd.ReadUInt16(), cd.ReadUInt32(), cd.ReadByte()));

        await Ticks(x.H, 240);                                                                  // time: 2 against 2, a replay
        Assert.Equal(1, x.C.Ch.ArenaId);
        Assert.Empty(x.Post.Mails);
        await Ticks(x.H, 300);                                                                  // ready + battle again: a second tie
        Assert.Equal((0, 0), (x.C.Ch.ArenaId, x.D.Ch.ArenaId));                                // the second side is out
        Assert.Equal((130f, 100f), (x.C.Ch.PosX, x.C.Ch.PosZ));
        Assert.Equal(1, x.A.Ch.ArenaId);
        var mail = Assert.Single(x.Post.Mails);
        Assert.Equal((1u, 0u, 50u, 0u), (mail.CharId, mail.Gold, mail.Silver, mail.Cooper));
    }

    [Fact]
    public async Task AWipedOutSide_Loses()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.A.S, Req(0));
        await x.H.Service.DispatchClientAsync(x.C.S, Req(0));
        await Ticks(x.H, 60);
        x.C.Ch.Hp = 0;
        x.D.Ch.Hp = 0;
        await Ticks(x.H, 240);                                                                  // time with the second side down
        Assert.Equal(0, x.C.Ch.ArenaId);
        Assert.Equal(1u, Assert.Single(x.Post.Mails).CharId);
        Assert.True(x.C.Ch.Hp > 0);                                                             // everyone is healed at the end
    }

    [Fact]
    public async Task GivingUpAWaitingArena_SendsThePartyOut_AndRefunds()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.A.S, Req(0));
        await x.H.Service.DispatchClientAsync(x.A.S, Req(1, 1));
        Assert.Equal((0, 0), (x.A.Ch.ArenaId, x.B.Ch.ArenaId));
        Assert.Equal(Fee, (uint)(Assert.Single(x.Post.Mails).Silver * 1000));
    }

    [Fact]
    public async Task LeavingTheParty_EndsATwoFighterTeam()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.A.S, Req(0));
        await x.H.Service.DispatchClientAsync(x.C.S, Req(0));
        var w = new PacketWriter(Msg.MW_PARTYATTR_REQ);
        w.WriteUInt32(4); w.WriteUInt32(104); w.WriteUInt16(0); w.WriteByte(0); w.WriteUInt32(0); w.WriteUInt16(0);
        await x.H.Service.DispatchWorldAsync(w.ToArray());                                     // Dee leaves her party
        Assert.Equal((0, 0), (x.C.Ch.ArenaId, x.D.Ch.ArenaId));
        Assert.Equal(1, x.A.Ch.ArenaId);
    }

    [Fact]
    public async Task AThirdParty_Waits_InTheQueue_AndComesInNext()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.A.S, Req(0));
        await x.H.Service.DispatchClientAsync(x.C.S, Req(0));
        await x.H.Service.DispatchClientAsync(x.E.S, Req(0));
        Assert.Equal(0, x.E.Ch.ArenaId);
        var info = new PacketReader(x.E.C.Last(Msg.CS_ARENAINFO_ACK)!);
        Assert.Equal((1u, 1u), (info.ReadUInt32(), info.ReadUInt32()));
        Assert.Equal("Ann", info.ReadString()); info.ReadByte(); info.ReadByte(); info.ReadUInt32(); info.ReadByte();
        Assert.Equal("Cid", info.ReadString()); info.ReadByte(); info.ReadByte();
        Assert.Equal((1, "Eve"), (info.ReadByte(), info.ReadString()));
        Assert.Equal(0, info.Remaining);

        await Ticks(x.H, 60);
        x.C.Ch.Hp = 0; x.D.Ch.Hp = 0;
        await Ticks(x.H, 240);                                                                  // the second side loses ...
        Assert.Equal((2, 2), (x.E.Ch.ArenaTeam, x.F.Ch.ArenaTeam));                            // ... and Eve's party takes its place
        Assert.Equal(1_000_000 - Fee, x.E.Ch.MoneyTotal);
    }

    [Fact]
    public async Task Ranking_WithOneArena_SaysNothingForTheOther()
    {
        var x = await Open();
        await x.H.Service.DispatchClientAsync(x.A.S, new PacketWriter(Msg.CS_ARENARANKING_REQ).WriteByte(1).ToArray());
        var r = new PacketReader(x.A.C.Last(Msg.CS_ARENARANKING_ACK)!);
        Assert.Equal((0, 0, 0, 0), (r.ReadByte(), r.ReadByte(), r.ReadByte(), r.ReadByte()));
        Assert.Equal(0, r.Remaining);
    }
}
