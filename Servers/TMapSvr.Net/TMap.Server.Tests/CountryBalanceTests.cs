using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

/// <summary>Batch S6b: the kingdom balance — the war-balance window's relay, the stage from the world's counts, and the balance line
/// sent with the character info.</summary>
public class CountryBalanceTests
{
    private static async Task<(MapTestHarness H, ClientSession S, FakeClientChannel C)> Open()
    {
        var t = new TemplateStore();
        t.SvrMsgs[6] = "Operator";
        var h = new MapTestHarness(t);
        var (s, c) = await h.EnterAsync(1, 1, 7, name: "Ann");
        return (h, s, c);
    }

    private static byte[] Recalc(byte d, byte c) => new PacketWriter(Msg.MW_RECALCCOUNTRYBALANCE_ACK).WriteByte(d).WriteByte(c).ToArray();

    [Fact]
    public async Task CharInfo_IsPrecededByTheBalanceLine()
    {
        var (_, _, c) = await Open();
        int chat = c.Sent.FindIndex(p => PacketHeader.ReadId(p) == Msg.CS_CHAT_ACK);
        int info = c.Sent.FindIndex(p => PacketHeader.ReadId(p) == Msg.CS_CHARINFO_ACK);
        Assert.True(chat >= 0 && chat < info);
        var r = new PacketReader(c.Sent[chat]);
        Assert.Equal((byte)ChatGroup.World, r.ReadByte()); r.ReadUInt32();
        Assert.Equal("Operator", r.ReadString());
        Assert.EndsWith("Kingdom Balancing in Iberia\n Valorian: 0.0%\n Derion: 0.0%\n Current Stage: No Penalty", r.ReadString());
    }

    [Theory]
    [InlineData(50, 50, 0, 1)]
    [InlineData(53, 47, 1, 1)]     // 6-point gap: stage 1, Defugel ahead so Craxion gets the bonus
    [InlineData(44, 56, 2, 0)]     // 12 points
    [InlineData(10, 90, 3, 0)]
    [InlineData(0, 0, 0, 1)]       // nobody: no stage (the C++ divided by zero)
    public async Task Recalc_GivesTheStageAndTheBonusCountry(byte d, byte cr, byte stage, byte bonus)
    {
        var (h, _, _) = await Open();
        await h.Service.DispatchWorldAsync(Recalc(d, cr));
        Assert.Equal((stage, bonus), (h.Service.BalanceStage, h.Service.BonusCountry));
    }

    [Fact]
    public async Task Recalc_ShowsInTheNextBalanceLine()
    {
        var (h, _, _) = await Open();
        await h.Service.DispatchWorldAsync(Recalc(3, 1));
        var (_, c2) = await h.EnterAsync(2, 2, 8, name: "Bob");
        var r = new PacketReader(c2.Last(Msg.CS_CHAT_ACK)!);
        r.ReadByte(); r.ReadUInt32(); r.ReadString();
        Assert.EndsWith("Valorian: 75.0%\n Derion: 25.0%\n Current Stage: 3", r.ReadString());
    }

    [Fact]
    public async Task BalanceWindow_AsksTheWorld_AndShowsFiveAndFiveForFewPlayers()
    {
        var (h, s, c) = await Open();
        h.World.Clear();
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_WARCOUNTRYBALANCE_REQ).ToArray());
        var ask = new PacketReader(h.World.Last(Msg.MW_WARCOUNTRYBALANCE_ACK)!);
        Assert.Equal((1u, 7u), (ask.ReadUInt32(), ask.ReadUInt32()));

        foreach (var (d, cr, wantD, wantC) in new[] { (3u, 4u, 5u, 5u), (12u, 30u, 12u, 30u) })
        {
            var w = new PacketWriter(Msg.MW_WARCOUNTRYBALANCE_REQ);
            w.WriteUInt32(1); w.WriteUInt32(7); w.WriteUInt32(d); w.WriteUInt32(cr); w.WriteByte(0);
            await h.Service.DispatchWorldAsync(w.ToArray());
            var r = new PacketReader(c.Last(Msg.CS_WARCOUNTRYBALANCE_ACK)!);
            Assert.Equal((wantD, wantC), (r.ReadUInt32(), r.ReadUInt32()));
        }
    }
}
