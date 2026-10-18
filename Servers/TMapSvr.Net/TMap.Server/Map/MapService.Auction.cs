using System.Text;
using TMap.Data;
using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>An auction lot (C++ <c>TAUCTION</c>, TMapType.h:2639) held by its auctioneer NPC.</summary>
public sealed class Auction
{
    public uint Id;
    public ushort NpcId;
    public uint Seller;
    public string SellerName = "";
    public Item Item = null!;
    public long Direct, Start, Bid;
    public byte BidCount;
    public long StartTime, End, BidTime;
    public uint RemainTime, Code;
    /// <summary>The best bidder's account (C++ <c>m_dwBidder</c>, compared with the bidder's user id).</summary>
    public uint BidderUser;
    /// <summary>The end is being settled; and when to try again if the database said it had not ended yet.</summary>
    public bool Ending;
    public long RetryAt;
}

/// <summary>
/// The auction house (C++ CSHandler.cpp:15495–16330, SSHandler.cpp:16158–17060, TNpc.cpp:105–290, TMapSvr.cpp:4055–4248). On in
/// the C++ 5.0 map server (the early returns are only for <c>BOW_COMPILE_MODE</c>, the BoW/BR builds); <see cref="AuctionEnabled"/>
/// can turn it off.
/// <list type="bullet">
/// <item><b>Selling</b> (<c>CS_AUCTIONREG_REQ</c>) at an auctioneer of one's own country: a direct price (≥ the start price), the
/// item leaves the bag, 2% of the direct price is paid at once. A stack is only sold unit by unit at the start price.</item>
/// <item><b>Bidding</b>: at least 5% over the best bid, below the direct price, not on a stack, not one's own lot, not over
/// one's own bid; the money goes at once and comes back by mail when outbid. <b>Buying</b> at the direct price ends the lot (or
/// takes units of a stack). The seller is paid by mail, less 3%; the buyer gets the item by mail.</item>
/// <item><b>Ending</b>: when the time is up the best bidder wins (the seller paid less 3%); without one the item goes back. A seller
/// may cancel (the bid is refunded; the 2% is not).</item>
/// <item><b>Lists</b>: watched, bid on, one's own lots, and a search by category, levels and option, sorted 22 ways, 5 pages a
/// time. A bid also watches the lot.</item>
/// <item>At most 30 letters, lots and bids together (<c>MAX_POST</c>).</item>
/// </list>
/// <para><b>Fixed C++ bugs:</b> the best bidder was a character id after a restart but an account id after a bid (the "already your
/// bid" check then compared unlike ids) — always the account now; an end the database refused (its clock not there yet) left the lot
/// stuck until a restart — tried again a minute later; lots ended, cancelled or bought stayed counted in the 30 until the list was
/// opened; the seller-name sort was not a strict order. A watch list change is saved at once (the C++ saves it at logout). <b>Not
/// ported:</b> the security-code lock and the channel (main cell) gate — one map server owns every cell.</para>
/// </summary>
public sealed partial class MapService
{
    public bool AuctionEnabled { get; set; } = true;

    /// <summary>The auction store — the game database in production, a fake in tests.</summary>
    public IAuctionStore? AuctionStore { get; set; }

    // AUCTION_RESULT / AUCTION_LIST_RESULT / AUCTION_BID_RESULT (NetCode.h:2448–2490)
    private const byte ArSuccess = 0, ArFail = 1, ArNpcNotFound = 2, ArInvalidChar = 3, ArInvalidPrice = 4, ArInvalidStartPrice = 5,
        ArItemNotFound = 6, ArInvalidItem = 7, ArInvalidAuction = 8, ArNeedMoney = 9, ArMaxPost = 10;
    private const byte AlrSuccess = 0, AlrFail = 1, AlrNpcNotFound = 2, AlrNoList = 3, AlrNoPage = 4;
    private const byte AbrFail = 1, AbrAuctionNotFound = 3, AbrSelf = 4, AbrLowPrice = 5, AbrNeedMoney = 6, AbrCountError = 7,
        AbrDuplicate = 8, AbrMaxPost = 10, AbrLimitedDate = 11;
    private const double AuctionSuccessFee = 0.03, AuctionBidFee = 0.05, AuctionRegFee = 0.02;
    private const int AuctionPageCount = 5, MaxPost = 30, TClassCount = 6;
    // SERVER_MESSAGE (CTProtocol.h:53): MSG_AUCTION_OPERATOR = 13 …
    private const uint MsgAuctionOperator = 13, MsgAuctionFail = 14, MsgAuctionBidSuccess = 15, MsgAuctionPrice = 16,
        MsgAuctionRegCancel = 17, MsgAuctionBidFail = 18, MsgAuctionOtherBidder = 19, MsgAuctionOtherWin = 20, MsgAuctionFailBody = 21,
        MsgAuctionBidSuccessBody = 22, MsgAuctionPriceBody = 23, MsgAuctionRegCancelBody = 24, MsgAuctionBidFailBody = 25,
        MsgAuctionOtherBidderBody = 26, MsgAuctionOtherBuyBody = 27, MsgAuctionDropBody = 28, MsgAuctionRegCancelDrop = 29;

