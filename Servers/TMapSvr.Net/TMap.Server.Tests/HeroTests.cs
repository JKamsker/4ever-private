using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

internal sealed class FakeHeroStore : IHeroStore
{
    public int HeroRet, SayRet;
    public readonly List<(uint Char, uint Guild, ushort Zone, string Hero)> Heroes = new();
    public readonly List<(byte Type, byte Month, uint Char, string Say)> Says = new();

    public Task<int> HeroSelectAsync(uint charId, uint guildId, ushort zoneId, string hero, long time)
    { Heroes.Add((charId, guildId, zoneId, hero)); return Task.FromResult(HeroRet); }

    public Task<int> SaveWarlordSayAsync(byte type, byte month, uint charId, string say)
    { Says.Add((type, month, charId, say)); return Task.FromResult(SayRet); }

    public readonly List<(byte Month, ushort Tournament, byte Entry, uint Chief, uint Winner, bool Shield)> Rewards = new();
    public TournamentRewardRow? Reward;

    public Task<TournamentRewardRow?> TournamentRewardAsync(byte month, ushort tournamentId, byte entryId, uint chiefId, uint winnerId, bool shield)
    { Rewards.Add((month, tournamentId, entryId, chiefId, winnerId, shield)); return Task.FromResult(Reward); }
}

/// <summary>Batch S6c: the hero list, naming a territory's hero, and the warlord's message.</summary>
public class HeroTests
{
    private const ushort Fort = 101, OtherFort = 102, Castle = 201;
    private const uint Guild = 77;

