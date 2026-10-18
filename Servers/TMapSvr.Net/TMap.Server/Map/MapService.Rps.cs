using TMap.Data;
using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// Mini-games, batch S5c — rock-paper-scissors against the NPC (C++ CSHandler.cpp:16960–17096, SSHandler.cpp:18738–18810,
/// TMapSvr.cpp:11671–11742, CSSender.cpp:5977–5996).
/// <list type="bullet">
/// <item>A ticket (the chart's win-0 item) starts a game of a type (<c>CS_RPSSTART_REQ</c>).</item>
/// <item>Each play (<c>CS_RPSGAME_REQ</c>) rolls the current step's odds — win, draw or lose; the NPC's card is picked to match.
/// A loss ends the game with nothing. A step with a win limit (<c>wWinKeep</c>) asks the world whether the win may stand
/// (<c>MW_RPSGAME_ACK</c> → <c>MW_RPSGAME_REQ</c>).</item>
/// <item>Card 3 cashes out, as does winning the last step: the reward of the wins so far comes by mail from the operator.</item>
/// <item>The world pushes odds changes made from the control tool (<c>MW_RPSGAMECHANGE_REQ</c>).</item>
/// </list>
/// </summary>
public sealed partial class MapService
{
    private const byte RpsSuccess = 0, RpsInGame = 1, RpsNoData = 2, RpsNeedItem = 3;          // RPS_RESULT
    private const byte RpsRock = 0, RpsPaper = 1, RpsScissors = 2, RpsCardCount = 3;          // RPSCARD_TYPE
    private const byte RpsNpcWins = 1, RpsNpcLoses = 2;                                       // TNMTWIN_WIN / TNMTWIN_LOSE, the player's side
    private const uint MsgRpsRewardTitle = 45, MsgRpsRewardMessage = 46;

    private static ushort RpsKey(byte type, byte win) => (ushort)(type | win << 8);

    private static void ResetRps(Character ch) { ch.RpsType = 0; ch.RpsWin = 0; }

