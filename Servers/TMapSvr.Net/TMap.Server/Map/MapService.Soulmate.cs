using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// Social, batch S1c — soulmates, the map's side (C++ CSHandler.cpp:13034–13320, SSHandler.cpp:14058, 14230–14420;
/// TPlayer.cpp:4648–4760). The bonds live in the world; the map checks and charges:
/// <list type="bullet">
/// <item><b>Searching</b> (<c>CS_SOULMATESEARCH_REQ</c>) costs the level's search fee and finds, through the world, someone of the
/// country within 10 levels (the lowest level first; the map passes the lowest level around it within 3 levels, 200 if none).
/// The fee is paid whatever the world finds.</item>
/// <item><b>Choosing</b> someone by name: first a look (<c>CS_SOULMATEREGREADY_REQ</c>), then the bond
/// (<c>CS_SOULMATEREG_REQ</c>), which costs the level's registration fee.</item>
/// <item><b>Ending</b> it (<c>CS_SOULMATEEND_REQ</c>): no new soulmate for a day.</item>
/// <item>Away from the fortune-teller, an NPC-call item (<c>IK_NPCCALL</c>) does it, on maps 0 and 8 only; one is used.</item>
/// <item><b>Effects:</b> a party member whose soulmate gains exp in the same kill gets a tenth more; a hurt player under 15% HP
/// takes HP from a party member, nearby, who chose it as soulmate (down to 65% of theirs).</item>
/// </list>
/// <para>The level reached is now told to the world (<c>MW_LEVELUP_ACK</c>, C++ GainExp), so the world ends a bond 10 levels apart.
/// <b>Not ported:</b> the security-code lock on choosing (as for the guild).</para>
/// </summary>
public sealed partial class MapService
{
    private const byte SoulmateSuccess = 0, SoulmateSilence = 2, SoulmateNeedMoney = 4, SoulmateAlready = 5,
        SoulmateNpcCallError = 6, SoulmateInvalidPos = 7;                                   // SOULMATE_RESULT
    private const uint SoulmateSilenceDuration = 86400;                                     // SOULMATE_SILENCE_DURATION

    private (uint Search, uint Reg) SoulmateCost(Character ch) => _templates.SoulmateCost.GetValueOrDefault(ch.Level);

    /// <summary>C++ <c>CheckSoulSilence</c>: a day after the last bond ended.</summary>
    private static bool CheckSoulSilence(Character ch, uint now) => ch.SoulSilence == 0 || now - ch.SoulSilence >= SoulmateSilenceDuration;

    private static void SetSoulmate(Character ch, uint id, string name, uint silence)
        => (ch.SoulmateId, ch.SoulmateName, ch.SoulSilence) = (id, name, silence);

    /// <summary>The NPC-call item check (C++ repeated in each handler): <paramref name="consume"/> uses one.</summary>
    private byte CheckSoulNpcCall(ClientSession s, Character ch, byte npcInven, byte npcItem, bool consume, bool countMatters,
        byte badPos = SoulmateInvalidPos)
    {
        if (npcInven == Proto.InvenNull || npcItem == Proto.InvalidSlot) return SoulmateSuccess;
        if (ch.MapId != 0 && ch.MapId != 8) return badPos;
        var inv = ch.FindInven(npcInven);
        var item = inv?.FindItem(npcItem);
        if (item is null || item.Template is not { Kind: IkNpcCall } || (countMatters && item.Count == 0)) return SoulmateNpcCallError;
        if (!consume) return SoulmateSuccess;
        item.Count -= 1;                                                                    // C++ UseItem(…, 1)
        if (item.Count == 0) { inv!.Items.Remove(item); SendCS_DELITEM_ACK(s, npcInven, item); }
        else SendCS_UPDATEITEM_ACK(s, npcInven, item);
        return SoulmateSuccess;
    }

    private static void SendCS_SOULMATESEARCH_ACK(ClientSession s, Character ch, byte result, uint id = 0, string name = "", uint region = 0)
    {
        var w = new PacketWriter(Msg.CS_SOULMATESEARCH_ACK);
        w.WriteByte(result); w.WriteUInt32(id); w.WriteString(name); w.WriteUInt32(region);
        w.WriteUInt32(ch.Gold); w.WriteUInt32(ch.Silver); w.WriteUInt32(ch.Cooper);
        s.Send(w);
    }

