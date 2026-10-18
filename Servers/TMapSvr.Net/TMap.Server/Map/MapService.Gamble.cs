using TMap.Data;
using TMap.Protocol;

namespace TMap.Server.Map;

/// <summary>
/// Mini-games, batch S5a — the fortune teller's gamble and the magic item shop (C++ CSHandler.cpp:13323–13730, 6484–6600;
/// CSSender.cpp:2901, 4658–4690; TItem.cpp:588, 664).
/// <list type="bullet">
/// <item><b>Gambling</b> a sealed item (<c>CS_GAMBLEOPEN_REQ</c>; <c>CS_GAMBLECHECK_REQ</c> only quotes): a wrapped item is unwrapped
/// and a mogg is taken off for free; otherwise the gamble level's fee is paid (<c>TLEVELCHART</c> <c>dwGambleCost</c> for a gamble
/// item, <c>dwRepCost</c> to unseal one), less the NPC's discount. With the item's chance (and the NPC's bonus) it turns into
/// another (<c>TGAMBLECHART</c>, by weight, a random count), else it gets 1–2 magic options — or, when none comes, it may be
/// destroyed (<c>GAMBLE_FAIL</c>) or stays as it was, unsealed (<c>GAMBLE_REPFAIL</c>). A stack gambles one unit.</item>
/// <item><b>The magic shop</b> (<c>TNPC_MAGICITEM</c>) sells prebuilt magic items (<c>TQUESTITEMCHART</c>), lasting their days.</item>
/// <item>Away from the NPC, an NPC-call item does it, on maps 0 and 8 only.</item>
/// </list>
/// <para><b>Fixed C++ bugs:</b> the gamble rolled its options at level 0 (<c>IMT_GAMBLE</c> passed as the magic buff) — now from the
/// item's gamble level, as meant; the magic shop charged for the count asked but gave one — it gives the count; an item lasting 0
/// days was born expired — it now lasts. <b>Not ported:</b> the security-code lock and the tournament gate on buying.</para>
/// </summary>
public sealed partial class MapService
{
    private const byte GambleSuccess = 0, GambleInvalidItem = 1, GambleNeedMoney = 2, GambleFail = 3, GambleRepFail = 4,
        GambleFullInven = 5, GambleInvalidPos = 6;                                              // GAMBLE_RESULT
    private const byte ItGamble = 13;
    private const byte ItemBuySuccess = 0, ItemBuyNotFound = 1, ItemBuyNeedMoney = 2, ItemBuyCantPush = 3, ItemBuyDealing = 4,
        ItemBuyNpcCallError = 5, ItemBuyInvalidPos = 6;                                         // TITEMBUY_RESULT
    private const byte TnpcMagicItem = 25;
    private const byte DuringTypeTake = 0x80;                                                  // DURINGTYPE_TIME / _DAY are in MapService.Pet.cs

    /// <summary>The fee to gamble an item (C++, both handlers): null when it can't be gambled.</summary>
    private uint? GambleCost(Item item)
    {
        if (item.Template is not { } t || !_templates.GambleCost.TryGetValue(item.GLevel, out var cost)) return null;
        if (t.Type == ItGamble && t.CanGamble == 0) return cost.Gamble;
        if (t.CanGamble != 0 && item.GLevel != 0) return cost.Rep;
        return null;
    }