    private static TemplateStore Store()
    {
        var t = new TemplateStore();
        BattleZone Zone(ushort id) => new(id, "Zone", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        foreach (var id in new[] { Fort, OtherFort, Castle }) t.BattleZones[id] = Zone(id);
        t.Territories.Add(new TerritoryRow(LocalType.Occupation, Fort, 1, Guild, "Ours", 0, 0, "", 0));
        t.Territories.Add(new TerritoryRow(LocalType.Occupation, OtherFort, 0, 5, "Theirs", 0, 0, "Zed", 0));
        t.Territories.Add(new TerritoryRow(LocalType.Castle, Castle, 1, Guild, "Ours", 0, 0, "Kay", 0));
        return t;
    }

    private static async Task<(MapTestHarness H, FakeHeroStore Db, ClientSession S, FakeClientChannel C, Character Ch)> Open(byte duty = 2 /* GUILD_DUTY_CHIEF */)
    {
        var h = new MapTestHarness(Store());
        h.Service.InitTerritories();
        var db = new FakeHeroStore();
        h.Service.HeroStore = db;
        var ch = new Character { CharId = 1, Name = "Ann", Level = 50, Country = 1, MaxHp = 100, Hp = 100 };
        var (s, c) = await h.EnterAsync(1, 1, 7, name: "Ann", preSeeded: ch);
        ch.Name = "Ann"; ch.GuildId = Guild; ch.GuildDuty = duty;
        c.Clear(); h.World.Clear();
        return (h, db, s, c, ch);
    }

    private static byte[] Select(ushort zone, string hero) => new PacketWriter(Msg.CS_HEROSELECT_REQ).WriteUInt16(zone).WriteString(hero).ToArray();
    private static byte Result(FakeClientChannel c) => new PacketReader(c.Last(Msg.CS_HEROSELECT_ACK)!).ReadByte();

    [Fact]
    public async Task HeroList_ShowsTheFortsThenTheCastles()
    {
        var (h, _, s, c, _) = await Open();
        await h.Service.DispatchClientAsync(s, new PacketWriter(Msg.CS_HEROLIST_REQ).ToArray());
        var r = new PacketReader(c.Last(Msg.CS_HEROLIST_ACK)!);
        Assert.Equal(2, r.ReadByte());
        Assert.Equal((Fort, ""), (r.ReadUInt16(), r.ReadString()));
        Assert.Equal((OtherFort, "Zed"), (r.ReadUInt16(), r.ReadString()));
        Assert.Equal(1, r.ReadByte());
        Assert.Equal((Castle, "Kay"), (r.ReadUInt16(), r.ReadString()));
        Assert.Equal(0, r.Remaining);
    }

    [Fact]
    public async Task Chief_NamesAHero_AndTheWorldIsTold()
    {
        var (h, db, s, c, _) = await Open();
        await h.Service.DispatchClientAsync(s, Select(Fort, "Bob"));
        Assert.Equal(0, Result(c));
        Assert.Equal((1u, Guild, Fort, "Bob"), Assert.Single(db.Heroes));
        var w = new PacketReader(h.World.Last(Msg.MW_HEROSELECT_ACK)!);
        Assert.Equal((Fort, "Bob"), (w.ReadUInt16(), w.ReadString()));
    }

    [Theory]
    [InlineData(Fort, "", 2)]          // HSR_INVALIDCHAR
    [InlineData(Fort, "Ann", 2)]       // oneself
    [InlineData(Fort, "Zed", 4)]       // HSR_SAMENAME: already another territory's hero
    [InlineData(OtherFort, "Bob", 5)]  // HSR_NOAUTHORITY: not the guild's territory
    [InlineData(999, "Bob", 3)]        // HSR_INVALIDZONE
    public async Task BadChoices_AreRefused(ushort zone, string hero, byte result)
    {
        var (h, db, s, c, _) = await Open();
        await h.Service.DispatchClientAsync(s, Select(zone, hero));
        Assert.Equal(result, Result(c));
        Assert.Empty(db.Heroes);
    }

    [Fact]
    public async Task OnlyTheChief_NamesAHero()
    {
        var (h, db, s, c, _) = await Open(duty: 0);
        await h.Service.DispatchClientAsync(s, Select(Fort, "Bob"));
        Assert.Equal(5, Result(c));
        Assert.Empty(db.Heroes);
    }

    [Fact]
    public async Task DatabaseRefusal_IsPassedOn_AndTheWorldNotTold()
    {
        var (h, db, s, c, _) = await Open();
        db.HeroRet = 7;                                                                        // HSR_HEROEXIST
        await h.Service.DispatchClientAsync(s, Select(Fort, "Bob"));
        Assert.Equal(7, Result(c));
        Assert.False(h.World.Has(Msg.MW_HEROSELECT_ACK));
    }

    private static async Task MakeWarlord(MapTestHarness h, uint charId)
    {
        await h.Service.DispatchWorldAsync(new PacketWriter(Msg.MW_MONTHRANKLIST_REQ).WriteByte(5).WriteByte(0).ToArray());   // ladder month 5
        var w = new PacketWriter(Msg.MW_FAMERANKUPDATE_REQ);
        w.WriteByte(0); w.WriteByte(4); w.WriteByte(0);                                        // the hero list, month 4, first
        w.WriteUInt32(0); w.WriteUInt32(0); w.WriteUInt32(charId); w.WriteString("Ann");
        w.WriteUInt32(0); w.WriteUInt32(0); w.WriteUInt16(0); w.WriteUInt16(0); w.WriteUInt32(0); w.WriteUInt32(0);
        for (int i = 0; i < 7; i++) w.WriteByte(0);
        w.WriteString(""); w.WriteString("");
        await h.Service.DispatchWorldAsync(w.ToArray());
    }

    private static byte[] Say(string text) => new PacketWriter(Msg.CS_WARLORDSAY_REQ).WriteByte(0).WriteString(text).ToArray();

    [Fact]
    public async Task Warlord_LeavesAMessage_ForEveryMap()
    {
        var (h, db, s, _, _) = await Open();
        await MakeWarlord(h, 1);
        h.World.Clear();
        await h.Service.DispatchClientAsync(s, Say("For the realm!"));
        Assert.Equal(((byte)0, (byte)4, 1u, "For the realm!"), Assert.Single(db.Says));
        var w = new PacketReader(h.World.Last(Msg.MW_WARLORDSAY_ACK)!);
        Assert.Equal(((byte)0, (byte)4, 1u, "For the realm!"), (w.ReadByte(), w.ReadByte(), w.ReadUInt32(), w.ReadString()));
    }

    [Fact]
    public async Task SomeoneElse_CannotSpeakForTheWarlord()
    {
        var (h, db, s, _, _) = await Open();
        await MakeWarlord(h, 99);
        await h.Service.DispatchClientAsync(s, Say("Mine now"));
        Assert.Empty(db.Says);
        Assert.False(h.World.Has(Msg.MW_WARLORDSAY_ACK));
    }
}
