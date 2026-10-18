using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

internal sealed class FakeAuctionStore : IAuctionStore
{
    public readonly List<AuctionRow> Rows = new();
    public readonly List<(uint Seller, ItemSaveData Item)> Saved = new();
    public readonly List<uint> Interest = new();
    public int RegRet, BidRet, BuyRet, CancelRet;
    public AuctionEndResult End;
    public int EndCalls;
    public uint NextId = 500;
    public (uint Id, string Name, long Price) Bidder;

    public Task<List<AuctionRow>> LoadAuctionsAsync() => Task.FromResult(Rows.ToList());
    public Task<AuctionLists> LoadAuctionListsAsync(uint charId) => Task.FromResult(new AuctionLists(new(), new(), new()));
    public Task SaveAuctionItemAsync(uint seller, ItemSaveData item) { Saved.Add((seller, item)); return Task.CompletedTask; }

    public Task<AuctionRegResult> AuctionRegAsync(ushort npcId, ushort hours, uint seller, long direct, long start, long dlId)
        => Task.FromResult(new AuctionRegResult(RegRet, RegRet == 0 ? NextId++ : 0, 1_000, 1_000 + hours * 3600L));

    public Task<AuctionCancelResult> AuctionRegCancelAsync(ushort npcId, uint auctionId)
        => Task.FromResult(new AuctionCancelResult(CancelRet, "Ann", Bidder.Name, Bidder.Id, Bidder.Price));

    public Task<AuctionBidResult> AuctionBidAsync(ushort npcId, uint auctionId, uint bidder, long bid)
    {
        var drop = Bidder;
        if (BidRet == 0) Bidder = (bidder, "", bid);
        return Task.FromResult(new AuctionBidResult(BidRet, drop.Price, drop.Id, drop.Name, 2_000));
    }

    public Task<AuctionBuyResult> AuctionBuyDirectAsync(ushort npcId, uint auctionId, uint buyer, byte count, long unitPrice)
        => Task.FromResult(new AuctionBuyResult(BuyRet, 1, "Ann", Bidder.Id, Bidder.Name, Bidder.Price));

    public Task<AuctionEndResult> AuctionEndAsync(ushort npcId, uint auctionId) { EndCalls++; return Task.FromResult(End); }

    public Task SaveAuctionInterestAsync(uint charId, IReadOnlyList<uint> ids)
    {
        lock (Interest) { Interest.Clear(); Interest.AddRange(ids); }
        return Task.CompletedTask;
    }
}

/// <summary>Batch S4: the auction house — selling, bidding, buying, the end and the lists, with the letters they send.</summary>
public class AuctionTests
{
    private const uint Ann = 1, Bob = 2;
    private const ushort Npc = 20841, Sword = 100, Potion = 200;
    private static readonly ItemTemplate SwordTpl = new(Sword, 0, new[] { 1f, 0f, 0f, 0f }, Type: 1, DefaultLevel: 10, Stack: 1, IsSell: 1, Code: 0x01_02_03);
    private static readonly ItemTemplate PotionTpl = new(Potion, 0, new[] { 1f, 0f, 0f, 0f }, Type: 7, Stack: 50, IsSell: 1, Code: 0x05_00_01);

    private sealed record Setup(MapTestHarness H, FakeAuctionStore Db, FakePostStore Post, ClientSession A, FakeClientChannel Ca, Character Ann,
        ClientSession B, FakeClientChannel Cb, Character Bob);

