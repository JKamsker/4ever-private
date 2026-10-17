using System.IO;
using TWorld.Protocol;
using TWorld.Server.World;
using Xunit;

namespace TWorld.Server.Tests;

/// <summary>The world's guild checks, per the C++ (SSHandler.cpp OnMW_GUILD*_ACK, TGuild.cpp).</summary>
public class GuildAuditTests
{
    private const uint GuildId = 9;
    private const uint Ann = 4001, Bob = 4002, Cid = 4003, Dan = 4004, Tom = 4005;   // chief, vice, vice, member, outsider
    private static uint Key(uint id) => 0xABC00000 + id;
    private static readonly TimeSpan Wait = TimeSpan.FromSeconds(5);

    /// <summary>Guild 9 at level 1: Ann (chief), Bob and Cid (vice-chiefs), Dan; one baron allowed.</summary>
    private static WorldTestHost NewHost(byte level = 1) => new(state =>
    {
        var gl = new GuildLevel { Level = level, MaxCnt = 20 };
        gl.Peer[0] = 1;
        state.GuildLevels[level] = gl;
        var g = new Guild { Id = GuildId, Name = "Audit", Chief = Ann, ChiefName = "Ann", Level = level, LevelChart = gl };
        foreach (var (id, name, duty) in new[] { (Ann, "Ann", GuildDuty.Chief), (Bob, "Bob", GuildDuty.ViceChief), (Cid, "Cid", GuildDuty.ViceChief), (Dan, "Dan", GuildDuty.None) })
        {
            g.Members[id] = new GuildMember { CharId = id, Name = name, Level = 20, Duty = (byte)duty };
            state.CharGuild[id] = GuildId;
        }
        state.Guilds[GuildId] = g;
    });

    private static async Task<TcpTestClient> Open(WorldTestHost host, params (uint Id, byte Country)[] chars)
    {
        var client = await host.ConnectAsync();
        GuildTests.Connect(client);
        foreach (var (id, country) in chars)
        {
            GuildTests.EnterChar(client, id, Key(id), level: 20, country: country);
            GuildTests.ConfirmEnter(client, id, Key(id));
        }
        return client;
    }

    /// <summary>An outsider (not in the roster) given a name, as the DB would.</summary>
    private static async Task Name(WorldTestHost host, uint id, string name)
    {
        await Task.Delay(100);
        var ch = host.State.Characters[id];
        ch.Name = name;
        host.State.CharactersByName[name] = ch;
    }

    private static PacketWriter Req(ushort id, uint charId, Action<PacketWriter> body)
    {
        var w = new PacketWriter(id);
        w.WriteUInt32(charId); w.WriteUInt32(Key(charId));
        body(w);
        return w;
    }

    private static PacketReader Next(TcpTestClient c, ushort id)
    {
        for (int i = 0; i < 20; i++)
        {
            var p = c.Receive(Wait);
            if (p.Id == id) return p;
        }
        throw new InvalidOperationException($"no 0x{id:X}");
    }

