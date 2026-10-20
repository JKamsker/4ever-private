using TMap.Data;

namespace TMap.Server.Map;

/// <summary>
/// Monster drops — the item a drop row gives (C++ <c>CTMonster::AddItem</c> TMonster.cpp:1080, <c>MakeSpecialItem</c> /
/// <c>SetMagicOpt</c> TMonster.cpp:1187–1350, <c>MonChoiceItem</c> / <c>MonChoiceMagicItem</c> TMapSvr.cpp:6872–6950,
/// <c>CTItem::SetGLevel</c> / <c>SetDuration</c> TItem.cpp:131).
/// <list type="bullet">
/// <item>A chart-type row drops its item — or, with none, one of its group picked by weight among the items of its drop levels,
/// id range and country. The item gets a random durability (11–100%), comes sealed for gambling at the monster's level (always
/// for a gamble item, else <c>wItemProb_G</c>%) or else may roll magic (1–2) or rare (3–5) options at the monster's level.</item>
/// <item>A type-0 row drops a prebuilt magic item (<c>TQUESTITEMCHART</c>) — its own, or one picked in its drop-level and id range.</item>
/// <item>The killer's item-drop and magic-drop buffs raise the odds; no drop when the best-levelled looter is 25+ levels above.</item>
/// </list>
/// <para>Not ported: the PC-bang magic bonus and the item/magic/money drop events.</para>
/// </summary>
public sealed partial class MapService
{
    private const byte SdtStatusItemDrop = 62, SdtStatusMagicDrop = 68;                       // SDT_STATUS_ITEMDROP / _MAGICDROP

    /// <summary>The C++ <c>OnDie</c> drop context: the looters' country, the best looter level, and the killer's drop buffs.</summary>
    private (byte Country, int MaxLevel, int AddMagic, int AddDrop) DropContext(Monster mon, uint killerId)
    {
        byte country = TcontryN;
        int maxLevel = 0, addMagic = 0, addDrop = 0;
        void Buffs(Character ch)
        {
            addMagic += MagicDropBuff(ch);
            addDrop = ItemDropBuff(ch, mon.ItemProb);
        }
        if (mon.KeeperType == OwnerParty)
        {
            foreach (var p in _state.PlayersAround(mon))
                if (p.Char is { } c && c.GetPartyId() == mon.KeeperId)
                {
                    country = WarCountryOf(c);
                    maxLevel = Math.Max(maxLevel, c.Level);
                    if (p.IsMain && c.CharId == killerId) Buffs(c);
                }
        }
        else if (mon.KeeperType == OwnerPrivate && _state.FindByChar(mon.KeeperId)?.Char is { } owner)
        {
            maxLevel = owner.Level;
            country = WarCountryOf(owner);
            Buffs(owner);
        }
        return (country, maxLevel, addMagic, addDrop);
    }

    /// <summary>C++ <c>GetMagicDropBuff</c> (TObjBase.cpp:4863): the first magic-drop buff's value.</summary>
    private int MagicDropBuff(Character ch) => ItemProbBuff(ch, SdtStatusMagicDrop);

    /// <summary>C++ <c>HaveItemDropBuff</c> (TObjBase.cpp:4847): the first item-drop buff, worked out on the monster's item chance.</summary>
    private int ItemDropBuff(Character ch, byte itemProb)
    {
        foreach (var m in ch.MaintainSkills)
            if (_templates.Skills.TryGetValue(m.SkillId, out var t))
                for (int j = 0; j < t.Data.Count; j++)
                    if (t.Data[j].Type == SkillTemplate.SdtStatus && t.Data[j].Exec == SdtStatusItemDrop) return t.Calculate(m.Level, j, itemProb);
        return 0;
    }

    /// <summary>The item a drop row gives, in the corpse slot <paramref name="slot"/> — null when none can be made (the C++ then
    /// stops rolling).</summary>
    private Item? MakeDropItem(Monster mon, MonItemRow row, byte slot, byte country, int addMagic)
    {
        if (row.ChartType == 0)
        {
            var magic = row.ItemId != 0 && _templates.QuestItems.TryGetValue(row.ItemId, out var own) ? own : MonChoiceMagicItem(row, country);
            if (magic is null || ItemFromRow(magic.Item) is not { } prebuilt) return null;
            prebuilt.ItemSlot = slot;
            prebuilt.Count = 1;
            SetEndTime(prebuilt, magic.UseTime * 86400L);
            return prebuilt;
        }

        var tpl = (row.ItemId != 0 ? _templates.Item(row.ItemId) : null) ?? MonChoiceItem(row, country);
        if (tpl is null) return null;
        var item = new Item { ItemSlot = slot, TemplateId = tpl.ItemId, Count = 1, Template = tpl };
        item.DuraMax = (uint)((ulong)tpl.DuraMax * (uint)(LootRng.Next(90) + 11) / 100);   // C++ SetDuration(TRUE)
        item.DuraCur = item.DuraMax;
        SetEndTime(item);
        LinkItemAttr(item);
        if (tpl.Type == ItGamble || (tpl.CanGamble != 0 && LootRng.Next(100) < tpl.ItemProbG)) item.GLevel = mon.Level;   // SetGLevel
        else DropMagic(mon, row, item, addMagic);
        return item;
    }