    private sealed class AuctionHouse
    {
        public readonly SortedDictionary<uint, Auction> All = new();
        public readonly Dictionary<uint, SortedDictionary<uint, Auction>> Find = new();
    }

    /// <summary>C++ <c>CTNpc::m_mapTAuction</c> / <c>m_mapTAuctionFind</c>, by NPC.</summary>
    private readonly Dictionary<ushort, AuctionHouse> _auctions = new();

    private AuctionHouse HouseOf(ushort npcId) => _auctions.TryGetValue(npcId, out var h) ? h : _auctions[npcId] = new AuctionHouse();

    private Auction? FindAuction(uint id)
    {
        foreach (var h in _auctions.Values) if (h.All.TryGetValue(id, out var a)) return a;
        return null;
    }

    /// <summary>The category keys a lot is indexed under (C++ <c>AddAuction</c>): one per class bit, or one with no class.</summary>
    private static IEnumerable<uint> AuctionCodes(uint code)
    {
        byte type = (byte)(code >> 16), cls = (byte)(code >> 8), kind = (byte)code;
        if (cls == 0) { yield return (uint)type << 16 | kind; yield break; }
        for (int i = 0; i < TClassCount; i++)
            if ((cls & (1 << i)) != 0) yield return (uint)type << 16 | (uint)(1 << i) << 8 | kind;
    }

    private void AddAuction(Auction a)
    {
        var h = HouseOf(a.NpcId);
        if (!h.All.TryAdd(a.Id, a)) return;
        foreach (uint key in AuctionCodes(a.Code))
            (h.Find.TryGetValue(key, out var m) ? m : h.Find[key] = new())[a.Id] = a;
    }

    private void DelAuction(Auction a)
    {
        var h = HouseOf(a.NpcId);
        foreach (uint key in AuctionCodes(a.Code)) if (h.Find.TryGetValue(key, out var m)) m.Remove(a.Id);
        h.All.Remove(a.Id);
    }

    /// <summary>C++ startup (TMapSvr.cpp:4055): every lot, with its best bid; each goes on the end timer.</summary>
    public async Task LoadAuctionsAsync()
    {
        if (AuctionStore is not { } db) return;
        foreach (var row in await Safe(() => db.LoadAuctionsAsync(), new List<AuctionRow>()))
        {
            if (ItemFromRow(row.Item) is not { } item) continue;
            AddAuction(new Auction
            {
                Id = row.Id, NpcId = row.NpcId, Seller = row.Seller, SellerName = row.SellerName, Item = item, Direct = row.Direct,
                Start = row.Start, Bid = row.BidPrice, BidCount = row.BidCount, StartTime = row.DateStart, End = row.DateEnd,
                BidTime = row.BidTime, Code = item.Template?.Code ?? 0, BidderUser = row.BidderUser,
            });
        }
    }

    /// <summary>C++ <c>OnDM_LOADCHAR_REQ</c>'s auction lists, once.</summary>
    private async Task LoadAuctionLists(Character ch)
    {
        if (ch.AuctionLoaded || AuctionStore is not { } db) return;
        var lists = await Safe(() => db.LoadAuctionListsAsync(ch.CharId), new AuctionLists(new(), new(), new()));
        (ch.AuctionInterest, ch.AuctionBid, ch.AuctionReg) = (lists.Interest, lists.Bid, lists.Reg);
        ch.AuctionLoaded = true;
    }

    private static void ListSave(List<uint> list, bool add, uint id)
    {
        list.Remove(id);
        if (add) list.Add(id);
    }

    private void SaveInterest(Character ch)
    {
        if (AuctionStore is { } db) { var ids = ch.AuctionInterest.ToList(); _ = EnqueueDbWrite(() => db.SaveAuctionInterestAsync(ch.CharId, ids)); }
    }