    private static void SendCS_SOULMATEREG_ACK(ClientSession s, Character ch, byte result, uint id = 0, string name = "", uint region = 0)
    {
        var w = new PacketWriter(Msg.CS_SOULMATEREG_ACK);
        w.WriteByte(result); w.WriteUInt32(id); w.WriteString(name); w.WriteUInt32(region);
        w.WriteUInt32(ch.Gold); w.WriteUInt32(ch.Silver); w.WriteUInt32(ch.Cooper);
        s.Send(w);
    }

    private static void SendCS_SOULMATEREGREADY_ACK(ClientSession s, byte result, string name, byte npcInven, byte npcItem)
    {
        var w = new PacketWriter(Msg.CS_SOULMATEREGREADY_ACK);
        w.WriteByte(result); w.WriteString(name); w.WriteByte(npcInven); w.WriteByte(npcItem);
        s.Send(w);
    }

    // ================================ the client's requests ================================

    /// <summary>C++ <c>OnCS_SOULMATESEARCH_REQ</c> (CSHandler.cpp:13034).</summary>
    private void OnCS_SOULMATESEARCH_REQ(ClientSession s, PacketReader r)
    {
        byte npcInven = r.ReadByte(), npcItem = r.ReadByte();
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        if (!CheckSoulSilence(ch, (uint)UnixNow())) { SendCS_SOULMATESEARCH_ACK(s, ch, SoulmateSilence); return; }
        if (!ch.UseMoney(SoulmateCost(ch).Search, commit: false)) { SendCS_SOULMATESEARCH_ACK(s, ch, SoulmateNeedMoney); return; }
        if (CheckSoulNpcCall(s, ch, npcInven, npcItem, consume: false, countMatters: true) is var npc and not SoulmateSuccess)
        {
            SendCS_SOULMATESEARCH_ACK(s, ch, npc);
            return;
        }
        byte level = 200;
        foreach (var p in _state.Neighbors(s))
            if (p.Char is { } o && o.CharId != ch.CharId && Math.Abs(ch.Level - o.Level) < 3 && level > o.Level) level = o.Level;
        SendToWorld(Msg.MW_SOULMATESEARCH_ACK, s, w => { w.WriteByte(level); w.WriteByte(npcInven); w.WriteByte(npcItem); });
    }

    /// <summary>C++ <c>OnCS_SOULMATEREGREADY_REQ</c> (CSHandler.cpp:13125): a look at someone before choosing it.</summary>
    private void OnCS_SOULMATEREGREADY_REQ(ClientSession s, PacketReader r)
    {
        string name = r.ReadString();
        byte npcInven = r.ReadByte(), npcItem = r.ReadByte();
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        if (!CheckSoulSilence(ch, (uint)UnixNow())) { SendCS_SOULMATEREGREADY_ACK(s, SoulmateSilence, "", npcInven, npcItem); return; }
        if (ch.Name == name) return;
        if (ch.SoulmateName == name) { SendCS_SOULMATEREGREADY_ACK(s, SoulmateAlready, "", npcInven, npcItem); return; }
        if (CheckSoulNpcCall(s, ch, npcInven, npcItem, consume: false, countMatters: true) is var npc and not SoulmateSuccess)
        {
            SendCS_SOULMATEREGREADY_ACK(s, npc, "", npcInven, npcItem);
            return;
        }
        SendToWorld(Msg.MW_SOULMATEREG_ACK, s, w => { w.WriteString(name); w.WriteByte(0); w.WriteByte(npcInven); w.WriteByte(npcItem); });
    }

