using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

internal sealed class FakeProtectedStore : IProtectedStore
{
    public readonly Dictionary<uint, List<ProtectedRow>> Rows = new();
    public readonly Dictionary<string, uint> Ids = new();
    public readonly List<(uint Char, uint Target)> Deleted = new();
    public readonly List<(uint Char, uint Target, byte Option)> Options = new();

    public Task<List<ProtectedRow>> LoadProtectedAsync(uint charId) => Task.FromResult(Rows.GetValueOrDefault(charId) ?? new());

    public Task<(int Ret, uint ProtectedId)> ProtectedInsertAsync(uint charId, string name, byte option)
    {
        if (!Ids.TryGetValue(name, out uint id)) return Task.FromResult((1, 0u));
        var list = Rows.TryGetValue(charId, out var l) ? l : Rows[charId] = new();
        if (list.Any(p => p.CharId == id)) return Task.FromResult((2, id));
        list.Add(new ProtectedRow(id, name, option));
        return Task.FromResult((0, id));
    }

    public Task ProtectedDeleteAsync(uint charId, uint protectedId) { lock (Deleted) Deleted.Add((charId, protectedId)); return Task.CompletedTask; }

    public Task SaveProtectedOptionAsync(uint charId, uint protectedId, byte option) { lock (Options) Options.Add((charId, protectedId, option)); return Task.CompletedTask; }
}

/// <summary>Social, batch S1: the protected list (kept by the map, checked by chat, invites, trades and duels) and the map's side of
/// friends and the messenger.</summary>
public class SocialTests
{
    private const uint Ann = 1, Bob = 2;
    private const byte Chat = 1, Invite = 2, Deal = 4, Duel = 8;

    private static async Task<(MapTestHarness h, FakeProtectedStore db, ClientSession a, FakeClientChannel ca, ClientSession b, FakeClientChannel cb)>
        Setup(Action<FakeProtectedStore>? seed = null)
    {
        var h = new MapTestHarness(new TemplateStore());
        var db = new FakeProtectedStore();
        db.Ids["Ann"] = Ann; db.Ids["Bob"] = Bob;
        seed?.Invoke(db);
        h.Service.ProtectedStore = db;
        Character Pc(uint id, string name) => new() { CharId = id, Name = name, Level = 30, MaxHp = 100, Hp = 100, MaxMp = 50, Mp = 50 };
        var (a, ca) = await h.EnterAsync(Ann, 1, 1, x: 100, z: 100, name: "Ann", preSeeded: Pc(Ann, "Ann"));
        var (b, cb) = await h.EnterAsync(Bob, 2, 2, x: 101, z: 100, name: "Bob", preSeeded: Pc(Bob, "Bob"));
        a.Char!.Name = "Ann"; b.Char!.Name = "Bob";
        ca.Clear(); cb.Clear(); h.World.Clear();
        return (h, db, a, ca, b, cb);
    }

    private static byte[] Cs(ushort id, Action<PacketWriter>? body = null) { var w = new PacketWriter(id); body?.Invoke(w); return w.ToArray(); }

    private static byte[] World(ushort id, ClientSession s, Action<PacketWriter> body)
    {
        var w = new PacketWriter(id);
        w.WriteUInt32(s.CharId); w.WriteUInt32(s.Key);
        body(w);
        return w.ToArray();
    }

    private static void Block(ClientSession s, uint who, string name, byte option) => s.Char!.Protected[who] = new ProtectedEntry { Name = name, Option = option };

    private static async Task Until(Func<bool> done)
    {
        for (int i = 0; i < 100 && !done(); i++) await Task.Delay(10);
    }

    // ================================ the protected list ================================

    [Fact]
    public async Task Entering_ReadsTheList_AndSendsIt()
    {
        var h = new MapTestHarness(new TemplateStore());
        var db = new FakeProtectedStore();
        db.Rows[Ann] = new() { new ProtectedRow(Bob, "Bob", Chat | Duel) };
        h.Service.ProtectedStore = db;
        var (a, ca) = await h.EnterAsync(Ann, 1, 1, name: "Ann", preSeeded: new Character { CharId = Ann, Name = "Ann" });

        Assert.Equal((byte)(Chat | Duel), a.Char!.Protected[Bob].Option);
        var r = new PacketReader(ca.Last(Msg.CS_PROTECTEDLIST_ACK)!);
        Assert.Equal((1, Bob, "Bob", (byte)(Chat | Duel)), (r.ReadByte(), r.ReadUInt32(), r.ReadString(), r.ReadByte()));
    }

