using System.Globalization;
using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// The kingdom balance, batch S6b (C++ CSHandler.cpp:17292, SSHandler.cpp:18818 and 19234, CSSender.cpp:373 and 5999).
/// <list type="bullet">
/// <item>The war-balance window asks the world how many Defugel and Craxion players share the character's level gap
/// (<c>CS_WARCOUNTRYBALANCE_REQ</c> → <c>MW_WARCOUNTRYBALANCE_ACK</c> → <c>MW_WARCOUNTRYBALANCE_REQ</c>); with 10 players or fewer
/// in all it shows 5 and 5.</item>
/// <item>Once a minute (and on a country change) the world sends the online counts (<c>MW_RECALCCOUNTRYBALANCE_ACK</c>); the map works
/// out the weaker side (the bonus country) and the stage — 1, 2 or 3 past a 5, 10 or 15 point gap.</item>
/// <item>Every character info ends with the operator's world chat line on the balance (C++ <c>SendCS_CHARINFO_ACK</c>).</item>
/// </list>
/// <para>The stage's PvP bonus and malus are commented out in the C++ 5.0; they are not ported.</para>
/// </summary>
public sealed partial class MapService
{
    private const uint WarBalanceBaseCount = 10;                                                 // WARBALANCE_BASECOUNT

    private ushort _balanceV, _balanceD;                                                         // C++ wVCount / wDCount
    private readonly Dictionary<(byte Country, byte Gap), uint> _warCountryBalance = new();      // C++ m_dwWarCountryBalance

    /// <summary>C++ <c>m_bBonusCountry</c>: the weaker side (0 Defugel, 1 Craxion).</summary>
    public byte BonusCountry { get; private set; }

    /// <summary>C++ <c>m_bStage</c>: 0 (no penalty) to 3.</summary>
    public byte BalanceStage { get; private set; }

    /// <summary>C++ <c>OnCS_WARCOUNTRYBALANCE_REQ</c> (CSHandler.cpp:17292).</summary>
    private void OnCS_WARCOUNTRYBALANCE_REQ(ClientSession s)
    {
        if (!s.IsMain || s.Char is null) return;
        SendToWorld(Msg.MW_WARCOUNTRYBALANCE_ACK, s);
    }

    /// <summary>C++ <c>OnMW_WARCOUNTRYBALANCE_REQ</c> (SSHandler.cpp:18818) + <c>SendCS_WARCOUNTRYBALANCE_ACK</c>.</summary>
    private void OnMW_WARCOUNTRYBALANCE_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32(), countD = r.ReadUInt32(), countC = r.ReadUInt32();
        byte gap = r.ReadByte();
        if (FindPlayer(charId, key) is not { } s) return;
        _warCountryBalance[(TcontryD, gap)] = countD;
        _warCountryBalance[(TcontryC, gap)] = countC;
        if (countD + countC <= WarBalanceBaseCount) countD = countC = WarBalanceBaseCount / 2;
        s.Send(new PacketWriter(Msg.CS_WARCOUNTRYBALANCE_ACK).WriteUInt32(countD).WriteUInt32(countC));
    }

    /// <summary>C++ <c>OnMW_RECALCCOUNTRYBALANCE_ACK</c> (SSHandler.cpp:19234): the bonus country and the stage.</summary>
    private void OnMW_RECALCCOUNTRYBALANCE_ACK(PacketReader r)
    {
        _balanceV = r.ReadByte();
        _balanceD = r.ReadByte();
        int total = _balanceV + _balanceD;
        if (total == 0) { BonusCountry = 1; BalanceStage = 0; return; }                          // the C++ divided by zero: no stage
        float fD = 100f * _balanceV / total, fC = 100f * _balanceD / total;
        BonusCountry = fD < fC ? (byte)0 : (byte)1;
        float diff = Math.Abs(fD - fC);
        BalanceStage = diff > 15f ? (byte)3 : diff > 10f ? (byte)2 : diff > 5f ? (byte)1 : (byte)0;
    }

    /// <summary>C++ <c>SendCS_CHARINFO_ACK</c>'s tail (CSSender.cpp:373): the operator's world chat line on the balance.</summary>
    private void SendBalanceChat(ClientSession s)
    {
        int total = _balanceV + _balanceD;
        float fD = _balanceV != 0 ? 100f * _balanceV / total : 0f, fC = _balanceD != 0 ? 100f * _balanceD / total : 0f;
        string stage = BalanceStage != 0 ? BalanceStage.ToString(CultureInfo.InvariantCulture) : "No Penalty";
        string msg = string.Format(CultureInfo.InvariantCulture, "Kingdom Balancing in Iberia\n Valorian: {0:F1}%\n Derion: {1:F1}%\n Current Stage: {2}",
            fD, fC, stage);
        s.Send(BuildCS_CHAT_ACK(ChatGroup.World, 0, SvrMsg(MsgNameOperator), NetString(msg)));
    }
}
