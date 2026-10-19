using TLogin.Protocol;

namespace TBot;

/// <summary>
/// <c>--Bot:Scenario=battlequeue</c>: the BoW and BR queues, relayed by the map to the world's real queues. A asks to join the BoW
/// queue, leaves it, and asks to join the BR queue — each answered with the world's verdict (outside the sign-up time, a refusal).
/// The world answers the BoW queue only when BoW is on (a <c>TBOWSETTINGSCHART</c> row, as in the C++); leaving is always answered
/// (it is also the BR queue's). A invites B into a BR team: B gets the invite with A's name, and B's refusal comes back to A; inviting
/// someone who isn't online is answered "not found". Nothing is written.
/// </summary>
public static partial class Scenarios
{
    private const ushort CS_REGISTERBOW_REQ = M + 0x0369, CS_REGISTERBOW_ACK = M + 0x036A, CS_CANCELBOWQUEUE_REQ = M + 0x036B,
        CS_CANCELBOWQUEUE_ACK = M + 0x036C, CS_REGISTERBR_REQ = M + 0x0379, CS_BRTEAMMATEADD_REQ = M + 0x037B, CS_BRTEAMMATEADD_ACK = M + 0x037C,
        CS_BRTEAMMATEADDRESULT_ACK = M + 0x037D, CS_ADDTOBRQUEUE_ACK = M + 0x0383;
    private const byte TeamAddSuccess = 0, TeamAddNotFound = 1, TeamAddRefuse = 3;

    private static async Task<int> RunBattleQueueAsync(BotConfig cfg)
    {
        bool bowOn = await new GameDb(cfg.GameConnectionString).RowAsync("SELECT TOP 1 wMapID FROM TBOWSETTINGSCHART") is not null;
        Report("battlequeue", bowOn ? "BoW is on" : "BoW is off (no TBOWSETTINGSCHART row)");
        using var a = new Bot(Clone(cfg, cfg.Account, "[A]"));
        using var b = new Bot(Clone(cfg, cfg.Account2, "[B]"));
        a.Enter(); b.Enter();
        Thread.Sleep(1500);
        string nameA = a.Spawn.Name;

        (byte Result, uint Tick)? Answer(PacketReader? p) => p is null ? null : Read(p, r => (r.ReadByte(), r.ReadUInt32()));

        a.Send(Req(CS_REGISTERBOW_REQ, _ => { }));
        var bow = Answer(a.TryWait(CS_REGISTERBOW_ACK));
        Check(bowOn ? "battlequeue: joining the BoW queue is answered by the world (CS_REGISTERBOW_ACK)"
                    : "battlequeue: with BoW off, joining its queue is not answered (as in the C++)", (bow is not null) == bowOn, $"{bow}");

        a.Send(Req(CS_CANCELBOWQUEUE_REQ, _ => { }));
        var cancel = Answer(a.TryWait(CS_CANCELBOWQUEUE_ACK));
        Check("battlequeue: leaving the queue is answered (CS_CANCELBOWQUEUE_ACK)", cancel is not null, $"{cancel}");

        a.Send(Req(CS_REGISTERBR_REQ, w => w.WriteByte(0)));
        var br = Answer(a.TryWait(CS_ADDTOBRQUEUE_ACK));
        Check("battlequeue: joining the BR queue is answered (CS_ADDTOBRQUEUE_ACK)", br is not null, $"{br}");

        a.Send(Req(CS_BRTEAMMATEADD_REQ, w => w.WriteString(nameA + "Nobody")));
        var missing = a.TryWait(CS_BRTEAMMATEADD_ACK);
        Check("battlequeue: inviting someone who isn't online is answered 'not found'", missing is not null
            && Read(missing, r => (r.ReadByte(), r.ReadString())).Item1 == TeamAddNotFound, Describe(missing));

        a.Send(Req(CS_BRTEAMMATEADD_REQ, w => w.WriteString(b.Spawn.Name)));
        var invite = b.TryWait(CS_BRTEAMMATEADD_ACK);
        Check("battlequeue: B gets A's BR team invite", invite is not null
            && Read(invite, r => (r.ReadByte(), r.ReadString())) == (TeamAddSuccess, nameA), Describe(invite));

        b.Send(Req(CS_BRTEAMMATEADDRESULT_ACK, w => { w.WriteByte(TeamAddRefuse); w.WriteString(nameA); }));
        var refused = a.TryWait(CS_BRTEAMMATEADD_ACK);
        Check("battlequeue: B's refusal comes back to A", refused is not null
            && Read(refused, r => (r.ReadByte(), r.ReadString())).Item1 == TeamAddRefuse, Describe(refused));
        return PrintResults();
    }
}
