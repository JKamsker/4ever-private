using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

/// <summary>Batch S5: the mini-games — the gamble, the magic shop, the soul lottery, rock-paper-scissors and the party loot lottery.</summary>
public class MinigameTests
{
    private const uint Ann = 1, Bob = 2, Cid = 3;
    private const ushort Npc = 3000, Sealed = 100, Ticket = 9416, Prize = 7690, Loot = 300;

    private sealed record Setup(TemplateStore T, MapTestHarness H, FakePostStore Post, ClientSession A, FakeClientChannel Ca, Character Ann,
        ClientSession B, FakeClientChannel Cb, Character Bob);

    private static ItemTemplate Tpl(ushort id, byte type = 1, byte stack = 1, byte canGamble = 0, byte gambleProb = 0, byte destroy = 0)
        => new(id, 0, new[] { 1f, 0f, 0f, 0f }, Type: type, Stack: stack, CanGamble: canGamble, GambleProb: gambleProb, DestroyProb: destroy);

    private static async Task<Setup> Open(Action<TemplateStore>? seed = null)
    {
        var t = new TemplateStore();
        t.Items[Sealed] = Tpl(Sealed, type: 13, destroy: 100);
        t.Items[Ticket] = Tpl(Ticket, stack: 10);
        t.Items[Prize] = Tpl(Prize, stack: 200);
        t.Items[Loot] = Tpl(Loot);
        t.LevelExp[19] = 1_000; t.LevelExp[20] = 5_000; t.LevelMoney[20] = 100;
        t.GambleCost[3] = (2_000, 500);
        t.SvrMsgs[45] = "RPS reward"; t.SvrMsgs[46] = "Well played";
        seed?.Invoke(t);
        var h = new MapTestHarness(t);
        var post = new FakePostStore();
        h.Service.PostStore = post;
        h.Service.ApplyItemIdSeed(1000);
        h.Service.CraftRng = new Random(7);
        h.Service.AddNpc(new Npc { Id = Npc, Type = 25, Country = 1, MapId = 0 });
        Character Pc(uint id, string name)
        {
            var ch = new Character { CharId = id, Name = name, Level = 20, Country = 1, MaxHp = 100, Hp = 100, Gold = 1 };
            ch.Invens.Add(new Inven { InvenId = 0xFF });
            return ch;
        }
        var ann = Pc(Ann, "Ann"); var bob = Pc(Bob, "Bob");
        var (a, ca) = await h.EnterAsync(Ann, 11, 1, x: 100, z: 100, name: "Ann", preSeeded: ann);
        var (b, cb) = await h.EnterAsync(Bob, 12, 2, x: 101, z: 100, name: "Bob", preSeeded: bob);
        ann.Name = "Ann"; bob.Name = "Bob"; ann.Gold = bob.Gold = 1; ann.Level = bob.Level = 20;
        ca.Clear(); cb.Clear(); h.World.Clear();
        return new Setup(t, h, post, a, ca, ann, b, cb, bob);
    }

    private static byte[] Cs(ushort id, Action<PacketWriter>? body = null) { var w = new PacketWriter(id); body?.Invoke(w); return w.ToArray(); }

    private static Item Give(Character ch, byte slot, ushort id, byte count = 1, byte glevel = 0)
    {
        var item = new Item { ItemSlot = slot, TemplateId = id, Count = count, GLevel = glevel, DlId = 77 + slot };
        ch.FindInven(0xFF)!.Items.Add(item);
        item.Template = null;
        return item;
    }

    private static Item Give(Setup x, Character ch, byte slot, ushort id, byte count = 1, byte glevel = 0)
    {
        var item = Give(ch, slot, id, count, glevel);
        item.Template = x.T.Item(id);
        return item;
    }

    // ================================ the gamble ================================