    /// <summary>C++ <c>OnCS_RPSSTART_REQ</c> (CSHandler.cpp:16960): the ticket is used, the game starts.</summary>
    private void OnCS_RPSSTART_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch) return;
        byte type = r.ReadByte(), inv = r.ReadByte(), slot = r.ReadByte();
        void Ack(byte result) => s.Send(new PacketWriter(Msg.CS_RPSSTART_ACK).WriteByte(result).WriteUInt32(ch.Gold).WriteUInt32(ch.Silver).WriteUInt32(ch.Cooper));
        if (ch.RpsType != 0) { Ack(RpsInGame); return; }
        if (!_templates.RpsGames.TryGetValue(RpsKey(type, 0), out var rps)) { Ack(RpsNoData); return; }
        var bag = ch.FindInven(inv);
        if (bag?.FindItem(slot) is not { Count: > 0 } ticket || ticket.TemplateId != rps.ItemId) { Ack(RpsNeedItem); return; }
        UseItemStack(s, inv, bag, ticket, 1);
        ch.RpsType = type;
        ch.RpsWin = 0;
        Ack(RpsSuccess);
    }

    /// <summary>C++ <c>OnCS_RPSGAME_REQ</c> (CSHandler.cpp:17025): one play, or cashing out.</summary>
    private void OnCS_RPSGAME_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch) return;
        byte card = r.ReadByte();
        if (ch.RpsType == 0) return;
        if (!_templates.RpsGames.TryGetValue(RpsKey(ch.RpsType, ch.RpsWin), out var rps)) { ResetRps(ch); return; }
        if (card >= RpsCardCount) { RpsReward(ch); return; }

        int roll = rps.Prob[2] != 0 ? Rand() % rps.Prob[2] : 0xFF;
        byte npc;
        if (roll < rps.Prob[0])
        {
            if (rps.WinKeep != 0)
            {
                SendToWorld(Msg.MW_RPSGAME_ACK, s, w => { w.WriteByte(ch.RpsType); w.WriteByte((byte)(ch.RpsWin + 1)); w.WriteByte(card); });
                return;
            }
            ch.RpsWin++;
            npc = NpcRps(RpsNpcWins, card);
        }
        else if (roll < rps.Prob[1]) npc = card;
        else
        {
            ResetRps(ch);
            npc = NpcRps(RpsNpcLoses, card);
        }
        SendRpsGame(s, ch, card, npc);
    }

    /// <summary>C++ <c>OnMW_RPSGAME_REQ</c> (SSHandler.cpp:18738): the world's word on a limited win.</summary>
    private void OnMW_RPSGAME_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        bool won = r.ReadByte() != 0;
        byte card = r.ReadByte();
        if (FindPlayer(charId, key) is not { Char: { RpsType: not 0 } ch } s) return;
        byte npc;
        if (won) { ch.RpsWin++; npc = NpcRps(RpsNpcWins, card); }
        else { ResetRps(ch); npc = NpcRps(RpsNpcLoses, card); }
        SendRpsGame(s, ch, card, npc);
    }

    /// <summary>C++ <c>OnMW_RPSGAMECHANGE_REQ</c> (SSHandler.cpp:18777): new odds from the control tool.</summary>
    private void OnMW_RPSGAMECHANGE_REQ(PacketReader r)
    {
        r.ReadByte();                                                                           // bGroup
        ushort count = r.ReadUInt16();
        for (int i = 0; i < count; i++)
        {
            byte type = r.ReadByte(), win = r.ReadByte(), pWin = r.ReadByte(), pDraw = r.ReadByte(), pLose = r.ReadByte();
            ushort keep = r.ReadUInt16(), period = r.ReadUInt16();
            if (!_templates.RpsGames.TryGetValue(RpsKey(type, win), out var rps)) continue;
            rps.Prob[0] = pWin; rps.Prob[1] = pDraw; rps.Prob[2] = pLose;
            rps.WinKeep = keep; rps.WinPeriod = period;
        }
    }

    /// <summary>C++ <c>SendCS_RPSGAME_ACK</c>, then the last step won cashes out on its own.</summary>
    private void SendRpsGame(ClientSession s, Character ch, byte card, byte npc)
    {
        s.Send(new PacketWriter(Msg.CS_RPSGAME_ACK).WriteByte(ch.RpsWin).WriteByte(card).WriteByte(npc));
        if (ch.RpsType != 0 && !_templates.RpsGames.ContainsKey(RpsKey(ch.RpsType, (byte)(ch.RpsWin + 1)))) RpsReward(ch);
    }

    /// <summary>C++ <c>RPSReward</c> (TMapSvr.cpp:11671): the wins so far pay by mail; the game ends.</summary>
    private void RpsReward(Character ch)
    {
        if (ch.RpsWin != 0 && _templates.RpsGames.TryGetValue(RpsKey(ch.RpsType, ch.RpsWin), out var rps))
        {
            var items = new List<(ItemTemplate, byte)>();
            for (int i = 0; i < 2; i++)
                if (rps.RewardItem[i] != 0 && rps.ItemCount[i] != 0 && _templates.Item(rps.RewardItem[i]) is { } tpl) items.Add((tpl, rps.ItemCount[i]));
            var (gold, silver, cooper) = SplitMoney(rps.RewardMoney);
            _ = MailOperator(ch.CharId, ch.Name, SvrMsg(MsgRpsRewardTitle), NetString(SvrMsg(MsgRpsRewardMessage)), items, gold, silver, cooper);
            _log.LogInformation("RPS {Name} type {Type} cashed out at {Win} wins.", ch.Name, ch.RpsType, ch.RpsWin);
        }
        ResetRps(ch);
    }

    /// <summary>C++ <c>SelectNpcRPS</c> (TMapSvr.cpp:11721): the NPC's card for the player's result.</summary>
    internal static byte NpcRps(byte playerResult, byte card) => card switch
    {
        RpsRock => playerResult == RpsNpcWins ? RpsScissors : RpsPaper,
        RpsPaper => playerResult == RpsNpcWins ? RpsRock : RpsScissors,
        RpsScissors => playerResult == RpsNpcWins ? RpsPaper : RpsRock,
        _ => RpsRock,
    };
}
