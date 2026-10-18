using System.Data;
using Microsoft.Data.SqlClient;

namespace TMap.Data;

/// <summary>An auction on load (C++ <c>CTBLAuctionTable</c> + <c>CTBLALLAuctionBidder</c>): the lot, its item and its best bid
/// (no bidder: 0, and the price is the start price).</summary>
public sealed record AuctionRow(uint Id, ushort NpcId, uint Seller, string SellerName, byte BidCount, long Direct, long Start,
    long DateStart, long DateEnd, FullItemRow Item, uint BidderChar, uint BidderUser, long BidPrice, long BidTime);

/// <summary>A character's auction lists (C++ <c>CTBLAuctionInterest</c> / <c>CTBLAuctionBidList</c> / <c>CTBLAuctionRegList</c>).</summary>
public sealed record AuctionLists(List<uint> Interest, List<uint> Bid, List<uint> Reg);

public readonly record struct AuctionRegResult(int Ret, uint Id, long Start, long End);
public readonly record struct AuctionCancelResult(int Ret, string Seller, string Bidder, uint BidderId, long BidPrice);
public readonly record struct AuctionBidResult(int Ret, long DropPrice, uint DropBidder, string DropName, long BidTime);
public readonly record struct AuctionBuyResult(int Ret, uint Seller, string SellerName, uint DropBidder, string DropName, long DropPrice);
public readonly record struct AuctionEndResult(int Ret, uint Buyer, uint Seller, string BuyerName, string SellerName, long Price);

/// <summary>
/// The auction house in the database (<c>TAUCTIONTABLE</c>, <c>TAUCTIONBIDDER</c>, <c>TAUCTIONINTEREST</c> and the <c>TAuction*</c>
/// procs; C++ DBAccess.h:3997–4170, 6917–7050). Behind an interface so the map logic can be tested against an in-memory fake.
/// </summary>
public interface IAuctionStore
{
    Task<List<AuctionRow>> LoadAuctionsAsync();
    Task<AuctionLists> LoadAuctionListsAsync(uint charId);
    /// <summary>C++ <c>CSPSaveItemDirect</c> before <c>TAuctionReg</c>: the lot's item row, still the seller's.</summary>
    Task SaveAuctionItemAsync(uint seller, ItemSaveData item);
    /// <summary><c>TAuctionReg</c>: the item row (already saved) becomes the NPC's; 0 done, 3 no seller, 4 bad prices, 6 no item.</summary>
    Task<AuctionRegResult> AuctionRegAsync(ushort npcId, ushort hours, uint seller, long direct, long start, long dlId);
    Task<AuctionCancelResult> AuctionRegCancelAsync(ushort npcId, uint auctionId);
    Task<AuctionBidResult> AuctionBidAsync(ushort npcId, uint auctionId, uint bidder, long bid);
    Task<AuctionBuyResult> AuctionBuyDirectAsync(ushort npcId, uint auctionId, uint buyer, byte count, long unitPrice);
    Task<AuctionEndResult> AuctionEndAsync(ushort npcId, uint auctionId);
    /// <summary><c>TSaveAuctionInterest</c>: the watch list replaced.</summary>
    Task SaveAuctionInterestAsync(uint charId, IReadOnlyList<uint> ids);
}

public sealed partial class GameDatabase : IAuctionStore
{
    private const string AuctionSql = @"
SELECT A.dwAuctionID, A.wNpcID, A.dwCharID, S.szName, A.bBidCount, A.dlDirectPrice, A.dlStartPrice, A.DateStart, A.DateEnd,
       I.bStorageType, I.dwStorageID, I.bItemID, I.wItemID, I.bLevel, I.bCount, I.bGLevel, I.dwDuraMax, I.dwDuraCur,
       I.bRefineCur, I.dEndTime, I.bGradeEffect,
       I.bMagic1, I.bMagic2, I.bMagic3, I.bMagic4, I.bMagic5, I.bMagic6,
       I.wValue1, I.wValue2, I.wValue3, I.wValue4, I.wValue5, I.wValue6,
       I.dwTime1, I.dwTime2, I.dwTime3, I.dwTime4, I.dwTime5, I.dwTime6,
       I.bGem, I.wMoggItemID, I.dlID,
       B.dwCharID, BC.dwUserID, B.dlBidPrice, B.DateBid
FROM TAUCTIONTABLE A
JOIN TCHARTABLE S ON S.dwCharID = A.dwCharID
JOIN TITEMTABLE I ON I.dlID = A.dlItemID
LEFT JOIN TAUCTIONBIDDER B ON B.dwAuctionID = A.dwAuctionID
LEFT JOIN TCHARTABLE BC ON BC.dwCharID = B.dwCharID";