    [Fact]
    public async Task Kick_AnOfficerOnlyByTheChief()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Bob, 1));
        c.Send(Req(Msg.MW_GUILDKICKOUT_ACK, Bob, w => w.WriteString("Cid")));   // a vice-chief: refused
        c.Send(Req(Msg.MW_GUILDKICKOUT_ACK, Bob, w => w.WriteString("Dan")));   // a member: kicked
        var leave = Next(c, Msg.MW_GUILDLEAVE_REQ);
        Assert.Equal(Bob, leave.ReadUInt32()); leave.ReadUInt32();
        Assert.Equal("Dan", leave.ReadString());
        Assert.Equal((byte)GuildResult.LeaveKick, leave.ReadByte());
        var g = host.State.Guilds[GuildId];
        Assert.NotNull(g.FindMember(Cid));
        Assert.Null(g.FindMember(Dan));
    }

    [Fact]
    public async Task Invite_AnotherCountry_Fails()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Ann, 1), (Tom, 2));
        await Name(host, Tom, "Tom");
        c.Send(Req(Msg.MW_GUILDINVITE_ACK, Ann, w => w.WriteString("Tom")));
        var join = Next(c, Msg.MW_GUILDJOIN_REQ);
        Assert.Equal(Ann, join.ReadUInt32()); join.ReadUInt32();
        Assert.Equal((byte)GuildResult.Fail, join.ReadByte());
    }

    [Fact]
    public async Task InviteAnswer_ARefusal_GoesToTheInviter()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Ann, 1), (Tom, 1));
        await Name(host, Tom, "Tom");
        c.Send(Req(Msg.MW_GUILDINVITEANSWER_ACK, Tom, w => { w.WriteByte(Ask.No); w.WriteUInt32(Ann); }));
        var join = Next(c, Msg.MW_GUILDJOIN_REQ);
        Assert.Equal(Ann, join.ReadUInt32()); join.ReadUInt32();
        Assert.Equal(Ask.No, join.ReadByte());
        join.ReadUInt32(); join.ReadUInt32(); join.ReadUInt32(); join.ReadString(); join.ReadUInt32();
        Assert.Equal("Tom", join.ReadString());
        Assert.Null(host.State.Guilds[GuildId].FindMember(Tom));
    }

    [Fact]
    public async Task InviteAnswer_Yes_JoinsThroughTheChief()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Ann, 1), (Tom, 1));
        await Name(host, Tom, "Tom");
        c.Send(Req(Msg.MW_GUILDINVITEANSWER_ACK, Tom, w => { w.WriteByte(Ask.Yes); w.WriteUInt32(Ann); }));
        var first = Next(c, Msg.MW_GUILDJOIN_REQ);
        var second = Next(c, Msg.MW_GUILDJOIN_REQ);
        Assert.Equal(Tom, first.ReadUInt32()); first.ReadUInt32();               // the newcomer, then the chief
        Assert.Equal((byte)GuildResult.JoinSuccess, first.ReadByte());
        Assert.Equal(Ann, second.ReadUInt32());
        Assert.NotNull(host.State.Guilds[GuildId].FindMember(Tom));
    }

    [Fact]
    public async Task Duty_NoThirdViceChief()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Ann, 1));
        c.Send(Req(Msg.MW_GUILDDUTY_ACK, Ann, w => { w.WriteString("Dan"); w.WriteByte((byte)GuildDuty.ViceChief); }));   // refused
        c.Send(Req(Msg.MW_GUILDDUTY_ACK, Ann, w => { w.WriteString("Bob"); w.WriteByte((byte)GuildDuty.None); }));
        var duty = Next(c, Msg.MW_GUILDDUTY_REQ);
        duty.ReadUInt32(); duty.ReadUInt32();
        Assert.Equal("Bob", duty.ReadString());
        Assert.Equal((byte)GuildDuty.None, duty.ReadByte());
        Assert.Equal((byte)GuildDuty.None, host.State.Guilds[GuildId].FindMember(Dan)!.Duty);
    }

    [Fact]
    public async Task Peer_TheLevelsCapRefuses()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Ann, 1));
        c.Send(Req(Msg.MW_GUILDPEER_ACK, Ann, w => { w.WriteString("Bob"); w.WriteByte(1); }));
        var ok = Next(c, Msg.MW_GUILDPEER_REQ);
        ok.ReadUInt32(); ok.ReadUInt32();
        Assert.Equal((byte)GuildResult.Success, ok.ReadByte());
        c.Send(Req(Msg.MW_GUILDPEER_ACK, Ann, w => { w.WriteString("Dan"); w.WriteByte(1); }));   // one baron at level 1
        var full = Next(c, Msg.MW_GUILDPEER_REQ);
        full.ReadUInt32(); full.ReadUInt32();
        Assert.Equal((byte)GuildResult.Fail, full.ReadByte());
    }

    [Fact]
    public void CheckPeerage_TheTopPeerageIsTheChiefs()
    {
        var gl = new GuildLevel { Level = 3, MaxCnt = 20 };
        for (int i = 0; i < 5; i++) gl.Peer[i] = 5;
        var g = new Guild { Id = 1, LevelChart = gl };
        Assert.False(WorldService.CheckPeerage(g, (byte)GuildDuty.ViceChief, 1));   // level 3-4: baron is the chief's
        Assert.True(WorldService.CheckPeerage(g, (byte)GuildDuty.Chief, 1));
        Assert.True(WorldService.CheckPeerage(g, (byte)GuildDuty.ViceChief, 2));
        Assert.False(WorldService.CheckPeerage(g, (byte)GuildDuty.Chief, 6));      // past MAX_GUILD_PEER_COUNT
    }

    [Fact]
    public async Task Articles_TheNewestTitlesTheBoard()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Ann, 1));
        c.Send(Req(Msg.MW_GUILDARTICLEADD_ACK, Ann, w => { w.WriteString("First"); w.WriteString("a"); }));
        Next(c, Msg.MW_GUILDARTICLELIST_REQ);
        c.Send(Req(Msg.MW_GUILDARTICLEADD_ACK, Ann, w => { w.WriteString("Second"); w.WriteString("b"); }));
        Next(c, Msg.MW_GUILDARTICLELIST_REQ);
        var g = host.State.Guilds[GuildId];
        Assert.Equal("Second", g.ArticleTitle);
        uint second = g.Articles.Values.Single(a => a.Title == "Second").Id;
        c.Send(Req(Msg.MW_GUILDARTICLEDEL_ACK, Ann, w => w.WriteUInt32(second)));
        Next(c, Msg.MW_GUILDARTICLELIST_REQ);
        Assert.Equal("First", g.ArticleTitle);
        Assert.Matches(@"^\d{4}-\d{2}-\d{2}$", g.Articles.Values.Single().Date);
    }

    [Fact]
    public async Task Disorg_TheSameStateIsIgnored()
    {
        await using var host = NewHost();
        using var c = await Open(host, (Ann, 1));
        c.Send(Req(Msg.MW_GUILDDISORGANIZATION_ACK, Ann, w => w.WriteByte(1)));
        Next(c, Msg.MW_GUILDDISORGANIZATION_REQ);
        c.Send(Req(Msg.MW_GUILDDISORGANIZATION_ACK, Ann, w => w.WriteByte(1)));
        Assert.ThrowsAny<IOException>(() => c.Receive(TimeSpan.FromMilliseconds(500)));
    }

    [Fact]
    public async Task WantedList_AnAdADayPastItsEnd_Expires()
    {
        await using var host = NewHost();
        host.State.GuildWanted[GuildId] = new GuildWanted { GuildId = GuildId, Country = 1, EndTime = DateTimeOffset.UtcNow.ToUnixTimeSeconds() - 2 * 86400 };
        host.State.GuildWanted[GuildId].Apps[Tom] = new GuildWantedApp { CharId = Tom, WantedId = GuildId, Name = "Tom" };
        using var c = await Open(host, (Ann, 1));
        c.Send(Req(Msg.MW_GUILDWANTEDLIST_ACK, Ann, _ => { }));
        var list = Next(c, Msg.MW_GUILDWANTEDLIST_REQ);
        list.ReadUInt32(); list.ReadUInt32();
        Assert.Equal(0u, list.ReadUInt32());
        Assert.Empty(host.State.GuildWanted);
    }
}
