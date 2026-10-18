using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=heroes</c>: a fort's hero. The test guild "TBotBlue" (A its chief) is given fort 1 (Craxion, its next defence in
/// three days), and the world and the map are restarted to load it. A reads the hero list (the forts, then the castles), names B the
/// fort's hero — saved in <c>TLOCALTABLE</c> and shown in the list once the world has told the map — and is refused for fort 2, which
/// its guild does not hold. The fort table and the guild are put back, and the world and the map restarted again.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_HEROSELECT_REQ = M + 0x01CD, CS_HEROSELECT_ACK = M + 0x01CE, CS_HEROLIST_REQ = M + 0x01CF, CS_HEROLIST_ACK = M + 0x01D0;
    private const ushort HeroFort = 1, OtherFort = 2;

    private static (List<(ushort Id, string Hero)> Forts, List<(ushort Id, string Hero)> Castles) HeroList(PacketReader p) => Read(p, r =>
    {
        var forts = new List<(ushort, string)>(); var castles = new List<(ushort, string)>();
        for (byte n = r.ReadByte(), i = 0; i < n; i++) forts.Add((r.ReadUInt16(), r.ReadString()));
        for (byte n = r.ReadByte(), i = 0; i < n; i++) castles.Add((r.ReadUInt16(), r.ReadString()));
        return (forts, castles);
    });

    private static async Task<int> RunHeroesAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account), idB = await FirstChar(db, cfg.Account2);
        try
        {
            await db.ExecAsync("IF OBJECT_ID('TBOT_BAK_TLOCALTABLE') IS NULL SELECT * INTO TBOT_BAK_TLOCALTABLE FROM TLOCALTABLE");
            await ClearGuilds(db);
            await db.ExecAsync($@"SET IDENTITY_INSERT TGUILDTABLE ON;
INSERT INTO TGUILDTABLE (dwID, szName, dwChief, bLevel, dwFame, dwFameColor, bMaxCabinet, dwGold, dwSilver, dwCooper, dwGI, dwExp,
    bGPoint, bStatus, bDisorg, dwTime, timeEstablish, dwPvPTotalPoint, dwPvPUseablePoint, dwPvPMonthPoint)
VALUES ({BlueGuild}, 'TBotBlue', @p0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, GETDATE(), 0, 0, 0);
SET IDENTITY_INSERT TGUILDTABLE OFF;
INSERT INTO TGUILDMEMBERTABLE (dwCharID, dwGuildID, bDuty, bPeer, dwService) VALUES (@p0, {BlueGuild}, 2, 0, 0);
UPDATE TLOCALTABLE SET dwGuild = {BlueGuild}, bCountry = 1, szHero = '', dateHero = '2000-01-01', dateDefend = DATEADD(day, 3, GETDATE())
    WHERE wLocalID = {HeroFort}", (int)idA);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] guild and fort fixtures in; restarting the world and the map ...");
            await Docker("compose restart worldsvr");
            await WaitForWorld();
            await RestartMap("the fort's new owner into");
            await Heroes(cfg, db);
        }
        finally
        {
            await Task.Delay(2500);
            await ClearGuilds(db);
            await db.ExecAsync(@"IF OBJECT_ID('TBOT_BAK_TLOCALTABLE') IS NOT NULL
                BEGIN DELETE FROM TLOCALTABLE; INSERT INTO TLOCALTABLE SELECT * FROM TBOT_BAK_TLOCALTABLE; DROP TABLE TBOT_BAK_TLOCALTABLE END");
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] fort table and guild put back; restarting the world and the map ...");
            await Docker("compose restart worldsvr");
            await WaitForWorld();
            await RestartMap("the fort's old owner back into");
        }
        return PrintResults();
    }

    private static async Task Heroes(BotConfig cfg, GameDb db)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        a.Enter(); b.Enter();
        Thread.Sleep(1500);
        string nameB = b.Spawn.Name;

        a.Send(Req(CS_HEROLIST_REQ, _ => { }));
        var list = a.TryWait(CS_HEROLIST_ACK);
        var (forts, castles) = list is null ? (new(), new()) : HeroList(list);
        Check("heroes: the hero list shows the forts, then the castles (CS_HEROLIST_ACK)", forts.Count > 0 && castles.Count > 0
            && forts.Any(f => f.Id == HeroFort && f.Hero == ""), $"{forts.Count} fort(s), {castles.Count} castle(s)");

        a.Send(Req(CS_HEROSELECT_REQ, w => { w.WriteUInt16(OtherFort); w.WriteString(nameB); }));
        var refused = a.TryWait(CS_HEROSELECT_ACK);
        Check("heroes: a fort the guild does not hold is refused (HSR_NOAUTHORITY)", refused is not null && Read(refused, r => r.ReadByte()) == 5, Describe(refused));

        a.Send(Req(CS_HEROSELECT_REQ, w => { w.WriteUInt16(HeroFort); w.WriteString(nameB); }));
        var named = a.TryWait(CS_HEROSELECT_ACK);
        Check("heroes: the chief names B the fort's hero (CS_HEROSELECT_ACK)", named is not null && Read(named, r => r.ReadByte()) == 0, Describe(named));
        Thread.Sleep(500);
        a.Send(Req(CS_HEROLIST_REQ, _ => { }));
        var after = a.TryWait(CS_HEROLIST_ACK);
        Check("heroes: … the world tells the map, and the list shows B", after is not null && HeroList(after).Forts.Any(f => f.Id == HeroFort && f.Hero == nameB),
            Describe(after));
        var row = await db.RowAsync("SELECT szHero FROM TLOCALTABLE WHERE wLocalID=@p0", (int)HeroFort);
        Check("heroes: … saved in TLOCALTABLE", row is not null && Convert.ToString(row[0]) == nameB, row is null ? "no row" : $"{row[0]}");
    }
}