    private static async Task<Setup> Open()
    {
        var t = new TemplateStore();
        t.Items[Sword] = SwordTpl; t.Items[Potion] = PotionTpl;
        t.SvrMsgs[13] = "Auctioneer"; t.SvrMsgs[22] = "You won %s."; t.SvrMsgs[24] = "Your %s is back.";
        var h = new MapTestHarness(t);
        var db = new FakeAuctionStore();
        var post = new FakePostStore();
        h.Service.AuctionStore = db; h.Service.PostStore = post;
        h.Service.ApplyItemIdSeed(1000);
        h.Service.AddNpc(new Npc { Id = Npc, Type = 10, Country = 1, MapId = 0 });
        Character Pc(uint id, string name)
        {
            var ch = new Character { CharId = id, Name = name, Level = 20, Country = 1, MaxHp = 100, Hp = 100, Gold = 1 };
            ch.Invens.Add(new Inven { InvenId = 0xFF });
            return ch;
        }
        var ann = Pc(Ann, "Ann"); var bob = Pc(Bob, "Bob");
        var (a, ca) = await h.EnterAsync(Ann, 11, 1, x: 100, z: 100, name: "Ann", preSeeded: ann);
        var (b, cb) = await h.EnterAsync(Bob, 12, 2, x: 101, z: 100, name: "Bob", preSeeded: bob);
        ann.Name = "Ann"; bob.Name = "Bob"; ann.Country = bob.Country = 1; ann.Gold = bob.Gold = 1;   // 1 000 000
        a.UserId = 11; b.UserId = 12;
        ca.Clear(); cb.Clear();
        return new Setup(h, db, post, a, ca, ann, b, cb, bob);
    }

    private static byte[] Cs(ushort id, Action<PacketWriter> body) { var w = new PacketWriter(id); body(w); return w.ToArray(); }

    private static byte[] Reg(byte slot, long direct, long start, ushort hours = 24) => Cs(Msg.CS_AUCTIONREG_REQ, w =>
    {
        w.WriteUInt16(Npc); w.WriteUInt16(hours); w.WriteByte(0xFF); w.WriteByte(slot); w.WriteInt64(direct); w.WriteInt64(start); w.WriteUInt32(0);
    });

    private static void Give(Character ch, byte slot, ItemTemplate tpl, byte count = 1)
        => ch.FindInven(0xFF)!.Items.Add(new Item { ItemSlot = slot, TemplateId = tpl.ItemId, Count = count, Template = tpl, DlId = 77 });

    private static (byte Result, uint Id) RegAck(FakeClientChannel c) { var r = new PacketReader(c.Last(Msg.CS_AUCTIONREG_ACK)!); return (r.ReadByte(), r.ReadUInt32()); }
    private static byte ByteAck(FakeClientChannel c, ushort id) => new PacketReader(c.Last(id)!).ReadByte();

    /// <summary>Ann's sword up at 10 000 direct, 1 000 start.</summary>
    private static async Task<uint> Sell(Setup x, long direct = 10_000, long start = 1_000)
    {
        Give(x.Ann, 5, SwordTpl);
        await x.H.Service.DispatchClientAsync(x.A, Reg(5, direct, start));
        return RegAck(x.Ca).Id;
    }

    private static byte[] Find(byte align = 0) => Cs(Msg.CS_AUCTIONFIND_REQ, w =>
    {
        w.WriteUInt16(Npc); w.WriteByte(0); w.WriteByte(255); w.WriteByte(0); w.WriteByte(255); w.WriteByte(0);
        w.WriteUInt16(1); w.WriteByte(10); w.WriteByte(align); w.WriteUInt16(0);
    });

    private static List<(uint Id, string Seller, long Direct, long Bid, byte Bids, ushort Item)> ReadList(byte[] packet, out byte result)
    {
        var r = new PacketReader(packet);
        result = r.ReadByte();
        var list = new List<(uint, string, long, long, byte, ushort)>();
        if (r.Remaining == 0) return list;
        r.ReadUInt16(); r.ReadUInt16();
        ushort n = r.ReadUInt16();
        for (int i = 0; i < n; i++)
        {
            uint id = r.ReadUInt32(); string seller = r.ReadString(); r.ReadInt64(); r.ReadUInt32();
            long direct = r.ReadInt64(); r.ReadInt64(); long bid = r.ReadInt64(); byte bids = r.ReadByte(); r.ReadUInt32();
            r.ReadByte(); ushort item = r.ReadUInt16();
            r.ReadByte(); r.ReadByte(); r.ReadUInt16(); r.ReadUInt16(); r.ReadByte(); r.ReadUInt32(); r.ReadUInt32(); r.ReadByte(); r.ReadByte();
            r.ReadByte(); r.ReadInt64(); r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadUInt16(); r.ReadUInt16(); r.ReadByte();
            for (byte m = r.ReadByte(), k = 0; k < m; k++) { r.ReadByte(); r.ReadUInt16(); }
            list.Add((id, seller, direct, bid, bids, item));
        }
        Assert.Equal(0, r.Remaining);
        return list;
    }

