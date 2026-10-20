using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=gm</c>: the GM tools. A's character is made an operator (<c>TOPERATORTABLE</c>, the map restarted to load it).
/// B is refused a chat ban (not an operator); A bans B's chat (B's chat is held back with the time left) and lifts it (B chats
/// again); A gives B the letter-only gift 3 (B gets the letter, A the result); A moves next to B and to a spot; the admin tool's
/// line (<c>CT_CHARMSG_ACK</c>) and event help message (<c>CT_HELPMESSAGE_REQ</c>, through the world) reach B. After: the operator
/// row, the help message, B's letters and the gift log are put back, and the map restarted.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_CHATBAN_REQ = M + 0x0223, CS_CHATBAN_ACK = M + 0x0224, CS_HELPMESSAGE_ACK = M + 0x0244, CS_CMMOVE_REQ = M + 0x0253,
        CS_CMMOVETOUSER_REQ = M + 0x0255, CS_CMGIFT_REQ = M + 0x0258, CS_CMGIFT_ACK = M + 0x0259;
    private const ushort LetterGift = 3;
    private const byte SmChatBanMsg = 10, HelpEventId = 3;

    private static async Task<int> RunGmAsync(BotConfig cfg)
    {
        var db = new GameDb(cfg.GameConnectionString);
        uint idA = await FirstChar(db, cfg.Account), idB = await FirstChar(db, cfg.Account2);
        int lastPost = Convert.ToInt32((await db.RowAsync("SELECT ISNULL(MAX(dwPostID), 0) FROM TPOSTTABLE"))![0]);
        var help = await db.RowAsync("SELECT dStart, dEnd, szMessage FROM THELPMESSAGETABLE WHERE bID=@p0", (int)HelpEventId);
        var start = DateTime.Now.AddMinutes(-1);
        try
        {
            await db.ExecAsync("IF NOT EXISTS (SELECT 1 FROM TOPERATORTABLE WHERE dwOperatorID=@p0) INSERT INTO TOPERATORTABLE (dwOperatorID) VALUES (@p0)", (int)idA);
            await RestartMap("the operator into");
            Gm(cfg, db);
        }
        finally
        {
            await Task.Delay(2500);
            await db.ExecAsync("DELETE FROM TOPERATORTABLE WHERE dwOperatorID=@p0", (int)idA);
            if (help is not null)
                await db.ExecAsync("UPDATE THELPMESSAGETABLE SET dStart=@p1, dEnd=@p2, szMessage=@p3 WHERE bID=@p0", (int)HelpEventId, help[0], help[1], help[2]);
            await db.ExecAsync("DELETE FROM TPOSTTABLE WHERE dwCharID=@p0 AND dwPostID > @p1", (int)idB, lastPost);
            await db.ExecAsync("DELETE FROM TCMGIFTTABLE WHERE dwCharID=@p0 AND wGiftID=@p1 AND tTakeDate >= @p2", (int)idB, (int)LetterGift, start);
            Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] operator row, help message, letters and gift log put back");
            await RestartMap("the operator out of");
        }
        return PrintResults();
    }

    private static void Gm(BotConfig cfg, GameDb db)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        a.Enter(); b.Enter();
        Thread.Sleep(1500);
        string nameA = a.Spawn.Name, nameB = b.Spawn.Name;
        PacketWriter Ban(string who, ushort minutes) => Req(CS_CHATBAN_REQ, w => { w.WriteString(who); w.WriteUInt16(minutes); });
        PacketWriter Say(string talk) => Req(CS_CHAT_REQ, w => { w.WriteString(nameB); w.WriteByte(1 /* CHAT_NEAR */); w.WriteUInt32(0); w.WriteString(""); w.WriteString(talk); });

        b.Send(Ban(nameA, 1));
        var refused = b.TryWait(CS_CHATBAN_ACK);
        Check("gm: a player who is not an operator is refused a chat ban (CHATBAN_NOTOPERATOR)", refused is not null && refused.ReadByte() == 1, Describe(refused));

        a.Send(Ban(nameB, 1));
        var banned = a.TryWait(CS_CHATBAN_ACK);
        Thread.Sleep(300);
        b.Send(Say("hello?"));
        var held = b.TryWait(CS_SYSTEMMSG_ACK);
        Check("gm: the operator bans B's chat, and B's chat is held back with the time left (SM_CHAT_BAN)", banned is not null && banned.ReadByte() == 0
            && held is not null && held.ReadByte() == SmChatBanMsg && held.ReadUInt32() is > 0 and <= 60, Describe(held));

        a.Send(Ban(nameB, 0));
        a.TryWait(CS_CHATBAN_ACK);
        Thread.Sleep(300);
        b.Send(Say("free again"));
        var echo = b.TryWait(CS_CHAT_ACK);
        Check("gm: lifted (0 minutes), B chats again", echo is not null, Describe(echo));

        a.Send(Req(CS_CMGIFT_REQ, w => { w.WriteString(nameB); w.WriteUInt16(LetterGift); }));
        var gift = a.TryWait(CS_CMGIFT_ACK);
        var letter = b.TryWait(CS_POSTRECV_ACK);
        Check("gm: the operator's gift reaches B as a letter, and the operator gets the result", gift is not null && gift.ReadByte() == 0 && letter is not null,
            $"{Describe(gift)}, letter {letter is not null}");

        a.Send(Req(CS_CMMOVETOUSER_REQ, w => w.WriteString(nameB)));
        var near = a.TryWait(CS_TELEPORT_ACK) ?? a.TryWait(CS_BEGINTELEPORT_ACK, timeoutMs: 1000);
        Check("gm: the operator is moved next to B (through the world)", near is not null, Describe(near));

        a.Send(Req(CS_CMMOVE_REQ, w => { w.WriteByte(1); w.WriteUInt16(b.Spawn.MapId); w.WriteFloat(b.Spawn.X + 10); w.WriteFloat(b.Spawn.Y); w.WriteFloat(b.Spawn.Z + 10); }));
        var moved = a.TryWait(CS_TELEPORT_ACK) ?? a.TryWait(CS_BEGINTELEPORT_ACK, timeoutMs: 1000);
        Check("gm: the operator moves to a spot (CS_CMMOVE_REQ)", moved is not null, Describe(moved));

        WorldTool.CharMsg(nameB, "Please behave");
        var line = b.TryWait(CS_CHAT_ACK, p => p.ReadByte() == 7 /* CHAT_OPERATOR */);
        Check("gm: the admin tool's line reaches B from the operator (CHAT_OPERATOR)", line is not null, Describe(line));

        long now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        WorldTool.HelpMessage(HelpEventId, now - 60, now + 3600, "TBot event notice");
        var notice = b.TryWait(CS_HELPMESSAGE_ACK, p => p.ReadByte() == HelpEventId && p.ReadString() == "TBot event notice");
        Check("gm: the admin tool's event help message is shown at once", notice is not null, Describe(notice));
    }
}