    /// <summary>C++ <c>CTPlayer::CanUseAuction</c> (TPlayer.cpp:5767): letters, lots and bids under 30 — lots and bids gone are
    /// dropped first (fixed: the C++ counted them until the list was opened).</summary>
    private bool CanUseAuction(Character ch)
    {
        ch.AuctionReg.RemoveAll(id => FindAuction(id) is null);
        ch.AuctionBid.RemoveAll(id => FindAuction(id) is null);
        return ch.PostTotal + ch.AuctionReg.Count + ch.AuctionBid.Count < MaxPost;
    }

    /// <summary>The auctioneer check (C++, repeated): the NPC, of the peace country, the player's or its aid country.</summary>
    private Npc? AuctionNpc(Character ch, ushort npcId)
        => _state.FindNpc(npcId) is { } npc && (npc.Country == TcontryB || npc.Country == ch.Country || npc.Country == ch.AidCountry) ? npc : null;

    // ================================ letters ================================

    /// <summary>C++ <c>CTTextLinkData::PackItem</c> (TTextLinkData.cpp:57): the item as a chat link — each byte marked (0x02 + the
    /// byte, or 0x03 for a zero).</summary>
    private static string ItemLink(Item it)
    {
        var raw = new List<byte>();
        void Put(ReadOnlySpan<byte> bytes) { foreach (byte b in bytes) { if (b != 0) { raw.Add(0x02); raw.Add(b); } else raw.Add(0x03); } }
        void U8(byte v) => Put(new[] { v });
        void U16(ushort v) => Put(BitConverter.GetBytes(v));
        void U32(uint v) => Put(BitConverter.GetBytes(v));
        U32(2); U16(it.TemplateId);                                                             // TTEXT_LINK_TYPE_ITEM
        U8(it.Level); U8(it.Gem); U32(it.DuraCur); U32(it.DuraMax); U8(it.RefineCur); U8(it.Template?.RefineMax ?? it.RefineMax);
        U8(it.GLevel); U8((byte)it.Ext[Item.IevWrap]); U8((byte)it.Ext[Item.IevEld]); U16(it.MoggItemId);
        U16((ushort)it.Ext[Item.IevCompanion]); U8(it.GradeEffect); Put(BitConverter.GetBytes(it.EndTime));
        foreach (var m in it.Magic.OrderBy(m => m.Id)) { U8(m.Id); U16((ushort)it.GetMagicValue(m.Id)); }
        return Encoding.Latin1.GetString(raw.ToArray());
    }

    /// <summary>C++ <c>CTTextLinker::MakeItemNetText</c> (TTextLinker.cpp:55) for one item: the text around <c>%s</c>, the link in the
    /// header (with its length and where it goes), within the 512-character chat limit.</summary>
    private static string ItemNetText(string format, Item it)
    {
        const int charMax = 512, dwordSize = 8;
        int size = dwordSize, pos = 0;
        var head = new StringBuilder();
        var body = new StringBuilder();
        int at = format.IndexOf("%s", StringComparison.Ordinal);
        if (at >= 0)
        {
            body.Append(format, 0, at);
            size += at;
            if (size > charMax) return MapService.NetString(body.ToString()[..Math.Max(0, body.Length - (size - charMax))]);
            string link = ItemLink(it);
            if (link.Length != 0)
            {
                if (size + link.Length + dwordSize > charMax) return MapService.NetString(body.ToString(), head.ToString());
                head.Append($"{link.Length:X4}{body.Length:X4}").Append(link);
                size += link.Length + dwordSize;
            }
            pos = at + 2;
        }
        if (pos < format.Length)
        {
            body.Append(format, pos, format.Length - pos);
            size += format.Length - pos;
            if (size > charMax)
            {
                int over = size - charMax;
                if (over > body.Length) return MapService.NetString("");
                body.Length -= over;
            }
        }
        return MapService.NetString(body.ToString(), head.ToString());
    }