    /// <summary>C++ <c>OnCS_SOULMATEREG_REQ</c> (CSHandler.cpp:13198). A wrong map answers <c>SOULMATE_NPCCALLERROR</c> here, as in
    /// the C++.</summary>
    private void OnCS_SOULMATEREG_REQ(ClientSession s, PacketReader r)
    {
        string name = r.ReadString();
        byte npcInven = r.ReadByte(), npcItem = r.ReadByte();
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        if (!CheckSoulSilence(ch, (uint)UnixNow())) { SendCS_SOULMATEREG_ACK(s, ch, SoulmateSilence); return; }
        if (ch.Name == name) return;
        if (ch.SoulmateName == name) { SendCS_SOULMATEREG_ACK(s, ch, SoulmateAlready); return; }
        if (!ch.UseMoney(SoulmateCost(ch).Reg, commit: false)) { SendCS_SOULMATEREG_ACK(s, ch, SoulmateNeedMoney); return; }
        if (CheckSoulNpcCall(s, ch, npcInven, npcItem, consume: false, countMatters: true, badPos: SoulmateNpcCallError) is var npc and not SoulmateSuccess)
        {
            SendCS_SOULMATEREG_ACK(s, ch, npc);
            return;
        }
        SendToWorld(Msg.MW_SOULMATEREG_ACK, s, w => { w.WriteString(name); w.WriteByte(1); w.WriteByte(npcInven); w.WriteByte(npcItem); });
    }

    /// <summary>C++ <c>OnCS_SOULMATEEND_REQ</c> (CSHandler.cpp:13301).</summary>
    private void OnCS_SOULMATEEND_REQ(ClientSession s)
    {
        if (s.IsMain && s.Char is { SoulmateId: not 0 } ch && !IsTutorial(ch)) SendToWorld(Msg.MW_SOULMATEEND_ACK, s);
    }

    // ================================ the world's answers ================================