    [Fact]
    public async Task Adding_SavesIt_RefusesChat_AndTellsTheWorld()
    {
        var (h, db, a, ca, _, _) = await Setup();

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_PROTECTEDADD_REQ, w => w.WriteString("Bob")));

        Assert.Equal(Chat, a.Char!.Protected[Bob].Option);
        Assert.Single(db.Rows[Ann]);
        var ack = new PacketReader(ca.Last(Msg.CS_PROTECTEDADD_ACK)!);
        Assert.Equal(((byte)0, Bob, "Bob", Chat), (ack.ReadByte(), ack.ReadUInt32(), ack.ReadString(), ack.ReadByte()));
        var mw = new PacketReader(h.World.Last(Msg.MW_PROTECTEDCHECK_ACK)!);
        Assert.Equal((a.CharId, a.Key, (byte)1, "Bob"), (mw.ReadUInt32(), mw.ReadUInt32(), mw.ReadByte(), mw.ReadString()));   // FRIEND_DISCONNECTION
    }

    [Theory]
    [InlineData("Ann", 3)]      // oneself: PROTECTED_FAIL
    [InlineData("", 3)]
    [InlineData("Nobody", 1)]   // the proc: no such character
    public async Task Adding_IsRefused(string name, byte result)
    {
        var (h, _, a, ca, _, _) = await Setup();

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_PROTECTEDADD_REQ, w => w.WriteString(name)));

        Assert.Equal(result, new PacketReader(ca.Last(Msg.CS_PROTECTEDADD_ACK)!).ReadByte());
        Assert.Empty(a.Char!.Protected);
        Assert.False(h.World.Has(Msg.MW_PROTECTEDCHECK_ACK));
    }

    [Fact]
    public async Task Erasing_TakesItOff_AndTellsTheWorld()
    {
        var (h, db, a, ca, _, _) = await Setup();
        Block(a, Bob, "Bob", Chat);

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_PROTECTEDERASE_REQ, w => w.WriteString("Cid")));
        Assert.Equal(1, new PacketReader(ca.Last(Msg.CS_PROTECTEDERASE_ACK)!).ReadByte());       // PROTECTED_NOTFOUND
        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_PROTECTEDERASE_REQ, w => w.WriteString("Bob")));

        Assert.Equal(0, new PacketReader(ca.Last(Msg.CS_PROTECTEDERASE_ACK)!).ReadByte());
        Assert.Empty(a.Char!.Protected);
        var mw = new PacketReader(h.World.Last(Msg.MW_PROTECTEDCHECK_ACK)!);
        mw.ReadUInt32(); mw.ReadUInt32();
        Assert.Equal(((byte)0, "Bob"), (mw.ReadByte(), mw.ReadString()));                    // FRIEND_CONNECTION
        await Until(() => db.Deleted.Count > 0);
        Assert.Equal((Ann, Bob), db.Deleted.Single());
    }

    [Fact]
    public async Task TheOption_IsChanged_AndSaved()
    {
        var (h, db, a, _, _, _) = await Setup();
        Block(a, Bob, "Bob", Chat);

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_PROTECTEDOPTION_REQ, w => { w.WriteUInt32(Bob); w.WriteByte(Chat | Deal); }));

        Assert.Equal((byte)(Chat | Deal), a.Char!.Protected[Bob].Option);
        await Until(() => db.Options.Count > 0);
        Assert.Equal((Ann, Bob, (byte)(Chat | Deal)), db.Options.Single());
    }

    [Fact]
    public async Task ChatFromSomeoneBlocked_NeverArrives_ButTheSenderSeesItsOwn()
    {
        var (h, _, a, ca, b, cb) = await Setup();
        Block(b, Ann, "Ann", Chat);
        byte[] Say(byte group, string to) => Cs(Msg.CS_CHAT_REQ, w => { w.WriteString("Ann"); w.WriteByte(group); w.WriteUInt32(0); w.WriteString(to); w.WriteString("hi"); });

        await h.Service.DispatchClientAsync(a, Say((byte)ChatGroup.Near, ""));
        await h.Service.DispatchClientAsync(a, Say((byte)ChatGroup.Whisper, "Bob"));

        Assert.False(cb.Has(Msg.CS_CHAT_ACK));
        Assert.Equal(2, ca.WithId(Msg.CS_CHAT_ACK).Count());
    }

    [Fact]
    public async Task ATradeOrADuel_IsRefused_ByWhatTheTargetBlocks()
    {
        var (h, _, a, ca, b, cb) = await Setup();
        Block(b, Ann, "Ann", Deal | Duel);

        await h.Service.DispatchClientAsync(a, MapTestHarness.DealAskReq("Bob"));
        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_DUELINVITE_REQ, w => w.WriteUInt32(Bob)));

        Assert.False(cb.Has(Msg.CS_DEALITEMASK_ACK));
        Assert.Equal((byte)DealResult.Deny, new PacketReader(ca.Last(Msg.CS_DEALITEMEND_ACK)!).ReadByte());
        Assert.False(cb.Has(Msg.CS_DUELINVITE_ACK));
        Assert.Equal(1, new PacketReader(ca.Last(Msg.CS_DUELSTART_ACK)!).ReadByte());            // DUEL_REFUSE
    }

    [Fact]
    public async Task APartyInvite_FromSomeoneBlocked_IsAnsweredNo()
    {
        var (h, _, _, _, b, cb) = await Setup();
        Block(b, Ann, "Ann", Invite);

        await h.Service.DispatchWorldAsync(World(Msg.MW_PARTYADD_REQ, b, w =>
        {
            w.WriteString("Ann"); w.WriteString("Ann"); w.WriteByte(2); w.WriteByte(0 /* PARTY_AGREE */); w.WriteUInt32(Ann);
        }));

        Assert.False(cb.Has(Msg.CS_PARTYJOINASK_ACK));
        var r = new PacketReader(h.World.Last(Msg.MW_PARTYJOIN_ACK)!);
        r.ReadString(); r.ReadString(); r.ReadByte();
        Assert.Equal(1, r.ReadByte());                                                             // ASK_NO
    }

    [Fact]
    public async Task AGuildInvite_FromSomeoneBlocked_IsDenied()
    {
        var (h, _, a, ca, b, cb) = await Setup();
        Block(b, Ann, "Ann", Invite);

        await h.Service.DispatchWorldAsync(World(Msg.MW_GUILDINVITE_REQ, b, w => { w.WriteString("Wolves"); w.WriteUInt32(Ann); w.WriteString("Ann"); }));

        Assert.False(cb.Has(Msg.CS_GUILDINVITE_ACK));
        Assert.Equal(1, new PacketReader(ca.Last(Msg.CS_GUILDINVITE_ACK)!).ReadByte());          // GUILD_JOIN_DENY
    }

    // ================================ friends ================================

    [Fact]
    public async Task AFriendRequest_GoesToTheWorld_UnlessTheNameIsBlocked()
    {
        var (h, _, a, ca, _, _) = await Setup();

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_FRIENDASK_REQ, w => w.WriteString("Bob")));
        var r = new PacketReader(h.World.Last(Msg.MW_FRIENDASK_ACK)!);
        Assert.Equal((a.CharId, a.Key, "Bob"), (r.ReadUInt32(), r.ReadUInt32(), r.ReadString()));

        h.World.Clear();
        Block(a, Bob, "Bob", Chat);
        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_FRIENDASK_REQ, w => w.WriteString("Bob")));
        Assert.False(h.World.Has(Msg.MW_FRIENDASK_ACK));
        var ack = new PacketReader(ca.Last(Msg.CS_FRIENDADD_ACK)!);
        Assert.Equal(((byte)1, 0u, "Bob"), (ack.ReadByte(), ack.ReadUInt32(), ack.ReadString()));   // FRIEND_REFUSE
    }

    [Fact]
    public async Task TheOneAsked_WhoBlocksTheAsker_LetsTheWorldRefuse()
    {
        var (h, _, _, _, b, cb) = await Setup();

        await h.Service.DispatchWorldAsync(World(Msg.MW_FRIENDASK_REQ, b, w => { w.WriteString("Ann"); w.WriteUInt32(Ann); }));
        Assert.Equal("Ann", new PacketReader(cb.Last(Msg.CS_FRIENDASK_ACK)!).ReadString());

        cb.Clear();
        Block(b, Ann, "Ann", Chat);
        await h.Service.DispatchWorldAsync(World(Msg.MW_FRIENDASK_REQ, b, w => { w.WriteString("Ann"); w.WriteUInt32(Ann); }));
        Assert.False(cb.Has(Msg.CS_FRIENDASK_ACK));
        var r = new PacketReader(h.World.Last(Msg.MW_FRIENDPROTECTEDASK_ACK)!);
        Assert.Equal((b.CharId, b.Key, "Ann"), (r.ReadUInt32(), r.ReadUInt32(), r.ReadString()));
    }

    [Fact]
    public async Task TheWorldsAnswers_ReachTheClient_AsTheyCame()
    {
        var (h, _, a, ca, _, _) = await Setup();

        await h.Service.DispatchWorldAsync(World(Msg.MW_FRIENDLIST_REQ, a, w =>
        {
            w.WriteUInt32(0); w.WriteByte(1); w.WriteByte(3); w.WriteString("Pals");
            w.WriteByte(1); w.WriteUInt32(Bob); w.WriteString("Bob"); w.WriteByte(30); w.WriteByte(3); w.WriteByte(2); w.WriteByte(1); w.WriteUInt32(7);
        }));
        await h.Service.DispatchWorldAsync(World(Msg.MW_FRIENDCONNECTION_REQ, a, w => { w.WriteByte(0); w.WriteString("Bob"); w.WriteUInt32(7); }));

        var list = new PacketReader(ca.Last(Msg.CS_FRIENDLIST_ACK)!);
        Assert.Equal((0u, (byte)1, (byte)3, "Pals"), (list.ReadUInt32(), list.ReadByte(), list.ReadByte(), list.ReadString()));
        Assert.Equal(((byte)1, Bob, "Bob"), (list.ReadByte(), list.ReadUInt32(), list.ReadString()));
        var con = new PacketReader(ca.Last(Msg.CS_FRIENDCONNECTION_ACK)!);
        Assert.Equal(((byte)0, "Bob", 7u), (con.ReadByte(), con.ReadString(), con.ReadUInt32()));
    }

    [Fact]
    public async Task AGroupRenamed_WithTooLongAName_IsRefused_AndNotPassedOn()
    {
        var (h, _, a, ca, _, _) = await Setup();
        string longName = new('x', 51);

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_FRIENDGROUPNAME_REQ, w => { w.WriteByte(2); w.WriteString(longName); }));

        Assert.Equal(3, new PacketReader(ca.Last(Msg.CS_FRIENDGROUPNAME_ACK)!).ReadByte());       // FRIEND_NOTFOUND
        Assert.False(h.World.Has(Msg.MW_FRIENDGROUPNAME_ACK));
    }

    // ================================ the messenger ================================

    [Fact]
    public async Task AnInvite_GoesToTheWorld_WithEveryTarget()
    {
        var (h, _, a, _, _, _) = await Setup();

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_TMSINVITE_REQ, w => { w.WriteUInt32(0); w.WriteByte(2); w.WriteUInt32(Bob); w.WriteUInt32(9); }));

        var r = new PacketReader(h.World.Last(Msg.MW_TMSINVITE_ACK)!);
        Assert.Equal((a.CharId, a.Key, 0u, (byte)2, Bob, 9u), (r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32(), r.ReadByte(), r.ReadUInt32(), r.ReadUInt32()));
    }

    [Theory]
    [InlineData((byte)0, (byte)1)]
    [InlineData(Invite, (byte)0)]
    public async Task BringingBackALeaver_AsksItsProtectedList(byte blocked, byte result)
    {
        var (h, _, a, _, b, _) = await Setup();
        if (blocked != 0) Block(b, Ann, "Ann", blocked);

        await h.Service.DispatchWorldAsync(World(Msg.MW_TMSINVITEASK_REQ, b, w => { w.WriteUInt32(Ann); w.WriteUInt32(a.Key); w.WriteUInt32(5); w.WriteString("hey"); }));

        var r = new PacketReader(h.World.Last(Msg.MW_TMSINVITEASK_ACK)!);
        Assert.Equal((Ann, a.Key, Bob, b.Key), (r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32()));
        Assert.Equal((result, 5u, "hey"), (r.ReadByte(), r.ReadUInt32(), r.ReadString()));
    }

    [Fact]
    public async Task TheWorldsMessages_ReachTheClient()
    {
        var (h, _, a, ca, _, _) = await Setup();

        await h.Service.DispatchWorldAsync(World(Msg.MW_TMSRECV_REQ, a, w => { w.WriteUInt32(5); w.WriteString("Bob"); w.WriteString("hi"); }));

        var r = new PacketReader(ca.Last(Msg.CS_TMSRECV_ACK)!);
        Assert.Equal((5u, "Bob", "hi"), (r.ReadUInt32(), r.ReadString(), r.ReadString()));
    }
}