    [Fact]
    public async Task Selling_TakesTheItemAndTwoPercent_AndListsIt()
    {
        var x = await Open();

        uint id = await Sell(x);

        Assert.Equal((0, 500u), RegAck(x.Ca));
        Assert.Empty(x.Ann.FindInven(0xFF)!.Items);
        Assert.Equal(1_000_000 - 200, x.Ann.MoneyTotal);
        Assert.Equal(Ann, x.Db.Saved.Single().Seller);
        Assert.Contains(id, x.Ann.AuctionReg);
        await x.H.Service.DispatchClientAsync(x.B, Find());
        var list = ReadList(x.Cb.Last(Msg.CS_AUCTIONFIND_ACK)!, out byte res);
        Assert.Equal((0, id, "Ann", 10_000L, 1_000L, Sword), (res, list.Single().Id, list[0].Seller, list[0].Direct, list[0].Bid, list[0].Item));
    }

    [Theory]
    [InlineData(1_000L, 2_000L, 4)]     // AR_INVALIDPRICE: direct below start
    [InlineData(5_000L, 0L, 5)]         // AR_INVALIDSTARTPRICE
    [InlineData(5_000L, 5_000L, 4)]     // a single item can't have direct = start
    [InlineData(100_000_000L, 1L, 9)]   // AR_NEEDMONEY: 2% of 100 000 000
    public async Task Selling_IsRefused(long direct, long start, byte result)
    {
        var x = await Open();
        Give(x.Ann, 5, SwordTpl);

        await x.H.Service.DispatchClientAsync(x.A, Reg(5, direct, start));

        Assert.Equal(result, RegAck(x.Ca).Result);
        Assert.Single(x.Ann.FindInven(0xFF)!.Items);
    }

    [Fact]
    public async Task ARefusedRegistration_MailsTheItemAndTheFeeBack()
    {
        var x = await Open();
        x.Db.RegRet = 6;

        await Sell(x);

        Assert.Equal(6, RegAck(x.Ca).Result);
        var mail = x.Post.Mails.Single();
        Assert.Equal((Ann, "Auctioneer", 200u), (mail.CharId, mail.Sender, mail.Cooper));
        Assert.Equal((ushort)Sword, mail.Items.Single().TemplateId);
        Assert.Contains("Your ", mail.Message);
    }

    [Fact]
    public async Task ABid_MustBeatTheBestByFivePercent_AndRefundsTheOneOutbid()
    {
        var x = await Open();
        uint id = await Sell(x);
        byte[] Bid(long price) => Cs(Msg.CS_AUCTIONBID_REQ, w => { w.WriteUInt16(Npc); w.WriteUInt32(id); w.WriteInt64(price); });

        await x.H.Service.DispatchClientAsync(x.B, Bid(1_040));
        Assert.Equal(5, ByteAck(x.Cb, Msg.CS_AUCTIONBID_ACK));                                   // ABR_LOWPRICE: 1 000 + 5%
        await x.H.Service.DispatchClientAsync(x.A, Bid(2_000));
        Assert.Equal(4, ByteAck(x.Ca, Msg.CS_AUCTIONBID_ACK));                                   // ABR_SELF

        x.Db.Bidder = (99, "Cid", 1_500);
        await x.H.Service.DispatchClientAsync(x.B, Bid(2_000));
        Assert.Equal(0, ByteAck(x.Cb, Msg.CS_AUCTIONBID_ACK));
        Assert.Equal(1_000_000 - 2_000, x.Bob.MoneyTotal);
        Assert.Contains(id, x.Bob.AuctionBid); Assert.Contains(id, x.Bob.AuctionInterest);
        var refund = x.Post.Mails.Single();
        Assert.Equal((99u, "Cid", 1u, 500u), (refund.CharId, refund.Target, refund.Silver, refund.Cooper));

        await x.H.Service.DispatchClientAsync(x.B, Bid(5_000));
        Assert.Equal(8, ByteAck(x.Cb, Msg.CS_AUCTIONBID_ACK));                                   // ABR_DUPLICATE: its own bid
    }