    public async Task<List<AuctionRow>> LoadAuctionsAsync()
    {
        await using var c = await OpenAsync(default);
        await using var cmd = new SqlCommand(AuctionSql, c);
        await using var r = await cmd.ExecuteReaderAsync();
        var list = new List<AuctionRow>();
        while (await r.ReadAsync())
        {
            const int i0 = 9;
            var magic = new byte[6]; var value = new ushort[6]; var ext = new uint[6];
            for (int i = 0; i < 6; i++) magic[i] = r.GetByteSafe(i0 + 12 + i);
            for (int i = 0; i < 6; i++) value[i] = r.GetUShortSafe(i0 + 18 + i);
            for (int i = 0; i < 6; i++) ext[i] = r.GetUIntSafe(i0 + 24 + i);
            var item = new FullItemRow(r.GetByteSafe(i0), r.GetUIntSafe(i0 + 1), r.GetByteSafe(i0 + 2), r.GetUShortSafe(i0 + 3),
                r.GetByteSafe(i0 + 4), r.GetByteSafe(i0 + 5), r.GetByteSafe(i0 + 6), r.GetUIntSafe(i0 + 7), r.GetUIntSafe(i0 + 8),
                r.GetByteSafe(i0 + 9), ToTime64(r, i0 + 10), r.GetByteSafe(i0 + 11), magic, value, ext, r.GetByteSafe(i0 + 30),
                r.GetUShortSafe(i0 + 31), r.GetInt64Safe(i0 + 32));
            const int b = i0 + 33;
            bool bid = !r.IsDBNull(b);
            long start = r.GetInt64Safe(6);
            list.Add(new AuctionRow(r.GetUIntSafe(0), r.GetUShortSafe(1), r.GetUIntSafe(2), r.GetStringSafe(3), r.GetByteSafe(4),
                r.GetInt64Safe(5), start, ToTime64(r, 7), ToTime64(r, 8), item,
                bid ? r.GetUIntSafe(b) : 0, bid ? r.GetUIntSafe(b + 1) : 0, bid ? r.GetInt64Safe(b + 2) : start, bid ? ToTime64(r, b + 3) : 0));
        }
        return list;
    }

    public Task SaveAuctionItemAsync(uint seller, ItemSaveData item) => SaveItemDirectAsync(seller, item);

    public async Task<AuctionLists> LoadAuctionListsAsync(uint charId)
    {
        await using var c = await OpenAsync(default);
        await using var cmd = new SqlCommand(@"SELECT dwAuctionID FROM TAUCTIONINTEREST WHERE dwCharID = @c;
SELECT dwAuctionID FROM TAUCTIONBIDDER WHERE dwCharID = @c;
SELECT dwAuctionID FROM TAUCTIONTABLE WHERE dwCharID = @c", c);
        cmd.Parameters.Add(SqlProc.In("@c", SqlDbType.Int, unchecked((int)charId)));
        await using var r = await cmd.ExecuteReaderAsync();
        var lists = new List<uint>[3];
        for (int k = 0; k < 3; k++)
        {
            lists[k] = new List<uint>();
            while (await r.ReadAsync()) lists[k].Add(r.GetUIntSafe(0));
            await r.NextResultAsync();
        }
        return new AuctionLists(lists[0], lists[1], lists[2]);
    }

    private static long Secs(SqlParameter p)
    {
        if (p.Value is not DateTime dt || dt.Year < 2000) return 0;
        return (long)(DateTime.SpecifyKind(dt, DateTimeKind.Utc) - DateTime.UnixEpoch).TotalSeconds;
    }

    private static string Str(SqlParameter p) => p.Value is DBNull or null ? "" : p.Value.ToString() ?? "";
    private static uint UInt(SqlParameter p) => p.Value is DBNull or null ? 0u : unchecked((uint)Convert.ToInt32(p.Value));
    private static long Long(SqlParameter p) => p.Value is DBNull or null ? 0L : Convert.ToInt64(p.Value);

    public async Task<AuctionRegResult> AuctionRegAsync(ushort npcId, ushort hours, uint seller, long direct, long start, long dlId)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        var id = SqlProc.Out("@p6", SqlDbType.Int);
        var ds = SqlProc.Out("@p7", SqlDbType.SmallDateTime);
        var de = SqlProc.Out("@p8", SqlDbType.SmallDateTime);
        await SqlProc.ExecAsync(c, "TAuctionReg", ret, new[]
        {
            SqlProc.In("@p0", SqlDbType.SmallInt, unchecked((short)npcId)), SqlProc.In("@p1", SqlDbType.SmallInt, unchecked((short)hours)),
            SqlProc.In("@p2", SqlDbType.Int, unchecked((int)seller)), SqlProc.In("@p3", SqlDbType.BigInt, direct),
            SqlProc.In("@p4", SqlDbType.BigInt, start), SqlProc.In("@p5", SqlDbType.BigInt, dlId), id, ds, de,
        }, default);
        return new AuctionRegResult(ret.AsInt(), UInt(id), Secs(ds), Secs(de));
    }

