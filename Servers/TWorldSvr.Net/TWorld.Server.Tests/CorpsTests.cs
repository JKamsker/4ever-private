using TWorld.Protocol;
using TWorld.Server.World;
using Xunit;

namespace TWorld.Server.Tests;

/// <summary>Batches S2/S3: corps and party member recall in the world, per the C++ (SSHandler.cpp:6778–7430, 8710–8880).</summary>
public class CorpsTests
{
    private const uint Ann = 6001, Bob = 6002, Cid = 6003, Dan = 6004;   // party 1: Ann (chief) + Bob; party 2: Cid (chief) + Dan
    private static uint Key(uint id) => 0xC0700000 + id;
    private static readonly TimeSpan Wait = TimeSpan.FromSeconds(5);

    private static async Task<(WorldTestHost Host, TcpTestClient Client, Party P1, Party P2)> Open()
    {
        var host = new WorldTestHost();
        var client = await host.ConnectAsync();
        GuildTests.Connect(client);
        var names = new[] { (Ann, "Ann"), (Bob, "Bob"), (Cid, "Cid"), (Dan, "Dan") };
        foreach (var (id, _) in names)
        {
            GuildTests.EnterChar(client, id, Key(id), level: 20, country: 1);
            GuildTests.ConfirmEnter(client, id, Key(id));
        }
        await Task.Delay(100);
        foreach (var (id, name) in names)
        {
            var ch = host.State.Characters[id];
            ch.Name = name;
            host.State.CharactersByName[name] = ch;
        }
        Party Make(ushort pid, uint chief, uint member)
        {
            var p = new Party { Id = pid, ChiefId = chief };
            p.AddMember(host.State.Characters[chief]); p.AddMember(host.State.Characters[member]);
            host.State.Parties[pid] = p;
            return p;
        }
        return (host, client, Make(0x501, Ann, Bob), Make(0x502, Cid, Dan));
    }

    private static PacketWriter Req(ushort id, uint charId, Action<PacketWriter> body)
    {
        var w = new PacketWriter(id);
        w.WriteUInt32(charId); w.WriteUInt32(Key(charId));
        body(w);
        return w;
    }

    private static List<PacketReader> Drain(TcpTestClient c, int ms = 600)
    {
        var list = new List<PacketReader>();
        var until = DateTime.UtcNow.AddMilliseconds(ms);
        while (DateTime.UtcNow < until)
            try { list.Add(c.Receive(TimeSpan.FromMilliseconds(150))); } catch (IOException) { }
        return list;
    }

    /// <summary>The recipients (char ids) of the packets with this id.</summary>
    private static List<uint> To(List<PacketReader> packets, ushort id)
        => packets.Where(p => p.Id == id).Select(p => p.ReadUInt32()).ToList();

    [Fact]
    public async Task AskingAMember_WhoIsNoChief_IsRefused()
    {
        var (host, client, _, _) = await Open();
        await using var _h = host; using var _c = client;

        client.Send(Req(Msg.MW_CORPSASK_ACK, Ann, w => w.WriteString("Dan")));

        var reply = Drain(client).Single(p => p.Id == Msg.MW_CORPSREPLY_REQ);
        Assert.Equal((Ann, Key(Ann), (byte)3, "Dan"), (reply.ReadUInt32(), reply.ReadUInt32(), reply.ReadByte(), reply.ReadString()));   // CORPS_NO_PARTY
    }

    [Fact]
    public async Task AYes_MakesACorps_UnderTheAsker_AndShowsEachSideTheOthersSquad()
    {
        var (host, client, p1, p2) = await Open();
        await using var _h = host; using var _c = client;

        client.Send(Req(Msg.MW_CORPSREPLY_ACK, Cid, w => { w.WriteByte(0); w.WriteString("Ann"); }));
        var got = Drain(client);

        var corps = host.State.CorpsMap.Values.Single();
        Assert.Equal((p1.Id, Ann), (corps.Commander, corps.GeneralId));
        Assert.Equal(corps.Id, p1.CorpsId); Assert.Equal(corps.Id, p2.CorpsId);
        Assert.Equal(new[] { Ann, Bob, Cid, Dan }, To(got, Msg.MW_ADDSQUAD_REQ).Distinct().OrderBy(x => x));
        Assert.Equal(new[] { Ann, Bob, Cid, Dan }, To(got, Msg.MW_CORPSJOIN_REQ).Distinct().OrderBy(x => x));
    }