    /// <summary>C++ <c>OnCS_GAMBLECHECK_REQ</c> (CSHandler.cpp:13323): the price, before discount, and the discount.</summary>
    private void OnCS_GAMBLECHECK_REQ(ClientSession s, PacketReader r)
    {
        byte inven = r.ReadByte(), slot = r.ReadByte();
        ushort npcId = r.ReadUInt16();
        if (!s.IsMain || s.Char is not { } ch || s.Deal.Status >= DealStart) return;
        void Ack(byte result, uint money = 0, byte discount = 0)
            => s.Send(new PacketWriter(Msg.CS_GAMBLECHECK_ACK).WriteByte(result).WriteByte(inven).WriteByte(slot).WriteUInt32(money).WriteByte(discount));
        if (ch.FindInven(inven)?.FindItem(slot) is not { Count: > 0 } item || !_templates.GambleCost.ContainsKey(item.GLevel)) { Ack(GambleInvalidItem); return; }
        byte discount = DiscountRate(ch, _state.FindNpc(npcId));
        uint money = GambleCost(item) ?? (item.Ext[Item.IevWrap] != 0 || item.MoggItemId != 0 ? 0u : 1u);
        if (!ch.UseMoney(money - money * discount / 100, commit: false)) { Ack(GambleNeedMoney); return; }
        Ack(GambleSuccess, money, discount);
    }

    /// <summary>C++ <c>OnCS_GAMBLEOPEN_REQ</c> (CSHandler.cpp:13404).</summary>
    private void OnCS_GAMBLEOPEN_REQ(ClientSession s, PacketReader r)
    {
        byte inven = r.ReadByte(), slot = r.ReadByte();
        ushort npcId = r.ReadUInt16();
        byte npcInven = r.ReadByte(), npcSlot = r.ReadByte();
        if (!s.IsMain || s.Char is not { } ch || s.Deal.Status >= DealStart) return;
        void Ack(byte result, byte a, byte b, Item? item = null)
        {
            var w = new PacketWriter(Msg.CS_GAMBLEOPEN_ACK);
            w.WriteByte(result); w.WriteUInt32(ch.Gold); w.WriteUInt32(ch.Silver); w.WriteUInt32(ch.Cooper); w.WriteByte(a); w.WriteByte(b);
            item?.WrapPacketClient(w, ch.CharId, addItemId: false);
            s.Send(w);
        }
        var bag = ch.FindInven(inven);
        if (bag?.FindItem(slot) is not { Count: > 0 } item) { Ack(GambleInvalidItem, inven, slot); return; }
        Inven? callBag = null;
        Item? call = null;
        if (npcInven != Proto.InvenNull && npcSlot != Proto.InvalidSlot)
        {
            if (ch.MapId != 0 && ch.MapId != 8) { Ack(GambleInvalidPos, npcInven, npcSlot); return; }
            callBag = ch.FindInven(npcInven);
            call = callBag?.FindItem(npcSlot);
            if (call is null || call.Template is not { Kind: IkNpcCall } || call.Count == 0) return;
        }

        byte result = GambleSuccess;
        uint money = 0;
        ushort newItem = 0;
        byte newCount = 0;
        bool changed = false;
        if (item.Ext[Item.IevWrap] != 0) { item.Ext[Item.IevWrap] = 0; EnqueueItemSave(s, inven, item); }
        else if (item.MoggItemId != 0) { item.MoggItemId = 0; EnqueueItemSave(s, inven, item); }
        else
        {
            if (!_templates.GambleCost.ContainsKey(item.GLevel) || GambleCost(item) is not { } cost) { Ack(GambleInvalidItem, inven, slot); return; }
            var npc = _state.FindNpc(npcId);
            money = cost - cost * DiscountRate(ch, npc) / 100;
            if (!ch.UseMoney(money, commit: false)) { Ack(GambleNeedMoney, inven, slot); return; }

            var gambled = item.Clone();
            gambled.Count = 1;
            gambled.DlId = GenItemId();
            byte prob = CalcProb(s, ch, npc, Prob.Gamble, gambled.Template!.GambleProb);
            if (prob > Rand() % 100)
            {
                if (!_templates.Gambles.TryGetValue((gambled.Template.Type, gambled.Template.Kind), out var rows) || rows.Count == 0) result = GambleFail;
                else
                {
                    int total = rows.Sum(g => g.Prob);
                    int pick = total == 0 ? 0 : Rand() % total, sum = 0;
                    var won = rows[^1];
                    foreach (var g in rows) { sum += g.Prob; if (sum >= pick) { won = g; break; } }
                    gambled.Count = (byte)(won.CountMax == 0 ? 1 : Rand() % won.CountMax + 1);
                    if (_templates.Item(won.ReplaceId) is { } to)
                    {
                        gambled.TemplateId = won.ReplaceId;
                        gambled.Template = to;
                        LinkItemAttr(gambled);                                                  // C++ SetItemAttr
                        if (to.Type == ItRefine) Catalyzer(gambled);
                        else MakeSpecialItem(ch, gambled, IkMagicGrade, 0, ImtGamble);
                        SetEndTime(gambled);
                        changed = true;
                    }
                    else result = GambleFail;
                }
            }
            else if (!MakeSpecialItem(ch, gambled, IkMagicGrade, 0, ImtGamble))
                result = gambled.Template.DestroyProb > Rand() % 100 ? GambleFail : GambleRepFail;
            gambled.GLevel = 0;
            newItem = gambled.TemplateId;
            newCount = gambled.Count;

            if (result == GambleFail) UseItemStack(s, inven, bag, item, 1);
            else if (item.Count > 1)
            {
                if (!HaveInvenBlank(ch) || !CanPush(ch, new[] { gambled })) { changed = false; result = GambleFullInven; }
                else { UseItemStack(s, inven, bag, item, 1); PushTItem(s, new[] { gambled }); }
            }
            else
            {
                bag.Items.Remove(item);
                gambled.ItemSlot = item.ItemSlot;
                bag.Items.Add(gambled);
                EnqueueItemDelete(s, item);
                EnqueueItemSave(s, inven, gambled);
                item = gambled;
            }
        }
        if (result != GambleFullInven)
        {
            if (call is not null) UseItemStack(s, npcInven, callBag!, call, 1);
            ch.UseMoney(money, commit: true);
            SendCS_MONEY_ACK(s, ch);
        }
        Ack(result, inven, slot, result == GambleFail ? null : item);
        if (changed && newItem != 0) CheckQuest(s, 0, ch.PosX, ch.PosY, ch.PosZ, newItem, QttGetItem, TtGetItem, newCount);
    }