    [Fact]
    public async Task BuyingAtTheDirectPrice_PaysTheSellerLessThreePercent_AndMailsTheItem()
    {
        var x = await Open();
        uint id = await Sell(x);
        x.Post.Mails.Clear();

        await x.H.Service.DispatchClientAsync(x.B, Cs(Msg.CS_AUCTIONBUYDIRECT_REQ, w => { w.WriteUInt16(Npc); w.WriteUInt32(id); w.WriteByte(1); }));

        Assert.Equal(0, ByteAck(x.Cb, Msg.CS_AUCTIONBUYDIRECT_ACK));
        Assert.Equal(1_000_000 - 10_000, x.Bob.MoneyTotal);
        var paid = x.Post.Mails.Single(m => m.CharId == Ann);
        Assert.Equal((9u, 700u), (paid.Silver, paid.Cooper));                                   // 10 000 − 3%
        var got = x.Post.Mails.Single(m => m.CharId == Bob);
        Assert.Equal((Sword, 1), (got.Items.Single().TemplateId, (int)got.Items.Single().Count));
        await x.H.Service.DispatchClientAsync(x.B, Find());
        ReadList(x.Cb.Last(Msg.CS_AUCTIONFIND_ACK)!, out byte res);
        Assert.Equal(3, res);                                                                   // ALR_NOLIST
    }

    [Fact]
    public async Task UnitsOfAStack_AreSoldAtTheStartPrice_TheRestStays()
    {
        var x = await Open();
        Give(x.Ann, 6, PotionTpl, 10);
        await x.H.Service.DispatchClientAsync(x.A, Reg(6, 1_000, 100));
        uint id = RegAck(x.Ca).Id;

        await x.H.Service.DispatchClientAsync(x.B, Cs(Msg.CS_AUCTIONBUYDIRECT_REQ, w => { w.WriteUInt16(Npc); w.WriteUInt32(id); w.WriteByte(3); }));

        Assert.Equal(1_000_000 - 300, x.Bob.MoneyTotal);
        Assert.Equal(3, x.Post.Mails.Single(m => m.CharId == Bob).Items.Single().Count);
        await x.H.Service.DispatchClientAsync(x.B, Find());
        var left = ReadList(x.Cb.Last(Msg.CS_AUCTIONFIND_ACK)!, out _).Single();
        Assert.Equal(700L, left.Direct);
    }