    [Fact]
    public async Task GambleCheck_QuotesTheGambleLevelFee()
    {
        var x = await Open();
        Give(x, x.Ann, 5, Sealed, glevel: 3);
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_GAMBLECHECK_REQ, w => { w.WriteByte(0xFF); w.WriteByte(5); w.WriteUInt16(Npc); }));
        var r = new PacketReader(x.Ca.Last(Msg.CS_GAMBLECHECK_ACK)!);
        Assert.Equal(0, r.ReadByte()); Assert.Equal(0xFF, r.ReadByte()); Assert.Equal(5, r.ReadByte());
        Assert.Equal(2_000u, r.ReadUInt32());
        r.ReadByte();
        Assert.Equal(0, r.Remaining);
    }

    [Fact]
    public async Task GambleOpen_Wrapped_UnwrapsForFree()
    {
        var x = await Open();
        var item = Give(x, x.Ann, 5, Sealed, glevel: 3);
        item.Ext[Item.IevWrap] = 1;
        long before = x.Ann.MoneyTotal;
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_GAMBLEOPEN_REQ, w => { w.WriteByte(0xFF); w.WriteByte(5); w.WriteUInt16(Npc); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        Assert.Equal(0, new PacketReader(x.Ca.Last(Msg.CS_GAMBLEOPEN_ACK)!).ReadByte());
        Assert.Equal(0u, item.Ext[Item.IevWrap]);
        Assert.Equal(before, x.Ann.MoneyTotal);
    }

    [Fact]
    public async Task GambleOpen_NoOptionAndSureDestroy_PaysAndLosesTheItem()
    {
        var x = await Open();
        Give(x, x.Ann, 5, Sealed, glevel: 3);
        long before = x.Ann.MoneyTotal;
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_GAMBLEOPEN_REQ, w => { w.WriteByte(0xFF); w.WriteByte(5); w.WriteUInt16(Npc); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        Assert.Equal(3, new PacketReader(x.Ca.Last(Msg.CS_GAMBLEOPEN_ACK)!).ReadByte());       // GAMBLE_FAIL
        Assert.Null(x.Ann.FindInven(0xFF)!.FindItem(5));
        Assert.Equal(before - 2_000, x.Ann.MoneyTotal);
    }

    [Fact]
    public async Task GambleOpen_NotEnoughMoney_IsRefused()
    {
        var x = await Open();
        Give(x, x.Ann, 5, Sealed, glevel: 3);
        x.Ann.Gold = 0; x.Ann.Silver = 1; x.Ann.Cooper = 0;
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_GAMBLEOPEN_REQ, w => { w.WriteByte(0xFF); w.WriteByte(5); w.WriteUInt16(Npc); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        Assert.Equal(2, new PacketReader(x.Ca.Last(Msg.CS_GAMBLEOPEN_ACK)!).ReadByte());       // GAMBLE_NEEDMONEY
        Assert.NotNull(x.Ann.FindInven(0xFF)!.FindItem(5));
    }

    // ================================ the magic shop ================================

    [Fact]
    public async Task MagicShop_SellsTheCountPaidFor()
    {
        var x = await Open();
        var row = new FullItemRow(0, 0, 0, Prize, 0, 1, 0, 0, 0, 0, 0, 0, new byte[6], new ushort[6], new uint[6], 0, 0, 0);
        x.H.State.FindNpc(Npc)!.MagicItems[9] = new QuestItemRow(9, row, 0, 300);
        long before = x.Ann.MoneyTotal;
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_MAGICITEMBUY_REQ, w => { w.WriteUInt16(Npc); w.WriteUInt32(9); w.WriteByte(3); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        Assert.Equal(0, new PacketReader(x.Ca.Last(Msg.CS_MAGICITEMBUY_ACK)!).ReadByte());
        var bought = x.Ann.FindInven(0xFF)!.Items.Single(i => i.TemplateId == Prize);
        Assert.Equal(3, bought.Count);
        Assert.Equal(0, bought.EndTime);                                                        // 0 days lasts forever
        Assert.Equal(before - 900, x.Ann.MoneyTotal);
    }

    // ================================ the soul lottery ================================

    [Fact]
    public async Task SoulLottery_NeedsAFullBar()
    {
        var x = await Open();
        x.Ann.SoulLotExp = 3_999;
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_SOULLOTTERY_REQ));
        Assert.False(x.Ca.Has(Msg.CS_SOULLOTTERY_ACK));
    }

    [Fact]
    public async Task SoulLottery_DrawsFourNumbers_ThenPays()
    {
        var x = await Open();
        x.Ann.SoulLotExp = 4_000; x.Ann.UseLot = true;
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_SOULLOTTERY_REQ));
        var r = new PacketReader(x.Ca.Last(Msg.CS_SOULLOTTERY_ACK)!);
        var numbers = new[] { r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32() };
        uint prize = r.ReadUInt32();
        Assert.Equal(0, r.Remaining);
        Assert.Equal(4, numbers.Distinct().Count());
        Assert.All(numbers, n => Assert.InRange(n, 1u, 30u));
        Assert.True(prize >= 100);
        Assert.Equal(0u, x.Ann.SoulLotExp);

        long before = x.Ann.MoneyTotal;
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_SOULLOTTERY_TRIGGER_REQ));
        r = new PacketReader(x.Ca.Last(Msg.CS_SOULLOTTERY_TRIGGER_ACK)!);
        uint draw = r.ReadUInt32(), money = r.ReadUInt32();
        bool won = r.ReadByte() != 0;
        Assert.Equal(numbers.Contains(draw), won);
        Assert.Equal(won ? prize : 20_000u, money);
        Assert.Equal(before + money, x.Ann.MoneyTotal);
        Assert.False(x.Ann.UseLot);
        Assert.Null(x.Ann.LotNumbers);

        x.Ca.Clear();
        await x.H.Service.DispatchClientAsync(x.A, Cs(Msg.CS_SOULLOTTERY_TRIGGER_REQ));            // nothing drawn any more
        Assert.False(x.Ca.Has(Msg.CS_SOULLOTTERY_TRIGGER_ACK));
    }

    [Fact]
    public async Task EventLotteryMessage_GoesToEveryone()
    {
        var x = await Open();
        var w = new PacketWriter(Msg.MW_EVENTMSGLOTTERY_REQ);
        w.WriteString("Ann, Bob");
        await x.H.Service.DispatchWorldAsync(w.ToArray());
        foreach (var c in new[] { x.Ca, x.Cb })
        {
            var r = new PacketReader(c.Last(Msg.CS_SYSTEMMSG_ACK)!);
            Assert.Equal(27, r.ReadByte());
            Assert.Equal("Ann, Bob", r.ReadString());
        }
    }

    // ================================ rock-paper-scissors ================================

    private static void RpsChart(TemplateStore t, byte win, byte lose, ushort winKeep = 0)
    {
        for (byte i = 0; i < 3; i++)
            t.RpsGames[(ushort)(1 | i << 8)] = new RpsGame
            {
                Type = 1, WinCount = i, ItemId = i == 0 ? Ticket : (ushort)0, RewardMoney = i * 2_000u, WinKeep = winKeep,
                RewardItem = { [0] = i == 2 ? Prize : (ushort)0 }, ItemCount = { [0] = i == 2 ? (byte)4 : (byte)0 },
                Prob = { [0] = win, [1] = win, [2] = lose },
            };
    }

    private static byte[] RpsStart(byte slot) => Cs(Msg.CS_RPSSTART_REQ, w => { w.WriteByte(1); w.WriteByte(0xFF); w.WriteByte(slot); });
    private static byte[] RpsPlay(byte card) => Cs(Msg.CS_RPSGAME_REQ, w => w.WriteByte(card));
    private static (byte Win, byte Player, byte Npc) RpsAck(FakeClientChannel c) { var r = new PacketReader(c.Last(Msg.CS_RPSGAME_ACK)!); return (r.ReadByte(), r.ReadByte(), r.ReadByte()); }

    [Fact]
    public async Task Rps_Start_NeedsTheTicket_AndUsesIt()
    {
        var x = await Open(t => RpsChart(t, 100, 100));
        await x.H.Service.DispatchClientAsync(x.A, RpsStart(5));
        Assert.Equal(3, new PacketReader(x.Ca.Last(Msg.CS_RPSSTART_ACK)!).ReadByte());          // RPS_NEEDITEM
        var ticket = Give(x, x.Ann, 5, Ticket, count: 2);
        await x.H.Service.DispatchClientAsync(x.A, RpsStart(5));
        var r = new PacketReader(x.Ca.Last(Msg.CS_RPSSTART_ACK)!);
        Assert.Equal(0, r.ReadByte()); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); Assert.Equal(0, r.Remaining);
        Assert.Equal(1, ticket.Count);
        await x.H.Service.DispatchClientAsync(x.A, RpsStart(5));
        Assert.Equal(1, new PacketReader(x.Ca.Last(Msg.CS_RPSSTART_ACK)!).ReadByte());          // RPS_INGAME
    }

    [Fact]
    public async Task Rps_SureWins_ThenTheLastStepPaysByMail()
    {
        var x = await Open(t => RpsChart(t, 100, 100));
        Give(x, x.Ann, 5, Ticket);
        await x.H.Service.DispatchClientAsync(x.A, RpsStart(5));
        await x.H.Service.DispatchClientAsync(x.A, RpsPlay(0));
        Assert.Equal((1, 0, 2), RpsAck(x.Ca));                                                 // rock beats scissors
        Assert.Empty(x.Post.Mails);
        await x.H.Service.DispatchClientAsync(x.A, RpsPlay(1));
        Assert.Equal((2, 1, 0), RpsAck(x.Ca));                                                 // paper beats rock; no step 3: cashed out
        var mail = Assert.Single(x.Post.Mails);
        Assert.Equal((0u, 4u, 0u), (mail.Gold, mail.Silver, mail.Cooper));
        Assert.Equal(Prize, Assert.Single(mail.Items).TemplateId);
        Assert.Equal(4, mail.Items[0].Count);
        Assert.Equal(0, x.Ann.RpsType);
    }

    [Fact]
    public async Task Rps_CashOut_PaysTheWinsSoFar()
    {
        var x = await Open(t => RpsChart(t, 100, 100));
        Give(x, x.Ann, 5, Ticket);
        await x.H.Service.DispatchClientAsync(x.A, RpsStart(5));
        await x.H.Service.DispatchClientAsync(x.A, RpsPlay(2));
        await x.H.Service.DispatchClientAsync(x.A, RpsPlay(3));
        var mail = Assert.Single(x.Post.Mails);
        Assert.Equal(2u, mail.Silver);
        Assert.Empty(mail.Items);
        Assert.Equal(0, x.Ann.RpsType);
    }

    [Fact]
    public async Task Rps_Loss_EndsWithNothing()
    {
        var x = await Open(t => RpsChart(t, 0, 100));
        Give(x, x.Ann, 5, Ticket);
        await x.H.Service.DispatchClientAsync(x.A, RpsStart(5));
        await x.H.Service.DispatchClientAsync(x.A, RpsPlay(0));
        Assert.Equal((0, 0, 1), RpsAck(x.Ca));                                                 // paper covers rock
        Assert.Equal(0, x.Ann.RpsType);
        Assert.Empty(x.Post.Mails);
    }

    [Fact]
    public async Task Rps_LimitedWin_AsksTheWorld()
    {
        var x = await Open(t => RpsChart(t, 100, 100, winKeep: 1));
        Give(x, x.Ann, 5, Ticket);
        await x.H.Service.DispatchClientAsync(x.A, RpsStart(5));
        await x.H.Service.DispatchClientAsync(x.A, RpsPlay(1));
        Assert.False(x.Ca.Has(Msg.CS_RPSGAME_ACK));
        var r = new PacketReader(x.H.World.Last(Msg.MW_RPSGAME_ACK)!);
        Assert.Equal(Ann, r.ReadUInt32()); r.ReadUInt32();
        Assert.Equal((1, 1, 1), (r.ReadByte(), r.ReadByte(), r.ReadByte()));

        var w = new PacketWriter(Msg.MW_RPSGAME_REQ);
        w.WriteUInt32(Ann); w.WriteUInt32(x.A.Key); w.WriteByte(0); w.WriteByte(1);
        await x.H.Service.DispatchWorldAsync(w.ToArray());
        Assert.Equal((0, 1, 2), RpsAck(x.Ca));                                                 // denied: a loss
        Assert.Equal(0, x.Ann.RpsType);
    }

    [Fact]
    public async Task Rps_OddsChange_FromTheWorld()
    {
        var x = await Open(t => RpsChart(t, 100, 100));
        var w = new PacketWriter(Msg.MW_RPSGAMECHANGE_REQ);
        w.WriteByte(0); w.WriteUInt16(1);
        w.WriteByte(1); w.WriteByte(0); w.WriteByte(10); w.WriteByte(20); w.WriteByte(30); w.WriteUInt16(5); w.WriteUInt16(7);
        await x.H.Service.DispatchWorldAsync(w.ToArray());
        var rps = x.T.RpsGames[1];
        Assert.Equal(new byte[] { 10, 20, 30 }, rps.Prob);
        Assert.Equal((5, 7), (rps.WinKeep, rps.WinPeriod));
    }

    // ================================ the party loot lottery ================================

    private static async Task<(Setup X, Monster Mon, Item Item)> LotteryCorpse()
    {
        var x = await Open();
        foreach (var ch in new[] { x.Ann, x.Bob }) { ch.PartyId = 9; ch.PartyType = 3; }
        var mon = new Monster { Id = 0x60001, ChartId = 500, Level = 5, MaxHp = 10, Hp = 0, PosX = 100, PosZ = 100, Channel = 1, MapId = 0 };
        x.H.Service.SpawnMonster(mon);
        mon.AddDamage(Ann, 9, 10);
        var item = new Item { ItemSlot = 0, TemplateId = Loot, Count = 1, Template = x.T.Item(Loot) };
        mon.CorpseInven.Items.Add(item);
        x.Ca.Clear(); x.Cb.Clear();
        return (x, mon, item);
    }

    private static byte[] Answer(byte yes, uint monId) => Cs(Msg.CS_MONITEMLOTTERY_REQ, w => { w.WriteByte(yes); w.WriteUInt32(monId); w.WriteUInt16(Loot); });

    [Fact]
    public async Task LootLottery_AsksThePartyAndHidesTheItem()
    {
        var (x, mon, item) = await LotteryCorpse();
        await x.H.Service.DispatchClientAsync(x.A, MapTestHarness.MonItemTakeReq(mon.Id, 0));
        Assert.Equal(2, item.MaxRouting);
        var r = new PacketReader(x.Cb.Last(Msg.CS_MONITEMLOTTERY_ACK)!);
        Assert.Equal(mon.Id, r.ReadUInt32());
        Assert.False(x.Ca.Has(Msg.CS_MONITEMLOTTERY_ACK));
        var list = new PacketReader(x.Ca.Last(Msg.CS_MONITEMLIST_ACK)!);
        list.ReadByte(); list.ReadByte(); list.ReadUInt32(); list.ReadUInt32(); list.ReadUInt32(); list.ReadUInt32();
        Assert.Equal(0, list.ReadByte());                                                      // drawn for: off the list
        Assert.Contains(item, mon.CorpseInven.Items);
    }

    [Fact]
    public async Task LootLottery_ANo_GivesItToTheOnlyYes()
    {
        var (x, mon, item) = await LotteryCorpse();
        await x.H.Service.DispatchClientAsync(x.A, MapTestHarness.MonItemTakeReq(mon.Id, 0));
        await x.H.Service.DispatchClientAsync(x.B, Answer(0, mon.Id));
        Assert.DoesNotContain(item, mon.CorpseInven.Items);
        Assert.Contains(x.Ann.FindInven(0xFF)!.Items, i => i.TemplateId == Loot);
        Assert.True(x.Ca.Has(Msg.CS_GETITEM_ACK));
        Assert.True(x.Cb.Has(Msg.CS_PARTYITEMTAKE_ACK));
    }

    [Fact]
    public async Task LootLottery_OnlyTheAskedAnswer_AndOnce()
    {
        var (x, mon, item) = await LotteryCorpse();
        var cid = new Character { CharId = Cid, Name = "Cid", Level = 20, Country = 1, MaxHp = 100, Hp = 100 };
        cid.Invens.Add(new Inven { InvenId = 0xFF });
        var (c, _) = await x.H.EnterAsync(Cid, 13, 3, x: 100, z: 101, name: "Cid", preSeeded: cid);
        await x.H.Service.DispatchClientAsync(x.A, MapTestHarness.MonItemTakeReq(mon.Id, 0));
        await x.H.Service.DispatchClientAsync(c, Answer(0, mon.Id));                            // not asked: ignored
        await x.H.Service.DispatchClientAsync(x.A, Answer(0, mon.Id));                          // the taker already said yes
        Assert.Equal(2, item.MaxRouting);
        Assert.Contains(item, mon.CorpseInven.Items);
        await x.H.Service.DispatchClientAsync(x.B, Answer(1, mon.Id));                          // everyone answered: drawn
        Assert.Equal(0, item.MaxRouting);
        Assert.DoesNotContain(item, mon.CorpseInven.Items);
    }

    [Fact]
    public async Task LootLottery_NobodyNear_TakesAtOnce()
    {
        var (x, mon, item) = await LotteryCorpse();
        x.Bob.PartyId = 8;
        await x.H.Service.DispatchClientAsync(x.A, MapTestHarness.MonItemTakeReq(mon.Id, 0));
        Assert.False(x.Cb.Has(Msg.CS_MONITEMLOTTERY_ACK));
        Assert.Contains(x.Ann.FindInven(0xFF)!.Items, i => i.TemplateId == Loot);
    }
}