    /// <summary>C++ <c>CTItem::Catalyzer</c> (TItem.cpp:664): a refine item's options — up to six, each less likely (100, 50, 20,
    /// 10, 5, 1%), among the refine options, at the base level.</summary>
    private void Catalyzer(Item item)
    {
        int[] opProb = { 100, 50, 20, 10, 5, 1 };
        var pool = _templates.Magics.Values.Where(m => m.MagicId != 0 && m.Refine != 0).OrderBy(m => m.MagicId).ToList();
        foreach (int p in opProb)
        {
            if (Rand() % 100 >= p || pool.Count == 0) continue;
            int sel = Rand() % pool.Count;
            var m = pool[sel];
            ushort value = MakeItemMagicValue(ItemMagicBaseLevel, m.RareBound);
            if (value == 0) continue;
            if (item.Magic.All(o => o.Id != m.MagicId)) item.Magic.Add(new MagicOption(m.MagicId, value, m));
            pool.RemoveAt(sel);
        }
    }

    /// <summary>C++ <c>CTItem::SetEndTime</c> (TItem.cpp:588): an item lasting from when it is taken ends that many hours or days on,
    /// at the next midnight.</summary>
    private void SetEndTime(Item item, long useSeconds = 0)
    {
        long add = useSeconds;
        if (add == 0 && item.Template is { } t && (t.UseType & DuringTypeTake) != 0)
            add = (t.UseType & DuringTypeTime) != 0 ? t.UseTime * 3600L : (t.UseType & DuringTypeDay) != 0 ? t.UseTime * 86400L : 0;
        if (add == 0) { item.EndTime = 0; return; }
        var now = DateTimeOffset.FromUnixTimeSeconds(UnixNow());
        add -= now.Second + now.Minute * 60;
        add += (24 - now.Hour) * 3600L;
        item.EndTime = now.ToUnixTimeSeconds() + add;
    }