    /// <summary>C++ <c>SendDM_POSTRECV_REQ</c> from the auctioneer: a letter, with money or an item (moved to the post by its row).</summary>
    private async Task AuctionMail(uint recvId, string recvName, uint title, string body, byte type, long money = 0, Item? item = null)
    {
        if (PostStore is not { } db) return;
        var (gold, silver, cooper) = SplitMoney(money);
        string sender = SvrMsg(MsgAuctionOperator), head = MapService.NetString(SvrMsg(title));
        long now = UnixNow();
        (int saved, uint postId, _) = await Safe(() => db.SavePostAsync(0, recvId, recvName, sender, head, body, 0, type, gold, silver, cooper, now),
            (PostInternal, 0u, 0u));
        if (saved != 0 || postId == 0) return;
        if (item is not null)
        {
            var row = BuildItemSave(0, item) with { StorageType = Proto.StoragePost, StorageId = postId, ItemSlot = 0 };
            await Safe(async () => { await db.SavePostItemAsync(recvId, row); return 0; }, 0);
        }
        NotifyPostRecv(postId, sender, recvName, head, type, now);
    }

    // ================================ the list answers ================================

    private void SendAuctionList(ClientSession s, ushort id, byte result, List<Auction>? list = null, ushort page = 0, ushort from = 0, ushort to = 0)
    {
        var w = new PacketWriter(id);
        w.WriteByte(result);
        if (list is not null)
        {
            w.WriteUInt16((ushort)list.Count); w.WriteUInt16(page); w.WriteUInt16((ushort)(to - from));
            for (int i = from; i < to; i++)
            {
                var a = list[i];
                w.WriteUInt32(a.Id); w.WriteString(a.SellerName); w.WriteInt64(a.End); w.WriteUInt32(a.RemainTime);
                w.WriteInt64(a.Direct); w.WriteInt64(a.Start); w.WriteInt64(a.Bid); w.WriteByte(a.BidCount); w.WriteUInt32(a.Code);
                a.Item.WrapPacketClient(w, s.CharId);
            }
        }
        s.Send(w);
    }

    private void SetRemain(Auction a, long now) => a.RemainTime = a.End > now ? (uint)(a.End - now) : 0;

    /// <summary>C++ <c>CTNpc::AlignAuction</c> (TNpc.cpp:264).</summary>
    private static List<Auction> AlignAuction(List<Auction> list, byte kind)
    {
        IEnumerable<Auction> Asc<T>(Func<Auction, T> key) => list.OrderBy(key);
        IEnumerable<Auction> Desc<T>(Func<Auction, T> key) => list.OrderByDescending(key);
        return (kind switch
        {
            0 => Asc(a => a.Id), 1 => Desc(a => a.Id), 2 => Asc(a => a.End), 3 => Desc(a => a.End),
            4 => Asc(a => a.RemainTime), 5 => Desc(a => a.RemainTime), 6 => Asc(a => a.Bid), 7 => Desc(a => a.Bid),
            8 => Asc(a => a.Direct), 9 => Desc(a => a.Direct), 10 => Asc(a => a.BidCount), 11 => Desc(a => a.BidCount),
            12 => Asc(a => a.Item.Template?.ClassId ?? 0), 13 => Desc(a => a.Item.Template?.ClassId ?? 0),
            14 => list.OrderBy(a => a.SellerName, StringComparer.Ordinal), 15 => list.OrderByDescending(a => a.SellerName, StringComparer.Ordinal),
            16 => list, 17 => Enumerable.Reverse(list),
            18 => Asc(a => a.Item.Template?.DefaultLevel ?? 0), 19 => Desc(a => a.Item.Template?.DefaultLevel ?? 0),
            20 => Asc(a => a.Item.Level), 21 => Desc(a => a.Item.Level),
            _ => list,
        }).ToList();
    }

    /// <summary>The paging every list does (C++, WORD arithmetic): from <c>count·(page−1)</c>, five pages long.</summary>
    private void SendAuctionPage(ClientSession s, ushort id, List<Auction> list, ushort page, byte perPage, byte align)
    {
        list = AlignAuction(list, align);
        ushort from = (ushort)(perPage * (page - 1));
        ushort to = (ushort)(from + perPage * AuctionPageCount);
        if (to > list.Count) to = (ushort)list.Count;
        if (to < from) { SendAuctionList(s, id, AlrNoPage); return; }
        SendAuctionList(s, id, AlrSuccess, list, page, from, to);
    }