    /// <summary>C++ <c>CTMonster::MakeSpecialItem</c> (TMonster.cpp:1187): rare (3–5) or magic (1–2) options, at 2/3 of the row's
    /// magic chance (the non-Korean rate), raised by the magic-drop buff.</summary>
    private void DropMagic(Monster mon, MonItemRow row, Item item, int addMagic)
    {
        if (item.Level != 0 || item.Template is not { } t) return;
        item.Magic.Clear();
        int magicProb = row.ProbMagic * 2 / 3;
        if (addMagic != 0) magicProb += magicProb * addMagic / 100;
        if (t.CanMagic == 0 || magicProb <= LootRng.Next(100)) return;
        if (t.CanRare != 0 && row.ProbRare > LootRng.Next(100) && row.RareOpt != 0)
        {
            int n = Math.Min(Math.Max(LootRng.Next(row.RareOpt) + 1, 3), 5);
            for (int i = 0; i < n; i++) DropMagicOpt(mon, item, MipRare);
        }
        else if (row.MagicOpt != 0)
        {
            int n = Math.Min(Math.Max(LootRng.Next(row.MagicOpt) + 1, 1), 2);
            for (int i = 0; i < n; i++) DropMagicOpt(mon, item, MipMagic);
        }
    }

    /// <summary>C++ <c>CTMonster::SetMagicOpt</c> (TMonster.cpp:1279): one option for the item's kind, open at the monster's level, not
    /// on the item nor sharing an exclusion group; its value from the monster's level — at the rare bound one time in three (one in two
    /// in a party zone, maps 900–999), else at the normal bound.</summary>
    private void DropMagicOpt(Monster mon, Item item, byte optType)
    {
        if (!_templates.MagicsByKind.TryGetValue(item.Template!.Kind, out var pool) || pool.Count == 0) return;
        var candidates = pool.Where(m => item.Magic.All(o => o.Id != m.MagicId) && m.MinLevel <= mon.Level
            && (optType != MipMagic || m.IsMagic != 0) && (optType != MipRare || m.IsRare != 0)).ToList();
        MagicTemplate? chosen = null;
        while (candidates.Count > 0)
        {
            int sel = LootRng.Next(candidates.Count);
            var c = candidates[sel];
            if (c.ExclIndex != 0 && item.Magic.Any(o => o.Template?.ExclIndex == c.ExclIndex)) { candidates.RemoveAt(sel); continue; }
            chosen = c;
            break;
        }
        if (chosen is null) return;
        bool partyZone = mon.MapId is >= 900 and < 1000;
        ushort value = (partyZone && LootRng.Next(2) != 0) || LootRng.Next(3) == 0
            ? MakeItemMagicValue(Math.Min(ItemMagicBaseLevel, (int)mon.Level), chosen.RareBound)
            : MakeItemMagicValue(mon.Level, chosen.MaxBound);
        if (value != 0) item.Magic.Add(new MagicOption(chosen.MagicId, value, chosen));
    }

    /// <summary>C++ <c>MonChoiceItem</c> (TMapSvr.cpp:6903): an item of the row's group, from its drop levels, in its id range, for the
    /// country (everyone's items, or all of them for a peace or B-country looter), picked by weight.</summary>
    private ItemTemplate? MonChoiceItem(MonItemRow row, byte country)
    {
        if (!_templates.DropItems.TryGetValue(row.ChartType, out var byLevel)) return null;
        var pick = new List<ItemTemplate>();
        int total = 0;
        foreach (var (level, items) in byLevel)
        {
            if (level < row.LevelMin || level > row.LevelMax) continue;
            foreach (var t in items)
                if (ItemInitState(t) != 0 && (country == TcontryPeace || country == TcontryB || t.ItemCountry == country || t.ItemCountry == TcontryN)
                    && t.ItemId >= row.ItemIdMin && t.ItemId <= row.ItemIdMax)
                {
                    pick.Add(t);
                    total += ItemWeight(t);
                }
        }
        if (pick.Count == 0 || total == 0) return null;
        int choice = LootRng.Next(total), cur = 0;
        foreach (var t in pick) { cur += ItemWeight(t); if (choice < cur) return t; }
        return pick[^1];
    }

    /// <summary>C++ <c>MonChoiceMagicItem</c> (TMapSvr.cpp:6872): a prebuilt magic item of the row's drop levels and id range, for the
    /// country, picked evenly.</summary>
    private QuestItemRow? MonChoiceMagicItem(MonItemRow row, byte country)
    {
        var pick = _templates.QuestItems.Values.Where(q => q.DropLevel >= row.LevelMin && q.DropLevel <= row.LevelMax
                && q.Id >= row.ItemIdMin && q.Id <= row.ItemIdMax
                && _templates.Item(q.Item.TemplateId) is { } t && (t.ItemCountry == country || t.ItemCountry == TcontryN))
            .OrderBy(q => q.Id).ToList();
        return pick.Count == 0 ? null : pick[LootRng.Next(pick.Count)];
    }
}