    // ================================ the magic shop ================================

    /// <summary>C++ <c>SendCS_NPCITEMLIST_ACK</c>'s <c>TNPC_MAGICITEM</c> case (CSSender.cpp:2901): the items for the character's class
    /// and country, each with its price and the item, ending its days (and 59 minutes) from now.</summary>
    private void SendCS_NPCMAGICLIST_ACK(ClientSession s, Character ch, Npc npc)
    {
        var w = new PacketWriter(Msg.CS_NPCITEMLIST_ACK);
        w.WriteUInt16(npc.Id); w.WriteByte(npc.Type); w.WriteByte(DiscountRate(ch, npc));
        var list = npc.MagicItems.Where(kv => _templates.Item(kv.Value.Item.TemplateId) is { } t && (t.ClassId & (1u << ch.Class)) != 0
                && (t.ItemCountry == TcontryN || npc.DiscountCondition == DccAllCountry || t.ItemCountry == ch.Country))
            .OrderBy(kv => kv.Key).ToList();
        w.WriteByte((byte)list.Count);
        foreach (var (id, row) in list)
        {
            w.WriteUInt32(id); w.WriteUInt32(row.Money);
            if (ItemFromRow(row.Item) is not { } item) continue;
            item.EndTime = UnixNow() + row.UseTime * 86400L + 59 * 60;
            item.WrapPacketClient(w, ch.CharId, addItemId: false);
        }
        s.Send(w);
    }

    /// <summary>C++ <c>OnCS_MAGICITEMBUY_REQ</c> (CSHandler.cpp:6484).</summary>
    private void OnCS_MAGICITEMBUY_REQ(ClientSession s, PacketReader r)
    {
        ushort npcId = r.ReadUInt16();
        uint id = r.ReadUInt32();
        byte count = r.ReadByte(), npcInven = r.ReadByte(), npcSlot = r.ReadByte();
        if (!s.IsMain || s.Char is not { } ch) return;
        void Ack(byte result) => s.Send(new PacketWriter(Msg.CS_MAGICITEMBUY_ACK).WriteByte(result).WriteUInt32(ch.Gold).WriteUInt32(ch.Silver).WriteUInt32(ch.Cooper));
        if (s.Deal.Status >= DealStart) { Ack(ItemBuyDealing); return; }
        if (_state.FindNpc(npcId) is not { } npc || count == 0) { Ack(ItemBuyNotFound); return; }
        if (!npc.MagicItems.TryGetValue(id, out var row) || ItemFromRow(row.Item) is not { Template: { } t } item) { Ack(ItemBuyNotFound); return; }
        if (t.Stack < count) count = t.Stack;
        uint price = row.Money * count;
        price -= price * DiscountRate(ch, npc) / 100;
        if (!ch.UseMoney(price, commit: false)) { Ack(ItemBuyNeedMoney); return; }
        if (!_itemIdReady) { Ack(ItemBuyNotFound); return; }
        item.Count = count;                                                                    // fixed: the C++ gave one
        item.DlId = GenItemId();
        item.EndTime = row.UseTime != 0 ? UnixNow() + row.UseTime * 86400L : 0;               // fixed: 0 days was born expired
        if (!CanPush(ch, new[] { item })) { Ack(ItemBuyCantPush); return; }
        if (npcInven != Proto.InvenNull && npcSlot != Proto.InvalidSlot)
        {
            if (ch.MapId != 0 && ch.MapId != 8) { Ack(ItemBuyInvalidPos); return; }
            var callBag = ch.FindInven(npcInven);
            if (callBag?.FindItem(npcSlot) is not { Count: > 0, Template.Kind: IkNpcCall } call) { Ack(ItemBuyNpcCallError); return; }
            UseItemStack(s, npcInven, callBag, call, 1);
        }
        PushTItem(s, new[] { item });
        ch.UseMoney(price, commit: true);
        Ack(ItemBuySuccess);
    }
}
