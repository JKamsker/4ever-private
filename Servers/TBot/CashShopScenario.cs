using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=cashshop</c>: the cash shop on the live procs (migration 022) and the test balance (<c>TGLOBAL_GSP.TCASHTESTTABLE</c>).
/// A's account is given 1000 cash and 10 bonus. A opens the shop (its balance), buys the cheapest item that may go to a cabinet (the
/// cash goes down, the item is in A's cabinet), takes it out into a backpack slot (the item table has it) and puts it back in (it
/// leaves the bag), buys cabinet time, sends B a present (B's account cabinet gets it, B gets a letter), and is refused with no cash
/// left. After: the balances, A's cabinet time, the cabinets' new rows, the item and B's letters are put back.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_CASHITEMCABINET_REQ = M + 0x0192, CS_CASHITEMCABINET_ACK = M + 0x0193, CS_CASHITEMGET_REQ = M + 0x0194,
        CS_CASHITEMGET_ACK = M + 0x0195, CS_CASHSHOPITEMLIST_REQ = M + 0x0196, CS_CASHSHOPITEMLIST_ACK = M + 0x0197,
        CS_CASHITEMBUY_REQ = M + 0x0198, CS_CASHITEMBUY_ACK = M + 0x0199, CS_CASHITEMPRESENT_REQ = M + 0x01B7,
        CS_CASHCABINETBUY_REQ = M + 0x01C4, CS_CASHCABINETBUY_ACK = M + 0x01C5, CS_CASHITEMPUTIN_REQ = M + 0x01C6,
        CS_CASHITEMPUTIN_ACK = M + 0x01C7;
    private const byte CashBag = 0xFF, CashSlot = 45;

    private static async Task<int> RunCashShopAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account), idB = await FirstChar(db, cfg.Account2);
        var users = await db.RowAsync("SELECT (SELECT dwUserID FROM TCHARTABLE WHERE dwCharID=@p0), (SELECT dwUserID FROM TCHARTABLE WHERE dwCharID=@p1)",
            (int)idA, (int)idB);
        int userA = Convert.ToInt32(users![0]), userB = Convert.ToInt32(users[1]);
        var pick = await db.RowAsync(@"SELECT TOP 1 v.wID, v.wItemID, v.dwMoney FROM TVIEW_CASHSHOPITEMCHART v JOIN TITEMCHART i ON i.wItemID = v.wItemID
            WHERE v.bCanSell = 1 AND v.dwMoney > 0 AND (i.bIsSell & 4) <> 0 AND i.bItemCountry = 3 ORDER BY v.dwMoney, v.wID");
        if (pick is null) { Report("cashshop", "no cash item that may go to a cabinet"); return 2; }
        ushort shopItem = Convert.ToUInt16(pick[0]), itemId = Convert.ToUInt16(pick[1]);
        uint price = Convert.ToUInt32(pick[2]);
        Report("cashshop", $"shop item {shopItem} (item {itemId}) at {price}");

        int lastCabinet = Convert.ToInt32((await db.RowAsync("SELECT ISNULL(MAX(dwID), 0) FROM TGLOBAL_GSP.dbo.TCASHITEMCABINETTABLE"))![0]);
        int lastPost = Convert.ToInt32((await db.RowAsync("SELECT ISNULL(MAX(dwPostID), 0) FROM TPOSTTABLE"))![0]);
        var cashRows = await db.RowAsync(@"SELECT (SELECT COUNT(*) FROM TGLOBAL_GSP.dbo.TCASHTESTTABLE WHERE dwUserID IN (@p0, @p1))", userA, userB);
        if (Convert.ToInt32(cashRows![0]) != 0) { Report("cashshop", "the bot accounts already have a cash row; not touching it"); return 2; }
        var cabinetUse = await db.RowAsync("SELECT dCabinetUse FROM TGLOBAL_GSP.dbo.TUSERINFOTABLE WHERE dwUserID=@p0", userA);
        try
        {
            await db.ExecAsync("DELETE FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND dwStorageID=255 AND bItemID=@p1",
                (int)idA, (int)CashSlot);
            await db.ExecAsync("INSERT INTO TGLOBAL_GSP.dbo.TCASHTESTTABLE (dwUserID, dwCash, dwBonus) VALUES (@p0, 1000, 10)", userA);
            CashShop(cfg, shopItem, itemId, price, db, idA, userA, userB);
        }
        finally
        {
            await Task.Delay(2500);                                                             // the logout save lands first
            await db.ExecAsync("DELETE FROM TGLOBAL_GSP.dbo.TCASHTESTTABLE WHERE dwUserID IN (@p0, @p1)", userA, userB);
            await db.ExecAsync("DELETE FROM TGLOBAL_GSP.dbo.TCASHITEMCABINETTABLE WHERE dwID > @p0 AND dwUserID IN (@p1, @p2)", lastCabinet, userA, userB);
            if (cabinetUse is null) await db.ExecAsync("DELETE FROM TGLOBAL_GSP.dbo.TUSERINFOTABLE WHERE dwUserID=@p0 AND bCanCreateCharCount=6 AND bAgreement=1", userA);
            else await db.ExecAsync("UPDATE TGLOBAL_GSP.dbo.TUSERINFOTABLE SET dCabinetUse=@p1 WHERE dwUserID=@p0", userA, cabinetUse[0]);
            await db.ExecAsync("DELETE FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND dwStorageID=255 AND bItemID=@p1",
                (int)idA, (int)CashSlot);
            await db.ExecAsync("DELETE FROM TPOSTTABLE WHERE dwCharID=@p0 AND dwPostID > @p1", (int)idB, lastPost);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] balances, cabinet time and rows, the item and B's letters put back");
        }
        return PrintResults();
    }

    private static void CashShop(BotConfig cfg, ushort shopItem, ushort itemId, uint price, GameDb db, uint idA, int userA, int userB)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        a.Enter(); b.Enter();
        Thread.Sleep(1500);

        a.Send(Req(CS_CASHSHOPITEMLIST_REQ, w => w.WriteUInt16(0)));
        var list = a.TryWait(CS_CASHSHOPITEMLIST_ACK);
        var bal = list is null ? default : (list.ReadUInt32(), list.ReadUInt32(), list.ReadByte());
        Check("cashshop: the shop shows the balance and its tabs (CS_CASHSHOPITEMLIST_ACK)", list is not null && bal.Item1 == 1000 && bal.Item2 == 10
            && bal.Item3 > 0, $"{bal}");

        (byte, uint, uint)? Buy(PacketReader? p) => p is null ? null : (p.ReadByte(), p.ReadUInt32(), p.ReadUInt32());
        a.Send(Req(CS_CASHITEMBUY_REQ, w => w.WriteUInt16(shopItem)));
        var bought = Buy(a.TryWait(CS_CASHITEMBUY_ACK));
        Check($"cashshop: buying takes {price} cash (CS_CASHITEMBUY_ACK)", bought == ((byte)0, 1000 - price, 10u), $"{bought}");

        a.Send(Req(CS_CASHITEMCABINET_REQ, _ => { }));
        var cab = a.TryWait(CS_CASHITEMCABINET_ACK);
        uint cabinetId = 0;
        if (cab is not null)
        {
            cab.ReadInt64(); cab.ReadUInt32(); cab.ReadUInt32();
            for (ushort n = cab.ReadUInt16(), i = 0; i < n; i++)
            {
                uint id = cab.ReadUInt32();
                if (MiniItem(cab).Id == itemId) cabinetId = id;
            }
        }
        Check("cashshop: the bought item is in A's cabinet (CS_CASHITEMCABINET_ACK)", cabinetId != 0, $"cabinet id {cabinetId}");

        a.Send(Req(CS_CASHITEMGET_REQ, w => { w.WriteUInt32(cabinetId); w.WriteByte(CashBag); w.WriteByte(CashSlot); }));
        var got = a.TryWait(CS_CASHITEMGET_ACK);
        var row = db.RowAsync("SELECT wItemID FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND dwStorageID=255 AND bItemID=@p1",
            (int)idA, (int)CashSlot).GetAwaiter().GetResult();
        Check("cashshop: taking it out puts it in the asked backpack slot (CS_CASHITEMGET_ACK, TITEMTABLE)", got is not null
            && got.ReadByte() == 0 && row is not null && Convert.ToUInt16(row[0]) == itemId, Describe(got));

        a.Send(Req(CS_CASHITEMPUTIN_REQ, w => { w.WriteByte(CashBag); w.WriteByte(CashSlot); }));
        var put = a.TryWait(CS_CASHITEMPUTIN_ACK);
        Thread.Sleep(300);
        var gone = db.RowAsync("SELECT COUNT(*) FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND dwStorageID=255 AND bItemID=@p1",
            (int)idA, (int)CashSlot).GetAwaiter().GetResult();
        Check("cashshop: putting it back in moves it to the cabinet (CS_CASHITEMPUTIN_ACK)", put is not null && put.ReadByte() == 0
            && put.ReadUInt32() != 0 && Convert.ToInt32(gone![0]) == 0, Describe(put));

        a.Send(Req(CS_CASHCABINETBUY_REQ, w => w.WriteUInt16(shopItem)));
        var time = a.TryWait(CS_CASHCABINETBUY_ACK);
        var t = time is null ? default : (time.ReadByte(), time.ReadInt64(), time.ReadUInt32());
        Check("cashshop: cabinet time is bought, with its new time (CS_CASHCABINETBUY_ACK)", time is not null && t.Item1 == 0 && t.Item2 > 0
            && t.Item3 == 1000 - 2 * price, $"{t}");

        a.Send(Req(CS_CASHITEMPRESENT_REQ, w => { w.WriteString(b.Spawn.Name); w.WriteUInt16(shopItem); }));
        var present = Buy(a.TryWait(CS_CASHITEMBUY_ACK));
        var letter = b.TryWait(CS_POSTRECV_ACK);
        var forB = db.RowAsync("SELECT COUNT(*) FROM TGLOBAL_GSP.dbo.TCASHITEMCABINETTABLE WHERE dwUserID=@p0 AND wItemID=@p1", userB, (int)itemId)
            .GetAwaiter().GetResult();
        Check("cashshop: a present goes to B's cabinet, with a letter", present is { Item1: 0 } && letter is not null
            && Convert.ToInt32(forB![0]) > 0, $"{present}, letter {letter is not null}");

        db.ExecAsync("UPDATE TGLOBAL_GSP.dbo.TCASHTESTTABLE SET dwCash = 0 WHERE dwUserID=@p0", userA).GetAwaiter().GetResult();
        a.Send(Req(CS_CASHITEMBUY_REQ, w => w.WriteUInt16(shopItem)));
        var broke = Buy(a.TryWait(CS_CASHITEMBUY_ACK));
        Check("cashshop: with no cash left the buy is refused (CASHSHOP_NEEDCASH)", broke is { Item1: 3 }, $"{broke}");
    }
}
