using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=minigame</c>: the mini-games, on A alone. A has its gamble fee quoted and gambles a sealed potion (6704, gamble
/// level 40: 2842 copper) that always turns into another (8614); buys a sabre from the magic shop (NPC 26319, 10 000 copper, 3 days);
/// asks for the soul lottery with an empty bar (no answer); then plays rock-paper-scissors with tickets (9416) until it wins once and
/// cashes out — 2 silver by the operator's mail. The saved items are checked in the database; the potion, sabre, tickets, letters and
/// A's money are put back after. The soul bar and the party loot lottery, which need kills, follow (MinigameKillScenario.cs).
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_GAMBLECHECK_REQ = M + 0x01A4, CS_GAMBLECHECK_ACK = M + 0x01A5, CS_GAMBLEOPEN_REQ = M + 0x01A6,
        CS_GAMBLEOPEN_ACK = M + 0x01A7, CS_MAGICITEMBUY_REQ = M + 0x0256, CS_MAGICITEMBUY_ACK = M + 0x0257,
        CS_SOULLOTTERY_REQ = M + 0x027B, CS_SOULLOTTERY_ACK = M + 0x027C, CS_RPSSTART_REQ = M + 0x0246, CS_RPSSTART_ACK = M + 0x0247,
        CS_RPSGAME_REQ = M + 0x0248, CS_RPSGAME_ACK = M + 0x0249;
    private const ushort FortuneTeller = 22228, MagicShop = 26319, SealedPotion = 6704, GambledPotion = 8614, ShopSabre = 11517, RpsTicket = 9416;
    private const byte GambleSlot = 45, TicketSlot = 46;
    private const long GambleDl = 900_000_041, TicketDl = 900_000_042;

    private static async Task<int> RunMinigameAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account);
        var money = await db.RowAsync("SELECT dwGold, dwSilver, dwCooper FROM TCHARTABLE WHERE dwCharID=@p0", (int)idA);
        var maxPost = await db.RowAsync("SELECT ISNULL(MAX(dwPostID), 0) FROM TPOSTTABLE");
        int lastPost = Convert.ToInt32(maxPost![0]);
        try
        {
            await ClearMinigameFixtures(db, idA, lastPost);
            await db.ExecAsync("UPDATE TCHARTABLE SET dwGold=1, dwSilver=0, dwCooper=0 WHERE dwCharID=@p0", (int)idA);
            foreach (var (dl, slot, item, count, glevel) in new[] { (GambleDl, GambleSlot, SealedPotion, 1, 40), (TicketDl, TicketSlot, RpsTicket, 10, 0) })
                await db.ExecAsync(@"INSERT INTO TITEMTABLE (dlID, bStorageType, dwStorageID, bOwnerType, dwOwnerID, bItemID, wItemID, bLevel,
                    bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime, bGradeEffect, bMagic1, bMagic2, bMagic3, bMagic4, bMagic5,
                    bMagic6, wValue1, wValue2, wValue3, wValue4, wValue5, wValue6, dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6,
                    bGem, wMoggItemID)
                    VALUES (@p0, 0, 255, 0, @p1, @p2, @p3, 0, @p4, @p5, 0, 0, 0, '1900-01-01', 0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0, 0)",
                    dl, (int)idA, (int)slot, (int)item, count, glevel);
            await Minigames(cfg, db, idA, lastPost);
        }
        finally
        {
            await Task.Delay(2500);                                              // the logout save lands first
            await ClearMinigameFixtures(db, idA, lastPost);
            if (money is not null)
                await db.ExecAsync("UPDATE TCHARTABLE SET dwGold=@p1, dwSilver=@p2, dwCooper=@p3 WHERE dwCharID=@p0", (int)idA, money[0], money[1], money[2]);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] potion, sabre, tickets and letters removed, money restored");
        }
        await MinigameKillsAsync(cfg, db, idA, await FirstChar(db, cfg.Account2));   // MinigameKillScenario.cs
        return PrintResults();
    }

    private static Task ClearMinigameFixtures(GameDb db, uint idA, int lastPost) => db.ExecAsync(
        @"DELETE FROM TITEMTABLE WHERE bStorageType=2 AND bOwnerType=0 AND dwOwnerID=@p0 AND dwStorageID > @p1;
          DELETE FROM TPOSTTABLE WHERE dwCharID=@p0 AND dwPostID > @p1;
          DELETE FROM TITEMTABLE WHERE dlID IN (@p2, @p3) OR (bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0
            AND (wItemID=@p4 OR (bItemID IN (@p5, @p6) AND wItemID IN (@p7, @p8, @p9))))",
        (int)idA, lastPost, GambleDl, TicketDl, (int)ShopSabre, (int)GambleSlot, (int)TicketSlot, (int)SealedPotion, (int)GambledPotion, (int)RpsTicket);

    /// <summary>An item block sent without its slot (C++ <c>WrapPacketClient(…, FALSE)</c>): its id and count.</summary>
    private static (ushort Id, byte Count) MiniItem(PacketReader r)
    {
        ushort id = r.ReadUInt16(); r.ReadByte(); r.ReadByte(); r.ReadUInt16(); r.ReadUInt16();
        byte count = r.ReadByte();
        r.ReadUInt32(); r.ReadUInt32(); r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadInt64();
        r.ReadByte(); r.ReadByte(); r.ReadByte(); r.ReadUInt16(); r.ReadUInt16(); r.ReadByte();
        for (byte m = r.ReadByte(), k = 0; k < m; k++) { r.ReadByte(); r.ReadUInt16(); }
        return (id, count);
    }

    private static PacketWriter RpsStart(byte slot) => Req(CS_RPSSTART_REQ, w => { w.WriteByte(1); w.WriteByte(0xFF); w.WriteByte(slot); });

    private static async Task Minigames(BotConfig cfg, GameDb db, uint idA, int lastPost)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        a.Enter();
        Thread.Sleep(1500);

        // The gamble: quoted, then opened.
        a.Send(Req(CS_GAMBLECHECK_REQ, w => { w.WriteByte(0xFF); w.WriteByte(GambleSlot); w.WriteUInt16(FortuneTeller); }));
        var check = a.TryWait(CS_GAMBLECHECK_ACK);
        Check("gamble: the fee is quoted from the gamble level (CS_GAMBLECHECK_ACK 2842)", check is not null
            && Read(check, r => (r.ReadByte(), r.ReadByte(), r.ReadByte(), r.ReadUInt32(), r.ReadByte())) == (0, 0xFF, GambleSlot, 2842u, 0), Describe(check));

        a.Discard(CS_MONEY_ACK);
        a.Send(Req(CS_GAMBLEOPEN_REQ, w => { w.WriteByte(0xFF); w.WriteByte(GambleSlot); w.WriteUInt16(FortuneTeller); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        var open = a.TryWait(CS_GAMBLEOPEN_ACK);
        var opened = open is null ? default : Read(open, r => (Result: r.ReadByte(), Money: MoneyOf3(r), Inv: r.ReadByte(), Slot: r.ReadByte(), Item: MiniItem(r)));
        Check("gamble: the sealed potion turns into another, the fee paid (CS_GAMBLEOPEN_ACK)", open is not null && opened.Result == 0
            && opened.Money == 1_000_000 - 2842 && (opened.Inv, opened.Slot) == (0xFF, GambleSlot) && opened.Item.Id == GambledPotion
            && opened.Item.Count is >= 1 and <= 30, Describe(open));
        var paid = a.TryWait(CS_MONEY_ACK);
        Check("gamble: … and the money is sent (CS_MONEY_ACK)", paid is not null && MoneyOf(paid) == 1_000_000 - 2842, Describe(paid));

        // The magic shop.
        a.Discard(CS_ADDITEM_ACK);
        a.Send(Req(CS_MAGICITEMBUY_REQ, w => { w.WriteUInt16(MagicShop); w.WriteUInt32(125); w.WriteByte(1); w.WriteByte(0xFF); w.WriteByte(0xFF); }));
        var buy = a.TryWait(CS_MAGICITEMBUY_ACK);
        Check("magic shop: A buys a sabre for 10 000 (CS_MAGICITEMBUY_ACK)", buy is not null
            && Read(buy, r => (r.ReadByte(), MoneyOf3(r))) == (0, 1_000_000 - 2842 - 10_000), Describe(buy));

        // The soul lottery: an empty bar gets no answer.
        a.Send(Req(CS_SOULLOTTERY_REQ, _ => { }));
        var soul = a.TryWait(CS_SOULLOTTERY_ACK, timeoutMs: 1500);
        Check("soul lottery: refused while the soul bar is not full", soul is null, Describe(soul));

        // Rock-paper-scissors.
        a.Send(RpsStart(47));
        var noTicket = a.TryWait(CS_RPSSTART_ACK);
        Check("rps: no ticket in that slot, no game (RPS_NEEDITEM)", noTicket is not null
            && Read(noTicket, r => (r.ReadByte(), MoneyOf3(r))).Item1 == 3, Describe(noTicket));

        bool started = true, cards = true, cashed = false;
        int games = 0, plays = 0;
        for (; games < 10 && !cashed && started; games++)
        {
            a.Send(RpsStart(TicketSlot));
            var st = a.TryWait(CS_RPSSTART_ACK);
            started = st is not null && Read(st, r => (r.ReadByte(), MoneyOf3(r))).Item1 == 0;
            for (byte win = 0; started && plays < 200; plays++)
            {
                a.Send(Req(CS_RPSGAME_REQ, w => w.WriteByte(0)));          // rock
                var g = a.TryWait(CS_RPSGAME_ACK);
                if (g is null) { cards = false; break; }
                var (w, player, npc) = Read(g, r => (r.ReadByte(), r.ReadByte(), r.ReadByte()));
                cards &= player == 0 && npc switch { 0 => w == win, 2 => w == win + 1, 1 => w == 0, _ => false };
                if (npc == 1) break;                                         // paper: lost
                if (npc == 2) { a.Send(Req(CS_RPSGAME_REQ, w2 => w2.WriteByte(3))); cashed = true; break; }   // won: cash out
            }
        }
        Check("rps: each ticket starts a game (CS_RPSSTART_ACK)", started && games > 0, $"{games} game(s)");
        Check("rps: every NPC card matches its result (CS_RPSGAME_ACK)", cards, $"{plays} play(s)");
        Check("rps: A wins once and cashes out", cashed, $"{games} game(s), {plays} play(s)");
        var notified = a.TryWait(CS_POSTRECV_ACK);
        Check("rps: the reward letter is announced (CS_POSTRECV_ACK)", notified is not null, Describe(notified));
        a.Dispose();                                                        // log out: the items and money are saved

        await Task.Delay(2500);
        var letters = await Letters(db, idA, lastPost);
        Check("rps: … from the operator with 2 silver (TPOSTTABLE)", letters.Count == 1 && letters[0].Sender == "Operator" && letters[0].Money == 2_000,
            string.Join("; ", letters));
        var tickets = await db.RowAsync("SELECT bCount FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND bItemID=@p1 AND wItemID=@p2",
            (int)idA, (int)TicketSlot, (int)RpsTicket);
        Check("saved: one ticket used per game", (tickets is null ? 0 : Convert.ToInt32(tickets[0])) == 10 - games,
            tickets is null ? "no tickets left" : $"{tickets[0]} left after {games} game(s)");
        var potion = await db.RowAsync("SELECT wItemID, bGLevel FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0 AND bStorageType=0 AND bItemID=@p1",
            (int)idA, (int)GambleSlot);
        Check("saved: the gambled potion in its slot, unsealed", potion is not null && Convert.ToInt32(potion[0]) == GambledPotion
            && Convert.ToInt32(potion[1]) == 0, potion is null ? "no row" : string.Join(",", potion));
        var sabre = await db.RowAsync(@"SELECT DATEDIFF(hour, GETDATE(), dEndTime) FROM TITEMTABLE WHERE bOwnerType=0 AND dwOwnerID=@p0
            AND bStorageType=0 AND wItemID=@p1", (int)idA, (int)ShopSabre);
        Check("saved: the sabre, lasting 3 days", sabre is not null && Convert.ToInt32(sabre[0]) is >= 71 and <= 73,
            sabre is null ? "no row" : $"{sabre[0]} h left");
        var cash = await db.RowAsync("SELECT CAST(dwGold AS bigint) * 1000000 + dwSilver * 1000 + dwCooper FROM TCHARTABLE WHERE dwCharID=@p0", (int)idA);
        Check("saved: A's money (fee and sabre paid)", cash is not null && Convert.ToInt64(cash[0]) == 1_000_000 - 2842 - 10_000,
            cash is null ? "no row" : $"{cash[0]}");
    }

    private static long MoneyOf3(PacketReader r) => (long)r.ReadUInt32() * 1_000_000 + r.ReadUInt32() * 1_000L + r.ReadUInt32();
}