    public async Task<AuctionCancelResult> AuctionRegCancelAsync(ushort npcId, uint auctionId)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        var seller = SqlProc.Out("@p2", SqlDbType.VarChar, 50);
        var bidder = SqlProc.Out("@p3", SqlDbType.VarChar, 50);
        var bidderId = SqlProc.Out("@p4", SqlDbType.Int);
        var price = SqlProc.Out("@p5", SqlDbType.BigInt);
        await SqlProc.ExecAsync(c, "TAuctionRegCancel", ret, new[]
        {
            SqlProc.In("@p0", SqlDbType.SmallInt, unchecked((short)npcId)), SqlProc.In("@p1", SqlDbType.Int, unchecked((int)auctionId)),
            seller, bidder, bidderId, price,
        }, default);
        return new AuctionCancelResult(ret.AsInt(), Str(seller), Str(bidder), UInt(bidderId), Long(price));
    }

    public async Task<AuctionBidResult> AuctionBidAsync(ushort npcId, uint auctionId, uint bidder, long bid)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        var dropPrice = SqlProc.Out("@p4", SqlDbType.BigInt);
        var dropBidder = SqlProc.Out("@p5", SqlDbType.Int);
        var dropName = SqlProc.Out("@p6", SqlDbType.VarChar, 50);
        var time = SqlProc.Out("@p7", SqlDbType.SmallDateTime);
        await SqlProc.ExecAsync(c, "TAuctionBid", ret, new[]
        {
            SqlProc.In("@p0", SqlDbType.SmallInt, unchecked((short)npcId)), SqlProc.In("@p1", SqlDbType.Int, unchecked((int)auctionId)),
            SqlProc.In("@p2", SqlDbType.Int, unchecked((int)bidder)), SqlProc.In("@p3", SqlDbType.BigInt, bid),
            dropPrice, dropBidder, dropName, time,
        }, default);
        return new AuctionBidResult(ret.AsInt(), Long(dropPrice), UInt(dropBidder), Str(dropName), Secs(time));
    }

    public async Task<AuctionBuyResult> AuctionBuyDirectAsync(ushort npcId, uint auctionId, uint buyer, byte count, long unitPrice)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        var seller = SqlProc.Out("@p5", SqlDbType.Int);
        var sellerName = SqlProc.Out("@p6", SqlDbType.VarChar, 50);
        var dropBidder = SqlProc.Out("@p7", SqlDbType.Int);
        var dropName = SqlProc.Out("@p8", SqlDbType.VarChar, 50);
        var dropPrice = SqlProc.Out("@p9", SqlDbType.BigInt);
        await SqlProc.ExecAsync(c, "TAuctionBuyDirect", ret, new[]
        {
            SqlProc.In("@p0", SqlDbType.SmallInt, unchecked((short)npcId)), SqlProc.In("@p1", SqlDbType.Int, unchecked((int)auctionId)),
            SqlProc.In("@p2", SqlDbType.Int, unchecked((int)buyer)), SqlProc.In("@p3", SqlDbType.TinyInt, count),
            SqlProc.In("@p4", SqlDbType.BigInt, unitPrice), seller, sellerName, dropBidder, dropName, dropPrice,
        }, default);
        return new AuctionBuyResult(ret.AsInt(), UInt(seller), Str(sellerName), UInt(dropBidder), Str(dropName), Long(dropPrice));
    }

    public async Task<AuctionEndResult> AuctionEndAsync(ushort npcId, uint auctionId)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        var buyer = SqlProc.Out("@p2", SqlDbType.Int);
        var seller = SqlProc.Out("@p3", SqlDbType.Int);
        var buyerName = SqlProc.Out("@p4", SqlDbType.VarChar, 50);
        var sellerName = SqlProc.Out("@p5", SqlDbType.VarChar, 50);
        var price = SqlProc.Out("@p6", SqlDbType.BigInt);
        await SqlProc.ExecAsync(c, "TAuctionEnd", ret, new[]
        {
            SqlProc.In("@p0", SqlDbType.SmallInt, unchecked((short)npcId)), SqlProc.In("@p1", SqlDbType.Int, unchecked((int)auctionId)),
            buyer, seller, buyerName, sellerName, price,
        }, default);
        return new AuctionEndResult(ret.AsInt(), UInt(buyer), UInt(seller), Str(buyerName), Str(sellerName), Long(price));
    }

    public async Task SaveAuctionInterestAsync(uint charId, IReadOnlyList<uint> ids)
    {
        await using var c = await OpenAsync(default);
        foreach (uint id in new uint[] { 0 }.Concat(ids))
            await SqlProc.ExecAsync(c, "TSaveAuctionInterest", null, new[]
            {
                SqlProc.In("@p0", SqlDbType.Int, unchecked((int)charId)), SqlProc.In("@p1", SqlDbType.Int, unchecked((int)id)),
            }, default);
    }
}