    [Fact]
    public async Task AReportFromAChief_ReachesItsPartyAndTheOtherChiefs()
    {
        var (host, client, _, _) = await Open();
        await using var _h = host; using var _c = client;
        client.Send(Req(Msg.MW_CORPSREPLY_ACK, Cid, w => { w.WriteByte(0); w.WriteString("Ann"); }));
        Drain(client);

        client.Send(Req(Msg.MW_CORPSHP_ACK, Cid, w => { w.WriteUInt16(0x502); w.WriteUInt32(Cid); w.WriteUInt32(100); w.WriteUInt32(40); }));
        var hp = Drain(client).Where(p => p.Id == Msg.MW_CORPSHP_REQ)
            .Select(p => (To: p.ReadUInt32(), Key: p.ReadUInt32(), Body: (p.ReadUInt16(), p.ReadUInt32(), p.ReadUInt32(), p.ReadUInt32()))).ToList();

        Assert.Equal(new[] { Ann, Cid, Dan }, hp.Select(x => x.To).OrderBy(x => x));
        Assert.All(hp, x => Assert.Equal(((ushort)0x502, Cid, 100u, 40u), x.Body));
        Assert.All(hp, x => Assert.Equal(Key(x.To), x.Key));
    }

    [Fact]
    public async Task TheGeneral_CantTakeOutAPartyOfAnotherCorps()
    {
        var (host, client, p1, _) = await Open();
        await using var _h = host; using var _c = client;
        client.Send(Req(Msg.MW_CORPSREPLY_ACK, Cid, w => { w.WriteByte(0); w.WriteString("Ann"); }));
        Drain(client);
        var stranger = new Party { Id = 0x5FF, CorpsId = 0x777 };
        host.State.Parties[stranger.Id] = stranger;

        client.Send(Req(Msg.MW_CORPSLEAVE_ACK, Ann, w => w.WriteUInt16(0x5FF)));
        Drain(client);

        Assert.Equal((ushort)0x777, stranger.CorpsId);
        client.Send(Req(Msg.MW_CORPSLEAVE_ACK, Ann, w => w.WriteUInt16(0x502)));             // its own corps' party: the corps ends
        Drain(client);
        Assert.Empty(host.State.CorpsMap);
        Assert.Equal(0, p1.CorpsId);
    }

    [Fact]
    public async Task ARecallAnswer_IsOnlyTakenFromTheOneAsked()
    {
        var (host, client, _, _) = await Open();
        await using var _h = host; using var _c = client;
        PacketWriter Answer(string target) => new PacketWriter(Msg.MW_PARTYMEMBERRECALLANS_ACK)
            .WriteByte(0).WriteString("Ann").WriteString(target).WriteByte(0).WriteByte(0xFF).WriteByte(5).WriteByte(1)
            .WriteUInt16(0).WriteFloat(10).WriteFloat(0).WriteFloat(20);

        client.Send(Answer("Dan"));                                                            // nobody asked Dan
        Assert.DoesNotContain(Drain(client), p => p.Id == Msg.MW_PARTYMEMBERRECALL_REQ);

        client.Send(Req(Msg.MW_PARTYMEMBERRECALL_ACK, Ann, w => { w.WriteByte(0xFF); w.WriteByte(5); w.WriteString("Ann"); w.WriteString("Bob"); }));
        var ask = Drain(client).Single(p => p.Id == Msg.MW_PARTYMEMBERRECALLANS_REQ);
        Assert.Equal((Bob, Key(Bob), "Ann", (byte)0), (ask.ReadUInt32(), ask.ReadUInt32(), ask.ReadString(), ask.ReadByte()));
        client.Send(Answer("Bob"));
        var go = Drain(client).Single(p => p.Id == Msg.MW_PARTYMEMBERRECALL_REQ);
        Assert.Equal((Ann, Key(Ann), (byte)0, "Bob"), (go.ReadUInt32(), go.ReadUInt32(), go.ReadByte(), go.ReadString()));
    }

    [Fact]
    public async Task ANewMember_IsShownItsSquad_AndTheOthersTheNewcomer()
    {
        var (host, client, p1, _) = await Open();
        await using var _h = host; using var _c = client;
        var cid = host.State.Characters[Cid];
        host.State.Parties[0x502].DelMember(Cid);

        client.Send(new PacketWriter(Msg.MW_PARTYJOIN_ACK).WriteString("Ann").WriteString("Cid").WriteByte(0).WriteByte(0)
            .WriteUInt32(100).WriteUInt32(100).WriteUInt32(50).WriteUInt32(50));
        var got = Drain(client);

        Assert.Same(p1, cid.Party);
        Assert.Contains(Cid, To(got, Msg.MW_ADDSQUAD_REQ));
        Assert.Equal(new[] { Ann, Bob }, To(got, Msg.MW_ADDCORPSUNIT_REQ).OrderBy(x => x));
    }
}
