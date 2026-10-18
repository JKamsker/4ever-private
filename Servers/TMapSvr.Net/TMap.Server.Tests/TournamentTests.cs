using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

/// <summary>Batch S6e-1: the tournament's windows and signing up — the world's info and steps, the sign-up checks and costs, the
/// world's answers relayed (and a refusal refunded), the party-add and bet letters, and the cheer item.</summary>
public class TournamentTests
{
    private const ushort Permit = 6000, CheerItem = 6100;
    private const uint Fee = 10_000;

    private sealed record Setup(TemplateStore T, MapTestHarness H, FakePostStore Post, ClientSession S, FakeClientChannel C, Character Ch);

    private static async Task<Setup> Open()
    {
        var t = new TemplateStore();
        t.Items[Permit] = new ItemTemplate(Permit, 0, new[] { 1f, 0f, 0f, 0f }, Stack: 10);
        t.Items[CheerItem] = new ItemTemplate(CheerItem, 0, new[] { 1f, 0f, 0f, 0f }, Stack: 10, UseValue: 30);
        t.SpawnPositions[100] = new SpawnPosRow(100, 0, 50, 0, 50, 0);                         // home
        foreach (var (id, text) in new[] { (42u, "Bet won"), (43u, "Your winnings"), (49u, "Party"), (50u, "You were added") }) t.SvrMsgs[id] = text;
        var h = new MapTestHarness(t);
        var post = new FakePostStore();
        h.Service.PostStore = post;
        var ch = new Character { CharId = 1, Name = "Ann", Level = 50, Country = 1, Class = 2, MaxHp = 100, Hp = 100, Gold = 1 };
        ch.Invens.Add(new Inven { InvenId = 0xFF });
        var (s, c) = await h.EnterAsync(1, 1, 7, name: "Ann", preSeeded: ch);
        ch.Name = "Ann"; ch.Gold = 1; ch.Level = 50; ch.Class = 2;
        s.ClientIp = 0x0100007F;
        c.Clear(); h.World.Clear();
        return new Setup(t, h, post, s, c, ch);
    }

    private static byte[] Info(byte step, byte group = 1, byte minLevel = 40, byte maxLevel = 60, uint classMask = 0xFFFF)
    {
        var w = new PacketWriter(Msg.MW_TOURNAMENTINFO_REQ);
        w.WriteByte(17); w.WriteByte(group); w.WriteByte(step); w.WriteByte(1);
        w.WriteByte(group); w.WriteByte(1); w.WriteString("Solo"); w.WriteByte(1); w.WriteUInt32(classMask); w.WriteUInt32(Fee); w.WriteUInt32(0);
        w.WriteUInt16(Permit); w.WriteByte(2); w.WriteByte(minLevel); w.WriteByte(maxLevel);
        w.WriteByte(1); w.WriteByte(1); w.WriteUInt16(500); w.WriteByte(1);                     // one reward
        return w.ToArray();
    }

    private static byte[] Apply(byte entry = 1) => new PacketWriter(Msg.CS_TOURNAMENTAPPLY_REQ).WriteByte(entry).WriteString("HW-1").ToArray();
    private static (byte Result, byte Entry) ApplyAck(FakeClientChannel c) { var r = new PacketReader(c.Last(Msg.CS_TOURNAMENTAPPLY_ACK)!); return (r.ReadByte(), r.ReadByte()); }

    private static void GivePermits(Setup x, byte count) =>
        x.Ch.FindInven(0xFF)!.Items.Add(new Item { ItemSlot = 3, TemplateId = Permit, Count = count, Template = x.T.Item(Permit) });

    [Fact]
    public async Task Info_TellsEveryoneTheStep()
    {
        var x = await Open();
        await x.H.Service.DispatchWorldAsync(Info(step: 2));
        var r = new PacketReader(x.C.Last(Msg.CS_SYSTEMMSG_ACK)!);
        Assert.Equal((18, 1, 2, 0u, 0L), (r.ReadByte(), r.ReadByte(), r.ReadByte(), r.ReadUInt32(), r.ReadInt64()));
    }

    [Fact]
    public async Task Apply_InTheNormalStep_PaysAndAsksTheWorld()
    {
        var x = await Open();
        await x.H.Service.DispatchWorldAsync(Info(step: 2));
        GivePermits(x, 3);
        await x.H.Service.DispatchClientAsync(x.S, Apply());
        Assert.Equal(1_000_000 - Fee, x.Ch.MoneyTotal);
        Assert.Equal(1, x.Ch.FindInven(0xFF)!.FindItem(3)!.Count);
        var w = new PacketReader(x.H.World.Last(Msg.MW_TOURNAMENT_ACK)!);
        Assert.Equal((1u, 7u, Msg.MW_TOURNAMENTAPPLY_ACK), (w.ReadUInt32(), w.ReadUInt32(), w.ReadUInt16()));
        Assert.Equal((1, "HW-1", 0x0100007Fu), (w.ReadByte(), w.ReadString(), w.ReadUInt32()));
        Assert.False(x.C.Has(Msg.CS_TOURNAMENTAPPLY_ACK));                                     // the world answers
    }

