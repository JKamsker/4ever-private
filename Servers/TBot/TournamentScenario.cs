using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=tournament</c>: the tournament's windows, against the world's real schedule (TTOURNAMENT* charts). A asks for the
/// schedule (the world's steps, relayed by the map), then for the sign-up window — answered as the step allows (the sign-up info, the
/// party list or the match list) — and signing up for an entry that does not exist is refused (TOURNAMENT_FAIL). Nothing is written.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_TOURNAMENTAPPLYINFO_REQ = M + 0x0227, CS_TOURNAMENTAPPLYINFO_ACK = M + 0x0228, CS_TOURNAMENTAPPLY_REQ = M + 0x0229,
        CS_TOURNAMENTAPPLY_ACK = M + 0x022A, CS_TOURNAMENTJOINLIST_ACK = M + 0x022C, CS_TOURNAMENTMATCHLIST_ACK = M + 0x0232,
        CS_TOURNAMENTSCHEDULE_REQ = M + 0x0241, CS_TOURNAMENTSCHEDULE_ACK = M + 0x0242;

    private static Task<int> RunTournamentAsync(BotConfig cfg)
    {
        using (var a = new Bot(Clone(cfg, cfg.Account, "[A]")))
        {
            a.Enter();
            Thread.Sleep(1000);

            a.Send(Req(CS_TOURNAMENTSCHEDULE_REQ, _ => { }));
            var sched = a.TryWait(CS_TOURNAMENTSCHEDULE_ACK);
            byte step = 0;
            Check("tournament: the schedule window shows the world's steps (CS_TOURNAMENTSCHEDULE_ACK)", sched is not null && Read(sched, r =>
            {
                r.ReadByte(); step = r.ReadByte();
                byte n = r.ReadByte();
                for (int i = 0; i < n; i++) { r.ReadByte(); r.ReadByte(); r.ReadInt64(); }
                return n;
            }) > 0, Describe(sched));
            Report("tournament", $"the world is at step {step}");

            a.Send(Req(CS_TOURNAMENTAPPLYINFO_REQ, _ => { }));
            ushort expected = step <= 2 ? CS_TOURNAMENTAPPLYINFO_ACK : step == 3 ? CS_TOURNAMENTJOINLIST_ACK : CS_TOURNAMENTMATCHLIST_ACK;
            var info = a.TryWait(expected, timeoutMs: 3000);
            Check($"tournament: the sign-up window is answered for step {step} (0x{expected:X4})", info is not null, Describe(info));

            a.Send(Req(CS_TOURNAMENTAPPLY_REQ, w => { w.WriteByte(99); w.WriteString("TBOT-HWID"); }));
            var apply = a.TryWait(CS_TOURNAMENTAPPLY_ACK);
            Check("tournament: signing up for an entry that does not exist is refused (TOURNAMENT_FAIL)", apply is not null
                && Read(apply, r => (r.ReadByte(), r.ReadByte())) == (10, 99), Describe(apply));
        }
        return Task.FromResult(PrintResults());
    }
}