    /// <summary>C++ <c>OnMW_SOULMATE_REQ</c> (SSHandler.cpp:14058): the soulmate on entering (or the end of the last bond).</summary>
    private void OnMW_SOULMATE_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        uint id = r.ReadUInt32();
        string name = r.ReadString();
        uint time = r.ReadUInt32();
        if (FindPlayer(charId, key) is not { Char: { } ch } s) return;
        SetSoulmate(ch, id, name, time);
        if (id != 0) s.Send(new PacketWriter(Msg.CS_SOULMATE_ACK).WriteUInt32(id).WriteString(name));
    }

    /// <summary>C++ <c>OnMW_SOULMATESEARCH_REQ</c> (SSHandler.cpp:14230): the fee is paid (and the NPC-call item used) whatever the
    /// world found.</summary>
    private void OnMW_SOULMATESEARCH_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte result = r.ReadByte();
        uint id = r.ReadUInt32();
        string name = r.ReadString();
        uint region = r.ReadUInt32();
        byte npcInven = r.ReadByte(), npcItem = r.ReadByte();
        if (FindPlayer(charId, key) is not { Char: { } ch } s) return;
        uint cost = SoulmateCost(ch).Search;
        if (!ch.UseMoney(cost, commit: false)) { SendCS_SOULMATESEARCH_ACK(s, ch, SoulmateNeedMoney); return; }
        if (CheckSoulNpcCall(s, ch, npcInven, npcItem, consume: true, countMatters: false) is var npc and not SoulmateSuccess)
        {
            SendCS_SOULMATESEARCH_ACK(s, ch, npc);
            return;
        }
        ch.UseMoney(cost, commit: true);
        SendCS_MONEY_ACK(s, ch);
        if (result == SoulmateSuccess) SetSoulmate(ch, id, name, 0);
        SendCS_SOULMATESEARCH_ACK(s, ch, result, id, name, region);
    }

    /// <summary>C++ <c>OnMW_SOULMATEREG_REQ</c> (SSHandler.cpp:14307): a bond made is paid for; a look only answers.</summary>
    private void OnMW_SOULMATEREG_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte result = r.ReadByte(), reg = r.ReadByte();
        byte npcInven = r.ReadByte(), npcItem = r.ReadByte();
        uint id = r.ReadUInt32();
        string name = r.ReadString();
        uint region = r.ReadUInt32();
        if (FindPlayer(charId, key) is not { Char: { } ch } s) return;
        if (result == SoulmateSuccess && reg != 0)
        {
            if (CheckSoulNpcCall(s, ch, npcInven, npcItem, consume: true, countMatters: false) is var npc and not SoulmateSuccess)
            {
                SendCS_SOULMATEREG_ACK(s, ch, npc);
                return;
            }
            ch.UseMoney(SoulmateCost(ch).Reg, commit: true);
            SetSoulmate(ch, id, name, 0);
            SendCS_MONEY_ACK(s, ch);
        }
        if (reg != 0) SendCS_SOULMATEREG_ACK(s, ch, result, id, name, region);
        else SendCS_SOULMATEREGREADY_ACK(s, result, name, npcInven, npcItem);
    }

    /// <summary>C++ <c>OnMW_SOULMATEEND_REQ</c> (SSHandler.cpp:14380).</summary>
    private void OnMW_SOULMATEEND_REQ(PacketReader r)
    {
        uint charId = r.ReadUInt32(), key = r.ReadUInt32();
        byte result = r.ReadByte();
        uint time = r.ReadUInt32();
        if (FindPlayer(charId, key) is not { Char: { } ch } s) return;
        if (result == SoulmateSuccess) SetSoulmate(ch, 0, "", time);
        s.Send(new PacketWriter(Msg.CS_SOULMATEEND_ACK).WriteByte(result));
    }

    // ================================ the effects ================================

    /// <summary>C++ <c>CTPlayer::CheckSoulEXP</c> (TPlayer.cpp:4667): of a party's kill, <paramref name="ch"/>'s soulmate among the
    /// members sharing it, alive, gets a tenth of <paramref name="exp"/> more.</summary>
    private void CheckSoulExp(IReadOnlyList<ClientSession> party, Character ch, uint exp)
    {
        if (ch.SoulmateId == 0 || ch.SoulSilence != 0) return;
        foreach (var p in party)
            if (p.IsMain && p.Char is { } m && m.CharId == ch.SoulmateId && m.Hp != 0)
            {
                GainExp(p, m, (uint)(exp * 0.1));
                return;
            }
    }

    /// <summary>C++ <c>CTPlayer::CheckSoulHP</c> (TPlayer.cpp:4686), after a hit: a player under 15% of its HP (not in a duel)
    /// takes HP from the party members around who chose it as soulmate (<see cref="TransSoulHp"/>), until it is over 15%.</summary>
    private void CheckSoulHp(ClientSession s, Character ch)
    {
        if (ch.DuelId != 0 || !CheckSoulTakeHp(ch)) return;
        foreach (var p in _state.InView(s).Reverse().ToList())
        {
            if (p.IsMain && p.Char is { } giver && giver.SoulmateId == ch.CharId && giver.SoulSilence == 0
                && giver.GetPartyId() != 0 && giver.GetPartyId() == ch.GetPartyId())
                TransSoulHp(p, giver, ch);
            if (!CheckSoulTakeHp(ch)) break;
        }
    }

    private bool CheckSoulTakeHp(Character ch) => ch.Hp != 0 && (uint)(MaxHpFor(ch) * 0.15) > ch.Hp;

    /// <summary>C++ <c>CTPlayer::TransSoulHP</c> (TPlayer.cpp:4740): the giver keeps 65% of its HP and passes the rest; its own HP is
    /// shown around (the C++ shows only the giver's).</summary>
    private void TransSoulHp(ClientSession gs, Character giver, Character to)
    {
        if (!gs.IsMain || giver.Hp == 0) return;
        uint keep = (uint)(MaxHpFor(giver) * 0.65);
        if (giver.Hp <= keep) return;
        uint trans = giver.Hp - keep;
        giver.Hp -= trans;
        to.Hp = (uint)Math.Min((long)MaxHpFor(to), (long)to.Hp + trans);
        BroadcastHpMp(gs, giver);
    }

    /// <summary>C++ <c>SendMW_LEVELUP_ACK</c> (TPlayer.cpp:1321): the world learns the level reached.</summary>
    private void SendMW_LEVELUP_ACK(ClientSession s, Character ch) => SendToWorld(Msg.MW_LEVELUP_ACK, s, w => w.WriteByte(ch.Level));
}
