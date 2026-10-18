using TMap.Data;
using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// Heroes and the warlord's message, batch S6c (C++ CSHandler.cpp:14783–14873, 16459; SSHandler.cpp:15414–15475, 17544–17600;
/// CSSender.cpp:5004–5040).
/// <list type="bullet">
/// <item>The hero list (<c>CS_HEROLIST_REQ</c>): every fort's and castle's hero.</item>
/// <item>A guild chief names the hero of a territory its guild holds (<c>CS_HEROSELECT_REQ</c>) — someone else, not already a hero
/// anywhere; the database checks the name, the country (a castle's hero must be of the guild) and the timing (<c>THeroSelect</c>),
/// then the world tells every map (<c>MW_HEROSELECT_ACK</c> → <c>MW_HEROSELECT_REQ</c>, MapService.Territory.cs).</item>
/// <item>Last month's leader of a fame list writes its message (<c>CS_WARLORDSAY_REQ</c>, <c>TSaveWarlordSay</c>), which the world
/// sends to every map (<c>MW_WARLORDSAY_ACK</c> → <c>MW_WARLORDSAY_REQ</c>, MapService.Rank.cs).</item>
/// </list>
/// <para>The security-code lock the C++ checks first is not ported (no secure codes): a player is treated as having none.</para>
/// </summary>
public sealed partial class MapService
{
    private const byte HsrSuccess = 0, HsrInternal = 1, HsrInvalidChar = 2, HsrInvalidZone = 3, HsrSameName = 4, HsrNoAuthority = 5;   // HERO_SELECT_RESULT

    public IHeroStore? HeroStore { get; set; }

    /// <summary>C++ <c>OnCS_HEROLIST_REQ</c> + <c>SendCS_HEROLIST_ACK</c>.</summary>
    private void OnCS_HEROLIST_REQ(ClientSession s)
    {
        if (!s.IsMain || s.Char is null) return;
        var w = new PacketWriter(Msg.CS_HEROLIST_ACK);
        w.WriteByte((byte)_occupations.Count);
        foreach (var t in _occupations) { w.WriteUInt16(t.Id); w.WriteString(t.Hero); }
        w.WriteByte((byte)_castles.Count);
        foreach (var t in _castles) { w.WriteUInt16(t.Id); w.WriteString(t.Hero); }
        s.Send(w);
    }

    /// <summary>C++ <c>OnCS_HEROSELECT_REQ</c> (CSHandler.cpp:14783) → <c>THeroSelect</c> → <c>OnDM_HEROSELECT_ACK</c>.</summary>
    private async Task OnCS_HEROSELECT_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        ushort zone = r.ReadUInt16();
        string hero = r.ReadString();
        void Ack(byte result) => s.Send(new PacketWriter(Msg.CS_HEROSELECT_ACK).WriteByte(result));
        if (hero.Length == 0 || hero.Length > MaxName || hero == ch.Name) { Ack(HsrInvalidChar); return; }
        if (ch.GuildDuty != GuildDutyChief) { Ack(HsrNoAuthority); return; }
        bool found = false;
        foreach (var t in _territories.Values)
        {
            if (t.Hero == hero) { Ack(HsrSameName); return; }
            if (t.Id != zone) continue;
            found = true;
            if (t.Guild != ch.GuildId) { Ack(HsrNoAuthority); return; }
        }
        if (!found) { Ack(HsrInvalidZone); return; }

        long now = UnixNow();
        int ret = HeroStore is { } db ? await Safe(() => db.HeroSelectAsync(ch.CharId, ch.GuildId, zone, hero, now), HsrInternal) : HsrInternal;
        Ack((byte)ret);
        if (ret != HsrSuccess) return;
        var w = new PacketWriter(Msg.MW_HEROSELECT_ACK);
        w.WriteUInt16(zone); w.WriteString(hero); w.WriteInt64(now);
        _world.Send(w);
    }

    /// <summary>C++ <c>OnCS_WARLORDSAY_REQ</c> (CSHandler.cpp:16459) → <c>TSaveWarlordSay</c> → <c>OnDM_WARLORDSAY_ACK</c>: only last
    /// month's leader of that list.</summary>
    private async Task OnCS_WARLORDSAY_REQ(ClientSession s, PacketReader r)
    {
        if (!s.IsMain || s.Char is not { } ch || IsTutorial(ch)) return;
        byte type = r.ReadByte();
        string say = r.ReadString();
        if (say.Length > MaxBoardTitle) say = say[..MaxBoardTitle];
        byte month = LastRankMonth;
        if (type >= FrtCount || _fameRank[type, month, 0].CharId != ch.CharId) return;
        if (HeroStore is not { } db || await Safe(() => db.SaveWarlordSayAsync(type, month, ch.CharId, say), 1) != 0) return;
        var w = new PacketWriter(Msg.MW_WARLORDSAY_ACK);
        w.WriteByte(type); w.WriteByte(month); w.WriteUInt32(ch.CharId); w.WriteString(say);
        _world.Send(w);
    }
}
