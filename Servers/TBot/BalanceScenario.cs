using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=balance</c>: the kingdom balance. A logs in and gets the operator's balance line with its character info. Its
/// war-balance window gets no answer: the window only counts levels 130 and up, and this game's level chart stops at 125 (as in the
/// C++). After the world's next minute tick (up to a minute), B logs in: its line counts A and B, both Craxion ("Derion" 100%, stage 3).
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_WARCOUNTRYBALANCE_REQ = M + 0x024C, CS_WARCOUNTRYBALANCE_ACK = M + 0x024D;

    private static string? BalanceLine(Bot bot)
    {
        byte[][] seen;
        lock (bot.Received) seen = bot.Received.ToArray();
        foreach (var p in seen.Where(p => PacketHeader.ReadId(p) == CS_CHAT_ACK))
        {
            var r = new PacketReader(p);
            if (r.ReadByte() != 3) continue;
            r.ReadUInt32(); r.ReadString();
            string text = r.ReadString();
            if (text.Contains("Kingdom Balancing")) return text;
        }
        return null;
    }

    private static Task<int> RunBalanceAsync(BotConfig cfg)
    {
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        a.Enter();
        Thread.Sleep(1000);
        string? line = BalanceLine(a);
        Check("balance: the character info comes with the operator's balance line (CS_CHAT_ACK)", line is not null
            && line.Contains("Valorian:") && line.Contains("Derion:") && line.Contains("Current Stage:"), line ?? "no line");

        a.Send(Req(CS_WARCOUNTRYBALANCE_REQ, _ => { }));
        var win = a.TryWait(CS_WARCOUNTRYBALANCE_ACK, timeoutMs: 1500);
        Check("balance: the war-balance window gets no answer below level 130 (the world's gaps start there)", win is null, Describe(win));

        int wait = 62 - DateTime.UtcNow.Second;
        Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] waiting {wait} s for the world's minute tick ...");
        Thread.Sleep(wait * 1000);
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        b.Enter();
        Thread.Sleep(1000);
        string? lineB = BalanceLine(b);
        Check("balance: after the minute tick, the next login counts the online Craxions (Derion 100%, stage 3)", lineB is not null
            && lineB.Contains("Valorian: 0.0%") && lineB.Contains("Derion: 100.0%") && lineB.EndsWith("Current Stage: 3"), lineB ?? "no line");
        return Task.FromResult(PrintResults());
    }
}