    [Theory]
    [InlineData(2, 60, 70, 0xFFFF, 1, 9)]     // TOURNAMENT_LEVEL
    [InlineData(2, 40, 60, 0x1, 1, 6)]        // TOURNAMENT_CLASS (class 2 not in the mask)
    [InlineData(2, 40, 60, 0xFFFF, 0, 8)]     // TOURNAMENT_ITEM (no permit)
    [InlineData(1, 40, 60, 0xFFFF, 1, 1)]     // TOURNAMENT_DISQUALIFY: the first step is for last month's first grade
    [InlineData(4, 40, 60, 0xFFFF, 1, 2)]     // TOURNAMENT_TIMEOUT
    public async Task Apply_Refusals(byte step, byte minLevel, byte maxLevel, uint mask, byte permits, byte result)
    {
        var x = await Open();
        await x.H.Service.DispatchWorldAsync(Info(step, minLevel: minLevel, maxLevel: maxLevel, classMask: mask));
        if (permits != 0) GivePermits(x, 3);
        await x.H.Service.DispatchClientAsync(x.S, Apply());
        Assert.Equal((result, (byte)1), ApplyAck(x.C));
        Assert.Equal(1_000_000, x.Ch.MoneyTotal);
        Assert.False(x.H.World.Has(Msg.MW_TOURNAMENT_ACK));
    }

    [Fact]
    public async Task WorldRefusal_GivesTheFeeAndPermitsBack_AndIsRelayed()
    {
        var x = await Open();
        await x.H.Service.DispatchWorldAsync(Info(step: 2));
        GivePermits(x, 2);
        await x.H.Service.DispatchClientAsync(x.S, Apply());
        Assert.Null(x.Ch.FindInven(0xFF)!.FindItem(3));
        var w = new PacketWriter(Msg.MW_TOURNAMENT_REQ);
        w.WriteUInt32(1); w.WriteUInt32(7); w.WriteUInt16(Msg.MW_TOURNAMENTAPPLY_REQ); w.WriteByte(3 /* ALREADYREG */); w.WriteByte(1);
        await x.H.Service.DispatchWorldAsync(w.ToArray());
        Assert.Equal(1_000_000, x.Ch.MoneyTotal);
        Assert.Equal(2, x.Ch.FindInven(0xFF)!.Items.Single(i => i.TemplateId == Permit).Count);
        Assert.Equal((3, (byte)1), ApplyAck(x.C));
    }

    [Fact]
    public async Task Windows_AreAskedOfTheWorld_AndRelayed()
    {
        var x = await Open();
        await x.H.Service.DispatchWorldAsync(Info(step: 2));
        await x.H.Service.DispatchClientAsync(x.S, new PacketWriter(Msg.CS_TOURNAMENTAPPLYINFO_REQ).ToArray());
        var w = new PacketReader(x.H.World.Last(Msg.MW_TOURNAMENT_ACK)!);
        w.ReadUInt32(); w.ReadUInt32();
        Assert.Equal(Msg.MW_TOURNAMENTAPPLYINFO_ACK, w.ReadUInt16());

        var answer = new PacketWriter(Msg.MW_TOURNAMENT_REQ);
        answer.WriteUInt32(1); answer.WriteUInt32(7); answer.WriteUInt16(Msg.MW_TOURNAMENTSCHEDULE_REQ);
        answer.WriteByte(1); answer.WriteByte(2); answer.WriteByte(0);
        await x.H.Service.DispatchWorldAsync(answer.ToArray());
        var r = new PacketReader(x.C.Last(Msg.CS_TOURNAMENTSCHEDULE_ACK)!);
        Assert.Equal((1, 2, 0), (r.ReadByte(), r.ReadByte(), r.ReadByte()));
        Assert.Equal(0, r.Remaining);
    }

    [Fact]
    public async Task PartyAdd_TellsTheMemberByMail()
    {
        var x = await Open();
        var w = new PacketWriter(Msg.MW_TOURNAMENT_REQ);
        w.WriteUInt32(1); w.WriteUInt32(7); w.WriteUInt16(Msg.MW_TOURNAMENTPARTYADD_REQ); w.WriteByte(0); w.WriteString("Bob"); w.WriteUInt32(2);
        await x.H.Service.DispatchWorldAsync(w.ToArray());
        var mail = Assert.Single(x.Post.Mails);
        Assert.Equal((2u, "Bob", (byte)0), (mail.CharId, mail.Target, mail.Type));
        Assert.True(x.C.Has(Msg.CS_TOURNAMENTPARTYADD_ACK));
    }

    [Fact]
    public async Task BetWon_IsPaidByMail()
    {
        var x = await Open();
        var w = new PacketWriter(Msg.MW_TOURNAMENTBATPOINT_REQ);
        w.WriteUInt32(1); w.WriteString("Ann"); w.WriteUInt32(2_500_000);
        await x.H.Service.DispatchWorldAsync(w.ToArray());
        var mail = Assert.Single(x.Post.Mails);
        Assert.Equal((2u, 500u, 0u), (mail.Gold, mail.Silver, mail.Cooper));
    }

    [Fact]
    public async Task Enable_GoingBack_SendsTournamentMapPlayersHome()
    {
        var x = await Open();
        await x.H.Service.DispatchWorldAsync(Info(step: 6));
        x.Ch.MapId = 505;
        x.Ch.Persist.LastSpawnId = 100;
        x.C.Clear();
        var w = new PacketWriter(Msg.MW_TOURNAMENTENABLE_REQ);
        w.WriteByte(1); w.WriteByte(0); w.WriteUInt32(0); w.WriteInt64(0);
        await x.H.Service.DispatchWorldAsync(w.ToArray());
        Assert.True(x.C.Has(Msg.CS_SYSTEMMSG_ACK));
        Assert.True(x.C.Has(Msg.CS_BEGINTELEPORT_ACK) || x.C.Has(Msg.CS_TELEPORT_ACK));
    }
}