    [Fact]
    public async Task Cancelling_SendsTheItemBack_AndRefundsTheBid()
    {
        var x = await Open();
        uint id = await Sell(x);
        x.Db.Bidder = (Bob, "Bob", 3_000);

        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_AUCTIONREGCANCEL_REQ, w => { w.WriteUInt16(Npc); w.WriteUInt32(id); }));

        Assert.Equal(0, ByteAck(x.Ca, Msg.CS_AUCTIONREGCANCEL_ACK));
        Assert.Single(x.Post.Mails.Single(m => m.CharId == Ann).Items);
        Assert.Equal(3u, x.Post.Mails.Single(m => m.CharId == Bob).Silver);
        Assert.DoesNotContain(id, x.Ann.AuctionReg);
    }

    [Fact]
    public async Task TheEnd_GivesTheItemToTheBestBidder_OrTriesAgainLater()
    {
        var x = await Open();
        await Sell(x);
        x.Post.Mails.Clear();
        x.H.Service.UnixNow = () => 1_000 + 24 * 3600;

        x.Db.End = new AuctionEndResult(2, 0, 0, "", "", 0);                                   // the database's clock not there yet
        await x.H.Service.RunAuctionEnds();
        await x.H.Service.RunAuctionEnds();
        Assert.Equal(1, x.Db.EndCalls);
        Assert.Empty(x.Post.Mails);

        x.H.Service.UnixNow = () => 1_000 + 24 * 3600 + 61;
        x.Db.End = new AuctionEndResult(0, Bob, Ann, "Bob", "Ann", 4_850);
        await x.H.Service.RunAuctionEnds();
        Assert.Equal(4_850u, x.Post.Mails.Single(m => m.CharId == Ann).Silver * 1000 + x.Post.Mails.Single(m => m.CharId == Ann).Cooper);
        Assert.Single(x.Post.Mails.Single(m => m.CharId == Bob).Items);
    }

    [Fact]
    public async Task TheLetter_CarriesTheItemAsALink()
    {
        var x = await Open();
        await Sell(x);
        x.Db.End = new AuctionEndResult(0, Bob, Ann, "Bob", "Ann", 1);
        x.H.Service.UnixNow = () => long.MaxValue / 2;

        await x.H.Service.RunAuctionEnds();

        string text = x.Post.Mails.Single(m => m.CharId == Bob).Message;                       // "You won %s."
        int head = Convert.ToInt32(text[..4], 16), body = Convert.ToInt32(text[4..8], 16);
        Assert.Equal("You won .", text.Substring(8 + head, body));
        Assert.Equal(8, Convert.ToInt32(text.Substring(12, 4), 16));                            // the link goes where "%s" was
        string link = text.Substring(16, Convert.ToInt32(text.Substring(8, 4), 16));
        Assert.Equal('\u0002', link[0]); Assert.Equal('\u0002', link[1]);                       // TTEXT_LINK_TYPE_ITEM = 2, marked
    }

    [Fact]
    public async Task Watching_AddsAndRemoves_AndIsSaved()
    {
        var x = await Open();
        uint id = await Sell(x);
        byte[] Watch(byte on) => Cs(Msg.CS_AUCTIONINTEREST_REQ, w => { w.WriteByte(on); w.WriteUInt32(id); w.WriteUInt16(Npc); });

        await x.H.Service.DispatchClientAsync(x.B, Watch(1));
        Assert.Equal(new[] { id }, x.Bob.AuctionInterest);
        await x.H.Service.DispatchClientAsync(x.B, Cs(Msg.CS_AUCTIONINTERESTLIST_REQ, w => { w.WriteUInt16(Npc); w.WriteUInt16(1); w.WriteByte(10); w.WriteByte(0); }));
        Assert.Single(ReadList(x.Cb.Last(Msg.CS_AUCTIONINTERESTLIST_ACK)!, out _));
        await x.H.Service.DispatchClientAsync(x.B, Watch(0));
        Assert.Empty(x.Bob.AuctionInterest);
        for (int i = 0; i < 50 && x.Db.Interest.Count != 0; i++) await Task.Delay(10);
        Assert.Empty(x.Db.Interest);
    }

    [Fact]
    public async Task SwitchedOff_NothingAnswers()
    {
        var x = await Open();
        x.H.Service.AuctionEnabled = false;
        Give(x.Ann, 5, SwordTpl);

        await x.H.Service.DispatchClientAsync(x.A, Reg(5, 10_000, 1_000));

        Assert.False(x.Ca.Has(Msg.CS_AUCTIONREG_ACK));
    }
}
