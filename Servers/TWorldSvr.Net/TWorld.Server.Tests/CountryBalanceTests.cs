using TWorld.Protocol;
using TWorld.Server.World;
using Xunit;

namespace TWorld.Server.Tests;

/// <summary>Batch S6b: the kingdom balance — once a minute the online Defugel and Craxion players on the counted map servers go to
/// every map (<c>MW_RECALCCOUNTRYBALANCE_ACK</c>).</summary>
public class CountryBalanceTests
{
    private static void ConnectMap(TcpTestClient client)
    {
        var c = new PacketWriter(Msg.MW_CONNECT_ACK);
        c.WriteUInt16((4 << 8) | 1); c.WriteByte(1); c.WriteByte(0);
        client.Send(c);
    }

    private static PacketReader? Next(TcpTestClient map, ushort id)
    {
        for (int i = 0; i < 20; i++) { var p = map.Receive(TimeSpan.FromSeconds(5)); if (p.Id == id) return p; }
        return null;
    }

    private static void Seed(WorldState s)
    {
        s.Characters[1] = new Character { CharId = 1, Country = 0, MainId = 1 };
        s.Characters[2] = new Character { CharId = 2, Country = 0, MainId = 1 };
        s.Characters[3] = new Character { CharId = 3, Country = 1, MainId = 1 };
        s.Characters[4] = new Character { CharId = 4, Country = 1, AidCountry = 0, MainId = 1 };   // fights for Defugel
        s.Characters[5] = new Character { CharId = 5, Country = 1, MainId = 9 };
    }

    [Fact]
    public async Task EveryMinute_TheCountsGoToEveryMap()
    {
        await using var host = new WorldTestHost(Seed);
        using var map = await host.ConnectAsync();
        ConnectMap(map);
        await Task.Delay(40);
        await host.Service.OnTimerAsync();
        var r = Next(map, Msg.MW_RECALCCOUNTRYBALANCE_ACK);
        Assert.NotNull(r);
        Assert.Equal(((byte)3, (byte)2), (r!.ReadByte(), r.ReadByte()));
    }

    [Fact]
    public async Task OnlyTheCountedServers_WhenSet()
    {
        await using var host = new WorldTestHost(s => { Seed(s); s.CountryBalanceServers = new HashSet<byte> { 9 }; });
        using var map = await host.ConnectAsync();
        ConnectMap(map);
        await Task.Delay(40);
        await host.Service.OnTimerAsync();
        var r = Next(map, Msg.MW_RECALCCOUNTRYBALANCE_ACK);
        Assert.Equal(((byte)0, (byte)1), (r!.ReadByte(), r.ReadByte()));
    }
}