    /// <summary>C++ <c>OnCS_AUCTIONINTERESTLIST_REQ</c> / <c>BIDLIST</c> / <c>REGLIST</c> (CSHandler.cpp:15687/15764/15843): the lots
    /// of one of the player's lists at this auctioneer — the ones gone are dropped from it.</summary>
    private void OnCS_AUCTIONMYLIST_REQ(ClientSession s, PacketReader r, ushort ackId, Func<Character, List<uint>> listOf, bool anyCountry)
    {
        if (!AuctionEnabled || !s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        ushort npcId = r.ReadUInt16(), page = r.ReadUInt16();
        byte perPage = r.ReadByte(), align = r.ReadByte();
        bool npcOk = anyCountry ? _state.FindNpc(npcId) is not null : AuctionNpc(ch, npcId) is not null;
        if (!npcOk) { SendAuctionList(s, ackId, anyCountry ? AlrNpcNotFound : AlrFail); return; }
        var house = HouseOf(npcId);
        long now = UnixNow();
        var ids = listOf(ch);
        var found = new List<Auction>();
        ids.RemoveAll(id =>
        {
            if (!house.All.TryGetValue(id, out var a)) return true;
            SetRemain(a, now);
            found.Add(a);
            return false;
        });
        SendAuctionPage(s, ackId, found, page, perPage, align);
    }

    /// <summary>C++ <c>CTNpc::AuctionCondition</c> (TNpc.cpp:195).</summary>
    private static bool AuctionCondition(Auction a, byte minWear, byte maxWear, byte minPower, byte maxPower, byte option, ushort itemId)
    {
        int wear = a.Item.EquipLevel, power = a.Item.Level;
        if (minWear > wear || wear > maxWear || minPower > power || power > maxPower) return false;
        if (option != 0 && a.Item.Magic.All(m => m.Id != option)) return false;
        return itemId == 0 || itemId == a.Item.TemplateId;
    }

    /// <summary>C++ <c>OnCS_AUCTIONFIND_REQ</c> (CSHandler.cpp:16221): by categories (each with an item, 0 = any), or all.</summary>
    private void OnCS_AUCTIONFIND_REQ(ClientSession s, PacketReader r)
    {
        if (!AuctionEnabled || !s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        ushort npcId = r.ReadUInt16();
        byte minWear = r.ReadByte(), maxWear = r.ReadByte(), minPower = r.ReadByte(), maxPower = r.ReadByte(), option = r.ReadByte();
        ushort page = r.ReadUInt16();
        byte perPage = r.ReadByte(), align = r.ReadByte();
        ushort n = r.ReadUInt16();
        var wanted = new List<(uint Code, ushort Item)>();
        for (int i = 0; i < n; i++) wanted.Add((r.ReadUInt32(), r.ReadUInt16()));
        if (AuctionNpc(ch, npcId) is null) { SendAuctionList(s, Msg.CS_AUCTIONFIND_ACK, AlrFail); return; }
        var house = HouseOf(npcId);
        long now = UnixNow();
        List<Auction> list;
        if (n == 0) list = house.All.Values.Where(a => AuctionCondition(a, minWear, maxWear, minPower, maxPower, option, 0)).ToList();
        else
        {
            var hits = new SortedDictionary<uint, Auction>();
            foreach (var (code, item) in wanted)
                if (house.Find.TryGetValue(code, out var m))
                    foreach (var a in m.Values)
                        if (AuctionCondition(a, minWear, maxWear, minPower, maxPower, option, item)) hits[a.Id] = a;
            list = hits.Values.ToList();
        }
        foreach (var a in list) SetRemain(a, now);
        if (list.Count == 0) { SendAuctionList(s, Msg.CS_AUCTIONFIND_ACK, AlrNoList); return; }
        if (list.Count < (ushort)(perPage * (page - 1))) { SendAuctionList(s, Msg.CS_AUCTIONFIND_ACK, AlrNoPage); return; }
        SendAuctionPage(s, Msg.CS_AUCTIONFIND_ACK, list, page, perPage, align);
    }

    /// <summary>C++ <c>OnCS_AUCTIONINTEREST_REQ</c> (CSHandler.cpp:15637): watch (1) or stop watching (0) a lot.</summary>
    private void OnCS_AUCTIONINTEREST_REQ(ClientSession s, PacketReader r)
    {
        if (!AuctionEnabled || !s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        byte type = r.ReadByte();
        uint id = r.ReadUInt32();
        ushort npcId = r.ReadUInt16();
        bool ok = AuctionNpc(ch, npcId) is not null && HouseOf(npcId).All.ContainsKey(id);
        if (ok) { ListSave(ch.AuctionInterest, type != 0, id); SaveInterest(ch); }
        s.Send(new PacketWriter(Msg.CS_AUCTIONINTEREST_ACK).WriteByte(ok ? (byte)1 : (byte)0).WriteByte(type));
    }

    // ================================ selling ================================

    private static void SendByteAck(ClientSession s, ushort id, byte result) => s.Send(new PacketWriter(id).WriteByte(result));

    /// <summary>C++ <c>OnCS_AUCTIONREG_REQ</c> (CSHandler.cpp:15495) and <c>OnDM_AUCTIONREG_REQ/ACK</c> (SSHandler.cpp:16158–16380).</summary>
    private async Task OnCS_AUCTIONREG_REQ(ClientSession s, PacketReader r)
    {
        if (!AuctionEnabled || !s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        ushort npcId = r.ReadUInt16(), hours = r.ReadUInt16();
        byte inven = r.ReadByte(), slot = r.ReadByte();
        long direct = r.ReadInt64(), start = r.ReadInt64();
        r.ReadUInt32();                                                                         // dwCode: the item's own is used
        void Ack(byte result, uint id = 0) => s.Send(new PacketWriter(Msg.CS_AUCTIONREG_ACK).WriteByte(result).WriteUInt32(id));

        if (!CanUseAuction(ch)) { Ack(ArMaxPost); return; }
        if (direct < start || direct == 0) { Ack(ArInvalidPrice); return; }
        if (start == 0) { Ack(ArInvalidStartPrice); return; }
        if (_state.FindNpc(npcId) is not { } npc || npc.Country != ch.Country) { Ack(ArFail); return; }
        var inv = ch.FindInven(inven);
        if (inv?.FindItem(slot) is not { Count: > 0 } item) { Ack(ArItemNotFound); return; }
        if (!item.CanDeal()) { Ack(ArInvalidItem); return; }
        if (item.Template?.Stack == 1 && direct == start) { Ack(ArInvalidPrice); return; }
        if (AuctionStore is not { } db || !_itemIdReady) { Ack(ArFail); return; }
        long fee = (long)(direct * AuctionRegFee);
        if (!ch.UseMoney(fee, commit: true)) { Ack(ArNeedMoney); return; }
        SendCS_MONEY_ACK(s, ch);
        inv.Items.Remove(item);
        SendCS_DELITEM_ACK(s, inven, item);                                                     // the bag's row goes
        item.DlId = GenItemId();                                                                // the lot's row, the seller's until the proc
        var save = BuildItemSave(inven, item);
        uint code = item.Template?.Code ?? 0;

        var reg = await Safe(async () =>
        {
            await db.SaveAuctionItemAsync(ch.CharId, save);
            return await db.AuctionRegAsync(npcId, hours, ch.CharId, direct, start, item.DlId);
        }, new AuctionRegResult(ArFail, 0, 0, 0));

        if (reg.Ret != ArSuccess)
        {
            await AuctionMail(ch.CharId, ch.Name, MsgAuctionRegCancel, ItemNetText(SvrMsg(MsgAuctionRegCancelBody), item), PostPackage, fee, item);
            if (s.Char == ch) Ack((byte)reg.Ret, reg.Id);
            return;
        }
        if (s.Char == ch) { Ack(ArSuccess, reg.Id); ListSave(ch.AuctionReg, true, reg.Id); }
        AddAuction(new Auction
        {
            Id = reg.Id, NpcId = npcId, Seller = ch.CharId, SellerName = ch.Name, Item = item, Direct = direct, Start = start, Bid = start,
            StartTime = reg.Start, End = reg.End, Code = code,
        });
    }

    /// <summary>C++ <c>OnCS_AUCTIONREGCANCEL_REQ</c> (CSHandler.cpp:15918) and <c>OnDM_AUCTIONREGCANCEL_ACK</c> (SSHandler.cpp:16590): the
    /// seller takes its lot back; the best bid is refunded.</summary>
    private async Task OnCS_AUCTIONREGCANCEL_REQ(ClientSession s, PacketReader r)
    {
        if (!AuctionEnabled || !s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        ushort npcId = r.ReadUInt16();
        uint id = r.ReadUInt32();
        if (_state.FindNpc(npcId) is null) { SendByteAck(s, Msg.CS_AUCTIONREGCANCEL_ACK, ArNpcNotFound); return; }
        if (!HouseOf(npcId).All.TryGetValue(id, out var a) || a.Ending) { SendByteAck(s, Msg.CS_AUCTIONREGCANCEL_ACK, ArInvalidAuction); return; }
        if (a.Seller != ch.CharId) { SendByteAck(s, Msg.CS_AUCTIONREGCANCEL_ACK, ArInvalidChar); return; }
        if (AuctionStore is not { } db) return;
        a.Ending = true;
        var res = await Safe(() => db.AuctionRegCancelAsync(npcId, id), new AuctionCancelResult(ArFail, "", "", 0, 0));
        if (s.Char == ch) SendByteAck(s, Msg.CS_AUCTIONREGCANCEL_ACK, (byte)res.Ret);
        if (res.Ret != ArSuccess) { a.Ending = false; return; }
        DelAuction(a);
        ListSave(ch.AuctionReg, false, id);
        await AuctionMail(ch.CharId, res.Seller, MsgAuctionRegCancel, ItemNetText(SvrMsg(MsgAuctionRegCancelBody), a.Item), PostPackage, item: a.Item);
        if (res.BidderId != 0)
            await AuctionMail(res.BidderId, res.Bidder, MsgAuctionRegCancelDrop, ItemNetText(SvrMsg(MsgAuctionDropBody), a.Item), PostNormal, res.BidPrice);
    }

    // ================================ buying ================================

    /// <summary>C++ <c>OnCS_AUCTIONBID_REQ</c> (CSHandler.cpp:15973) and <c>OnDM_AUCTIONBID_ACK</c> (SSHandler.cpp:16751).</summary>
    private async Task OnCS_AUCTIONBID_REQ(ClientSession s, PacketReader r)
    {
        if (!AuctionEnabled || !s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        ushort npcId = r.ReadUInt16();
        uint id = r.ReadUInt32();
        long bid = r.ReadInt64();
        void Ack(byte result) => SendByteAck(s, Msg.CS_AUCTIONBID_ACK, result);
        if (!CanUseAuction(ch)) { Ack(AbrMaxPost); return; }
        if (AuctionNpc(ch, npcId) is not { } npc) { Ack(AbrFail); return; }
        if (!HouseOf(npcId).All.TryGetValue(id, out var a) || a.Ending) { Ack(AbrAuctionNotFound); return; }
        if (npc.Country != TcontryB && npc.Country != ch.Country
            && DateTimeOffset.FromUnixTimeSeconds(a.End).DayOfWeek < DateTimeOffset.FromUnixTimeSeconds(UnixNow()).DayOfWeek) { Ack(AbrLimitedDate); return; }
        if (bid == 0 || bid < a.Bid + a.Bid * AuctionBidFee) { Ack(AbrLowPrice); return; }
        if (bid >= a.Direct) { Ack(AbrLowPrice); return; }
        if (a.Item.Template?.Stack > 1) { Ack(AbrAuctionNotFound); return; }
        if (a.Seller == ch.CharId) { Ack(AbrSelf); return; }
        if (a.BidderUser != 0 && a.BidderUser == s.UserId) { Ack(AbrDuplicate); return; }
        if (AuctionStore is not { } db) return;
        if (!ch.UseMoney(bid, commit: true)) { Ack(AbrNeedMoney); return; }
        SendCS_MONEY_ACK(s, ch);

        var res = await Safe(() => db.AuctionBidAsync(npcId, id, ch.CharId, bid), new AuctionBidResult(AbrFail, 0, 0, "", 0));
        if (s.Char == ch) Ack((byte)res.Ret);
        if (res.Ret != 0)
        {
            await AuctionMail(ch.CharId, ch.Name, MsgAuctionBidFail, ItemNetText(SvrMsg(MsgAuctionBidFailBody), a.Item), PostNormal, bid);
            return;
        }
        (a.BidTime, a.Bid, a.BidderUser) = (res.BidTime, bid, s.UserId);
        a.BidCount++;
        ListSave(ch.AuctionBid, true, id);
        ListSave(ch.AuctionInterest, true, id);
        SaveInterest(ch);
        if (res.DropName.Length != 0 && FindByName(res.DropName)?.Char is { } dropped) ListSave(dropped.AuctionBid, false, id);
        if (res.DropBidder != 0)
            await AuctionMail(res.DropBidder, res.DropName, MsgAuctionOtherBidder, ItemNetText(SvrMsg(MsgAuctionOtherBidderBody), a.Item), PostNormal, res.DropPrice);
    }

    /// <summary>C++ <c>OnCS_AUCTIONBUYDIRECT_REQ</c> (CSHandler.cpp:16109) and <c>OnDM_AUCTIONBUYDIRECT_ACK</c> (SSHandler.cpp:16935): at the
    /// direct price (a stack: units at the start price).</summary>
    private async Task OnCS_AUCTIONBUYDIRECT_REQ(ClientSession s, PacketReader r)
    {
        if (!AuctionEnabled || !s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        ushort npcId = r.ReadUInt16();
        uint id = r.ReadUInt32();
        byte count = r.ReadByte();
        void Ack(byte result) => SendByteAck(s, Msg.CS_AUCTIONBUYDIRECT_ACK, result);
        if (!CanUseAuction(ch)) { Ack(AbrMaxPost); return; }
        if (AuctionNpc(ch, npcId) is null) { Ack(AbrFail); return; }
        if (!HouseOf(npcId).All.TryGetValue(id, out var a) || a.Ending) { Ack(AbrAuctionNotFound); return; }
        if (count == 0 || count > a.Item.Count) { Ack(AbrCountError); return; }
        if (a.Seller == ch.CharId) { Ack(AbrSelf); return; }
        long unit = a.Item.Template?.Stack > 1 ? a.Start : a.Direct;
        if (AuctionStore is not { } db || !_itemIdReady) return;
        if (!ch.UseMoney(unit * count, commit: true)) { Ack(AbrNeedMoney); return; }
        SendCS_MONEY_ACK(s, ch);
        bool whole = count == a.Item.Count;
        if (whole) a.Ending = true;

        var res = await Safe(() => db.AuctionBuyDirectAsync(npcId, id, ch.CharId, count, unit), new AuctionBuyResult(AbrFail, 0, "", 0, "", 0));
        if (s.Char == ch) Ack((byte)res.Ret);
        if (res.Ret != 0)
        {
            a.Ending = false;
            await AuctionMail(ch.CharId, ch.Name, MsgAuctionBidFail, MapService.NetString(""), PostNormal, unit * count);
            return;
        }
        Item bought;
        if (whole) { bought = a.Item; DelAuction(a); }
        else
        {
            bought = a.Item.Clone();
            bought.Count = count;
            bought.DlId = GenItemId();
            a.Item.Count -= count;
            a.Direct -= unit * count;
        }
        long total = unit * count;
        await AuctionMail(res.Seller, res.SellerName, MsgAuctionPrice, ItemNetText(SvrMsg(MsgAuctionPriceBody), bought), PostNormal,
            total - (long)(total * AuctionSuccessFee));
        await AuctionMail(ch.CharId, ch.Name, MsgAuctionBidSuccess, ItemNetText(SvrMsg(MsgAuctionBidSuccessBody), bought), PostPackage, item: bought);
        if (res.DropName.Length != 0)
        {
            if (FindByName(res.DropName)?.Char is { } dropped) ListSave(dropped.AuctionBid, false, id);
            await AuctionMail(res.DropBidder, res.DropName, MsgAuctionOtherWin, ItemNetText(SvrMsg(MsgAuctionOtherBuyBody), bought), PostNormal, res.DropPrice);
        }
    }

    // ================================ the end ================================

    /// <summary>C++ <c>AuctionTimeCheck</c> (TMapSvr.cpp:6338) and <c>OnDM_AUCTIONEND_ACK</c> (SSHandler.cpp:16429): lots whose time is up
    /// go to the best bidder, or back to the seller.</summary>
    public async Task RunAuctionEnds()
    {
        if (!AuctionEnabled || AuctionStore is not { } db) return;
        long now = UnixNow();
        foreach (var a in _auctions.Values.SelectMany(h => h.All.Values).Where(a => !a.Ending && a.End <= now && a.RetryAt <= now).ToList())
        {
            a.Ending = true;
            var res = await Safe(() => db.AuctionEndAsync(a.NpcId, a.Id), new AuctionEndResult(1, 0, 0, "", "", 0));
            if (res.Ret != 0) { a.Ending = false; a.RetryAt = now + 60; continue; }       // fixed: the C++ left it stuck
            if (res.Buyer == 0)
                await AuctionMail(res.Seller, res.SellerName, MsgAuctionFail, ItemNetText(SvrMsg(MsgAuctionFailBody), a.Item), PostPackage, item: a.Item);
            else
            {
                await AuctionMail(res.Seller, res.SellerName, MsgAuctionPrice, ItemNetText(SvrMsg(MsgAuctionPriceBody), a.Item), PostNormal, res.Price);
                await AuctionMail(res.Buyer, res.BuyerName, MsgAuctionBidSuccess, ItemNetText(SvrMsg(MsgAuctionBidSuccessBody), a.Item), PostPackage, item: a.Item);
            }
            DelAuction(a);
        }
    }
}
