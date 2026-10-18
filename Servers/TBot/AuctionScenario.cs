using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=auction</c>: the auction house at the Craxion auctioneer (NPC 20841). A puts a sword up (2% of the direct
/// price paid), B finds it and bids, can't bid over itself, and A cancels — A gets the sword back by mail, B its bid. A puts a
/// second sword up and B buys it at the direct price — A is paid less 3%, B gets the sword by mail. A third lot with no time and
/// no bidder ends on its own and goes back to A. Letters, lots and bids are checked in the database; A's and B's money, the
/// swords, the letters and the lots are cleaned up after.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_AUCTIONREG_REQ = M + 0x01FD, CS_AUCTIONREG_ACK = M + 0x01FE, CS_AUCTIONREGCANCEL_REQ = M + 0x0205,
        CS_AUCTIONREGCANCEL_ACK = M + 0x0206, CS_AUCTIONBID_REQ = M + 0x0207, CS_AUCTIONBID_ACK = M + 0x0208,
        CS_AUCTIONBUYDIRECT_REQ = M + 0x0209, CS_AUCTIONBUYDIRECT_ACK = M + 0x020A, CS_AUCTIONFIND_REQ = M + 0x020B, CS_AUCTIONFIND_ACK = M + 0x020C;
    private const ushort Auctioneer = 20841;
    private static readonly (byte Slot, ushort Item, long DlId)[] AuctionSwords = { (45, 103, 900_000_031), (46, 101, 900_000_032), (47, 102, 900_000_033) };

    private static async Task<int> RunAuctionAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account), idB = await FirstChar(db, cfg.Account2);
        const string Money = "SELECT dwGold, dwSilver, dwCooper FROM TCHARTABLE WHERE dwCharID=@p0";
        var moneyA = await db.RowAsync(Money, (int)idA);
        var moneyB = await db.RowAsync(Money, (int)idB);
        var maxPost = await db.RowAsync("SELECT ISNULL(MAX(dwPostID), 0) FROM TPOSTTABLE");
        int lastPost = Convert.ToInt32(maxPost![0]);
        try
        {
            await ClearAuctionFixtures(db, idA, idB, lastPost);
            await db.ExecAsync("UPDATE TCHARTABLE SET dwGold=1, dwSilver=0, dwCooper=0 WHERE dwCharID IN (@p0, @p1)", (int)idA, (int)idB);
            foreach (var (slot, item, dl) in AuctionSwords)
                await db.ExecAsync(@"INSERT INTO TITEMTABLE (dlID, bStorageType, dwStorageID, bOwnerType, dwOwnerID, bItemID, wItemID, bLevel,
                    bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime, bGradeEffect, bMagic1, bMagic2, bMagic3, bMagic4, bMagic5,
                    bMagic6, wValue1, wValue2, wValue3, wValue4, wValue5, wValue6, dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6,
                    bGem, wMoggItemID)
                    VALUES (@p0, 0, 255, 0, @p1, @p2, @p3, 0, 1, 0, 0, 0, 0, '1900-01-01', 0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0, 0)",
                    dl, (int)idA, (int)slot, (int)item);
            await AuctionLife(cfg, db, idA, idB, lastPost);
        }
        finally
        {
            await Task.Delay(2500);                                              // the logout saves land first
            int left = await ClearAuctionFixtures(db, idA, idB, lastPost);
            foreach (var (id, row) in new[] { (idA, moneyA), (idB, moneyB) })
                if (row is not null)
                    await db.ExecAsync("UPDATE TCHARTABLE SET dwGold=@p1, dwSilver=@p2, dwCooper=@p3 WHERE dwCharID=@p0", (int)id, row[0], row[1], row[2]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] swords, letters and lots removed, money restored");
            if (left > 0) { Console.WriteLine("lots were left on the map; restarting it ..."); await Docker("compose restart mapsvr"); }
        }
        return PrintResults();
    }

    /// <summary>The run's letters (and their items), the swords wherever they went, and A's lots; returns how many lots there were.</summary>
    private static async Task<int> ClearAuctionFixtures(GameDb db, uint idA, uint idB, int lastPost)
    {
        var lots = await db.RowAsync("SELECT COUNT(*) FROM TAUCTIONTABLE WHERE dwCharID=@p0", (int)idA);
        await db.ExecAsync(@"DELETE FROM TITEMTABLE WHERE dlID IN (SELECT dlItemID FROM TAUCTIONTABLE WHERE dwCharID=@p0);
            DELETE FROM TAUCTIONINTEREST WHERE dwAuctionID IN (SELECT dwAuctionID FROM TAUCTIONTABLE WHERE dwCharID=@p0);
            DELETE FROM TAUCTIONBIDDER WHERE dwAuctionID IN (SELECT dwAuctionID FROM TAUCTIONTABLE WHERE dwCharID=@p0) OR dwCharID=@p1;
            DELETE FROM TAUCTIONTABLE WHERE dwCharID=@p0;
            DELETE FROM TAUCTIONINTEREST WHERE dwCharID IN (@p0, @p1);
            DELETE FROM TITEMTABLE WHERE bStorageType=2 AND bOwnerType=0 AND dwOwnerID IN (@p0, @p1) AND dwStorageID > @p2;
            DELETE FROM TPOSTTABLE WHERE dwCharID IN (@p0, @p1) AND dwPostID > @p2;
            DELETE FROM TITEMTABLE WHERE dlID IN (@p3, @p4, @p5) OR (bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND wItemID IN (101, 102, 103) AND bItemID IN (45, 46, 47))",
            (int)idA, (int)idB, lastPost, AuctionSwords[0].DlId, AuctionSwords[1].DlId, AuctionSwords[2].DlId);
        return Convert.ToInt32(lots![0]);
    }

    private static PacketWriter AuctionReg(byte slot, long direct, long start, ushort hours) => Req(CS_AUCTIONREG_REQ, w =>
    {
        w.WriteUInt16(Auctioneer); w.WriteUInt16(hours); w.WriteByte(0xFF); w.WriteByte(slot); w.WriteInt64(direct); w.WriteInt64(start); w.WriteUInt32(0);
    });

    private static (byte Result, uint Id) AuctionRegAck(PacketReader p) => Read(p, r => (r.ReadByte(), r.ReadUInt32()));

    /// <summary>A CS_AUCTIONFIND_ACK read whole: the result and the lots (id, seller, direct, best bid, item).</summary>
    private static (byte Result, List<(uint Id, string Seller, long Direct, long Bid, ushort Item)> Lots) AuctionList(PacketReader p) => Read(p, r =>
    {
        byte result = r.ReadByte();
        var lots = new List<(uint, string, long, long, ushort)>();
        if (result != 0) return (result, lots);
        r.ReadUInt16(); r.ReadUInt16();
        for (ushort n = r.ReadUInt16(), i = 0; i < n; i++)
        {
            uint id = r.ReadUInt32(); string seller = r.ReadString(); r.ReadInt64(); r.ReadUInt32();
            long direct = r.ReadInt64(); r.ReadInt64(); long bid = r.ReadInt64(); r.ReadByte(); r.ReadUInt32();
            r.ReadByte(); ushort item = r.ReadUInt16(); r.ReadByte(); r.ReadByte(); r.ReadUInt16(); r.ReadUInt16(); r.ReadByte();
            r.ReadUInt32(); r.ReadUInt32(); r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadInt64(); r.ReadByte(); r.ReadByte(); r.ReadByte();
            r.ReadUInt16(); r.ReadUInt16(); r.ReadByte();
            for (byte m = r.ReadByte(), k = 0; k < m; k++) { r.ReadByte(); r.ReadUInt16(); }
            lots.Add((id, seller, direct, bid, item));
        }
        return (result, lots);
    });

    private static long MoneyOf(PacketReader p) => Read(p, r => (long)r.ReadUInt32() * 1_000_000 + r.ReadUInt32() * 1_000L + r.ReadUInt32());

    /// <summary>The run's letters to a character: (sender, money in copper, item in it).</summary>
    private static async Task<List<(string Sender, long Money, int Item)>> Letters(GameDb db, uint charId, int lastPost)
    {
        var list = new List<(string, long, int)>();
        for (int k = 0; ; k++)
        {
            var row = await db.RowAsync(@"SELECT szSender, CAST(dwGold AS bigint) * 1000000 + dwSilver * 1000 + dwCooper,
                ISNULL((SELECT TOP 1 wItemID FROM TITEMTABLE I WHERE I.bStorageType=2 AND I.dwStorageID=P.dwPostID AND I.dwOwnerID=P.dwCharID), 0)
                FROM TPOSTTABLE P WHERE dwCharID=@p0 AND dwPostID > @p1 ORDER BY dwPostID OFFSET @p2 ROWS FETCH NEXT 1 ROWS ONLY",
                (int)charId, lastPost, k);
            if (row is null) return list;
            list.Add((Convert.ToString(row[0]) ?? "", Convert.ToInt64(row[1]), Convert.ToInt32(row[2])));
        }
    }

    private static async Task AuctionLife(BotConfig cfg, GameDb db, uint idA, uint idB, int lastPost)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        a.Enter(); b.Enter();
        Thread.Sleep(1500);
        string nameA = a.Spawn.Name;

        // Lot 1: up, found, bid on, cancelled.
        a.Discard(CS_MONEY_ACK);
        a.Send(AuctionReg(46, 10_000, 1_000, 24));
        var reg = a.TryWait(CS_AUCTIONREG_ACK);
        var (r1, lot1) = reg is null ? ((byte)255, 0u) : AuctionRegAck(reg);
        var paid = a.TryWait(CS_MONEY_ACK);
        Check("auction: A puts a sword up and pays 2% of the direct price (CS_AUCTIONREG_ACK)", r1 == 0 && lot1 != 0
            && paid is not null && MoneyOf(paid) == 1_000_000 - 200, $"{Describe(reg)} / {Describe(paid)}");
        await Task.Delay(500);
        var row = await db.RowAsync("SELECT A.dwCharID, I.bOwnerType, I.wItemID FROM TAUCTIONTABLE A JOIN TITEMTABLE I ON I.dlID = A.dlItemID WHERE A.dwAuctionID=@p0", (int)lot1);
        Check("auction: … the lot is saved, its sword now the auctioneer's (TAUCTIONTABLE, bOwnerType 2)", row is not null
            && Convert.ToUInt32(row[0]) == idA && Convert.ToInt32(row[1]) == 2 && Convert.ToInt32(row[2]) == 101, row is null ? "no row" : string.Join(",", row));

        b.Send(Req(CS_AUCTIONFIND_REQ, w =>
        {
            w.WriteUInt16(Auctioneer); w.WriteByte(0); w.WriteByte(255); w.WriteByte(0); w.WriteByte(255); w.WriteByte(0);
            w.WriteUInt16(1); w.WriteByte(10); w.WriteByte(1); w.WriteUInt16(0);
        }));
        var found = b.TryWait(CS_AUCTIONFIND_ACK);
        var f = found is null ? default : AuctionList(found);
        Check("auction: B finds it (CS_AUCTIONFIND_ACK, seller, prices, the sword)", found is not null && f.Result == 0
            && f.Lots.Any(l => l == (lot1, nameA, 10_000L, 1_000L, (ushort)101)), Describe(found));

        b.Discard(CS_MONEY_ACK);
        b.Send(Req(CS_AUCTIONBID_REQ, w => { w.WriteUInt16(Auctioneer); w.WriteUInt32(lot1); w.WriteInt64(2_000); }));
        var bid = b.TryWait(CS_AUCTIONBID_ACK);
        var bpaid = b.TryWait(CS_MONEY_ACK);
        Check("auction: B bids 2 000, paid at once (CS_AUCTIONBID_ACK)", bid is not null && Read(bid, r => r.ReadByte()) == 0
            && bpaid is not null && MoneyOf(bpaid) == 1_000_000 - 2_000, $"{Describe(bid)} / {Describe(bpaid)}");
        b.Send(Req(CS_AUCTIONBID_REQ, w => { w.WriteUInt16(Auctioneer); w.WriteUInt32(lot1); w.WriteInt64(3_000); }));
        var dup = b.TryWait(CS_AUCTIONBID_ACK);
        Check("auction: … and can't bid over its own bid (ABR_DUPLICATE)", dup is not null && Read(dup, r => r.ReadByte()) == 8, Describe(dup));
        await Task.Delay(300);
        var bidder = await db.RowAsync("SELECT dwCharID, dlBidPrice FROM TAUCTIONBIDDER WHERE dwAuctionID=@p0", (int)lot1);
        Check("auction: … the bid is saved (TAUCTIONBIDDER)", bidder is not null && Convert.ToUInt32(bidder[0]) == idB && Convert.ToInt64(bidder[1]) == 2_000,
            bidder is null ? "no row" : string.Join(",", bidder));

        a.Send(Req(CS_AUCTIONREGCANCEL_REQ, w => { w.WriteUInt16(Auctioneer); w.WriteUInt32(lot1); }));
        var cancel = a.TryWait(CS_AUCTIONREGCANCEL_ACK);
        Check("auction: A cancels the lot (CS_AUCTIONREGCANCEL_ACK)", cancel is not null && Read(cancel, r => r.ReadByte()) == 0, Describe(cancel));
        await Task.Delay(800);
        var la = await Letters(db, idA, lastPost);
        var lb = await Letters(db, idB, lastPost);
        Check("auction: … A gets the sword back by mail, from the Auctioner", la.Any(l => l.Sender == "Auctioner" && l.Item == 101), string.Join(";", la));
        Check("auction: … and B its 2 000 back", lb.Any(l => l.Sender == "Auctioner" && l.Money == 2_000 && l.Item == 0), string.Join(";", lb));

        // Lot 2: bought at the direct price.
        a.Send(AuctionReg(47, 10_000, 1_000, 24));
        var reg2 = a.TryWait(CS_AUCTIONREG_ACK);
        uint lot2 = reg2 is null ? 0 : AuctionRegAck(reg2).Id;
        b.Discard(CS_MONEY_ACK);
        b.Send(Req(CS_AUCTIONBUYDIRECT_REQ, w => { w.WriteUInt16(Auctioneer); w.WriteUInt32(lot2); w.WriteByte(1); }));
        var buy = b.TryWait(CS_AUCTIONBUYDIRECT_ACK);
        var bpaid2 = b.TryWait(CS_MONEY_ACK);
        Check("auction: B buys A's second sword at 10 000 (CS_AUCTIONBUYDIRECT_ACK)", lot2 != 0 && buy is not null && Read(buy, r => r.ReadByte()) == 0
            && bpaid2 is not null && MoneyOf(bpaid2) == 1_000_000 - 2_000 - 10_000, $"{Describe(reg2)} / {Describe(buy)} / {Describe(bpaid2)}");
        await Task.Delay(800);
        la = await Letters(db, idA, lastPost);
        lb = await Letters(db, idB, lastPost);
        Check("auction: … A is paid 10 000 less 3% by mail", la.Any(l => l.Money == 9_700 && l.Item == 0), string.Join(";", la));
        Check("auction: … B gets the sword by mail", lb.Any(l => l.Item == 102), string.Join(";", lb));
        var gone = await db.RowAsync("SELECT COUNT(*) FROM TAUCTIONTABLE WHERE dwAuctionID=@p0", (int)lot2);
        Check("auction: … and the lot is gone (TAUCTIONTABLE)", gone is not null && Convert.ToInt32(gone[0]) == 0, gone is null ? "" : $"{gone[0]} rows");

        // Lot 3: no time, no bidder — it ends and goes back.
        a.Send(AuctionReg(45, 10_000, 1_000, 0));
        var reg3 = a.TryWait(CS_AUCTIONREG_ACK);
        uint lot3 = reg3 is null ? 0 : AuctionRegAck(reg3).Id;
        Check("auction: A puts a third sword up for no time", lot3 != 0, Describe(reg3));
        bool ended = false;
        for (int i = 0; i < 40 && !ended; i++)
        {
            await Task.Delay(5000);
            ended = (await Letters(db, idA, lastPost)).Any(l => l.Item == 103);
        }
        var still = await db.RowAsync("SELECT COUNT(*) FROM TAUCTIONTABLE WHERE dwAuctionID=@p0", (int)lot3);
        Check("auction: … with no bidder it ends and the sword goes back to A by mail", ended && still is not null && Convert.ToInt32(still[0]) == 0,
            $"letter: {ended}, lot rows: {still?[0]}");
    }
}
