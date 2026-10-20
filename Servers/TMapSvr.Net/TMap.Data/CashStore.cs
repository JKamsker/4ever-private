using System.Data;
using Microsoft.Data.SqlClient;

namespace TMap.Data;

/// <summary>The cash writes and reads the map makes (MapService.CashShop.cs, MapService.ItemLeftovers.cs). The balance is
/// <c>TGlobal_gsp.TCASHTESTTABLE</c> (the baseline's stand-in for the cash server, which its procs reach through
/// <c>TGlobal_gsp</c>); the cabinet is <c>TGlobal_gsp.TCASHITEMCABINETTABLE</c>, per account.</summary>
public interface ICashStore
{
    /// <summary><c>TStopTheClock</c> (C++ <c>CSPStopTheClock</c>): buys a cash item's days for a timed item or bag — 0 done,
    /// otherwise not enough cash. Null when the call failed.</summary>
    Task<int?> StopTheClockAsync(uint charId, ushort shopItemId);

    /// <summary><c>TCashGet</c> (C++ <c>CSPGetCash</c>): the account's cash and bonus.</summary>
    Task<(uint Cash, uint Bonus)> GetCashAsync(uint userId);

    /// <summary><c>TGetCashCabinetUseTime</c> + <c>TVIEW_CASHITEMCABINET</c> (C++ <c>OnDM_CASHITEMCABINET_REQ</c>): the cabinet's
    /// paid-until time (0 when none), the balance, and the account's cabinet items (at most 500) by cabinet id.</summary>
    Task<CashCabinet> CabinetAsync(uint charId, uint userId);

    /// <summary><c>TCashItemGet</c>: a cabinet item into a bag slot. 0 done (the item, with its row id, is in the bag in the
    /// database), else CASHSHOP_RESULT. Null when the call failed.</summary>
    Task<(int Ret, FullItemRow Item)?> CashItemGetAsync(uint userId, uint charId, uint cashItemId, byte inven, byte slot);

    /// <summary><c>TCashItemBuy</c>: buy (type 1) into one's cabinet or present (type 2) into <paramref name="target"/>'s.
    /// Null when the call failed.</summary>
    Task<CashBuyResult?> CashItemBuyAsync(uint userId, uint charId, ushort cashItemId, byte buyType, string target);

    /// <summary><c>TCashCabinetBuy</c>: the cabinet paid for longer. Null when the call failed.</summary>
    Task<(int Ret, long Time, uint Cash, uint Bonus)?> CashCabinetBuyAsync(uint charId, ushort shopItemId);

    /// <summary><c>TCashItemPutin</c>: a bag item moved into the account's cabinet (the row moves in the database). Null when
    /// the call failed.</summary>
    Task<(int Ret, uint NewId)?> CashItemPutinAsync(uint charId, byte inven, byte slot);

    /// <summary><c>TGainCashBonus</c>: a cash bonus item's value added to the bonus (the item's row is deleted). Null when the
    /// call failed.</summary>
    Task<int?> GainCashBonusAsync(uint charId, byte inven, byte slot, uint bonus);
}

public sealed record CashCabinet(long UseTime, uint Cash, uint Bonus, List<(uint Id, FullItemRow Item)> Items);

public sealed record CashBuyResult(int Ret, uint Cash, uint Bonus, long DlId, uint TargetCharId, string Present);

public sealed partial class GameDatabase : ICashStore
{
    public async Task<int?> StopTheClockAsync(uint charId, ushort shopItemId)
    {
        try
        {
            await using var c = await OpenAsync(default);
            var ret = SqlProc.Ret();
            await SqlProc.ExecAsync(c, "TStopTheClock", ret, new[]
            {
                SqlProc.In("@dwCharID", SqlDbType.Int, unchecked((int)charId)),
                SqlProc.In("@wShopItemID", SqlDbType.SmallInt, unchecked((short)shopItemId)),
            }, default);
            return ret.AsInt();
        }
        catch (SqlException) { return null; }
    }

    public async Task<(uint Cash, uint Bonus)> GetCashAsync(uint userId)
    {
        try
        {
            await using var c = await OpenAsync(default);
            var cash = SqlProc.Out("@c", SqlDbType.Int); var bonus = SqlProc.Out("@b", SqlDbType.Int);
            await SqlProc.ExecAsync(c, "TCashGet", null, new[] { SqlProc.In("@u", SqlDbType.Int, unchecked((int)userId)), cash, bonus }, default);
            return (cash.AsUInt(), bonus.AsUInt());
        }
        catch (SqlException) { return (0, 0); }
    }

    public async Task<CashCabinet> CabinetAsync(uint charId, uint userId)
    {
        await using var c = await OpenAsync(default);
        long useTime = 0; uint cash = 0, bonus = 0;
        try
        {
            var ret = SqlProc.Ret();
            var time = SqlProc.Out("@t", SqlDbType.SmallDateTime); var cs = SqlProc.Out("@c", SqlDbType.Int); var bs = SqlProc.Out("@b", SqlDbType.Int);
            await SqlProc.ExecAsync(c, "TGetCashCabinetUseTime", ret, new[] { time, cs, bs, SqlProc.In("@ch", SqlDbType.Int, unchecked((int)charId)) }, default);
            if (ret.AsInt() == 0) { useTime = ToTime64(time.Value); cash = cs.AsUInt(); bonus = bs.AsUInt(); }
        }
        catch (SqlException) { }

        var items = new List<(uint, FullItemRow)>();
        await using var cmd = new SqlCommand(@"SELECT TOP 500 dwID, wItemID, bLevel, bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime,
                 bGradeEffect, bMagic1, bMagic2, bMagic3, bMagic4, bMagic5, bMagic6, wValue1, wValue2, wValue3, wValue4, wValue5, wValue6,
                 dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6, bGem, wMoggItemID FROM TVIEW_CASHITEMCABINET WHERE dwUserID = @u", c);
        cmd.Parameters.Add(SqlProc.In("@u", SqlDbType.Int, unchecked((int)userId)));
        await using var r = await cmd.ExecuteReaderAsync();
        while (await r.ReadAsync())
        {
            var magic = new byte[6]; var value = new ushort[6]; var ext = new uint[6];
            for (int i = 0; i < 6; i++) magic[i] = r.GetByteSafe(10 + i);
            for (int i = 0; i < 6; i++) value[i] = r.GetUShortSafe(16 + i);
            for (int i = 0; i < 6; i++) ext[i] = r.GetUIntSafe(22 + i);
            items.Add((r.GetUIntSafe(0), new FullItemRow(0, 0, 0, r.GetUShortSafe(1), r.GetByteSafe(2), r.GetByteSafe(3), r.GetByteSafe(4),
                r.GetUIntSafe(5), r.GetUIntSafe(6), r.GetByteSafe(7), ToTime64(r, 8), r.GetByteSafe(9), magic, value, ext,
                r.GetByteSafe(28), r.GetUShortSafe(29), 0)));
        }
        return new CashCabinet(useTime, cash, bonus, items);
    }

    public async Task<(int Ret, FullItemRow Item)?> CashItemGetAsync(uint userId, uint charId, uint cashItemId, byte inven, byte slot)
    {
        try
        {
            await using var c = await OpenAsync(default);
            var ret = SqlProc.Ret();
            var o = new List<SqlParameter>
            {
                SqlProc.Out("@dl", SqlDbType.BigInt), SqlProc.Out("@id", SqlDbType.SmallInt), SqlProc.Out("@lv", SqlDbType.TinyInt),
                SqlProc.Out("@cn", SqlDbType.TinyInt), SqlProc.Out("@gl", SqlDbType.TinyInt), SqlProc.Out("@dm", SqlDbType.Int),
                SqlProc.Out("@dc", SqlDbType.Int), SqlProc.Out("@rc", SqlDbType.TinyInt), SqlProc.Out("@et", SqlDbType.SmallDateTime),
                SqlProc.Out("@ge", SqlDbType.TinyInt),
            };
            for (int i = 0; i < 6; i++) o.Add(SqlProc.Out($"@m{i}", SqlDbType.TinyInt));
            for (int i = 0; i < 6; i++) o.Add(SqlProc.Out($"@v{i}", SqlDbType.SmallInt));
            for (int i = 0; i < 6; i++) o.Add(SqlProc.Out($"@t{i}", SqlDbType.Int));
            o.Add(SqlProc.Out("@gem", SqlDbType.TinyInt)); o.Add(SqlProc.Out("@mog", SqlDbType.SmallInt));
            var args = new List<SqlParameter>(o)
            {
                SqlProc.In("@u", SqlDbType.Int, unchecked((int)userId)), SqlProc.In("@ch", SqlDbType.Int, unchecked((int)charId)),
                SqlProc.In("@ci", SqlDbType.Int, unchecked((int)cashItemId)), SqlProc.In("@in", SqlDbType.TinyInt, inven),
                SqlProc.In("@sl", SqlDbType.TinyInt, slot),
            };
            await SqlProc.ExecAsync(c, "TCashItemGet", ret, args, default);
            var magic = new byte[6]; var value = new ushort[6]; var ext = new uint[6];
            for (int i = 0; i < 6; i++) { magic[i] = o[10 + i].AsByte(); value[i] = unchecked((ushort)o[16 + i].AsInt()); ext[i] = o[22 + i].AsUInt(); }
            long dl = o[0].Value is null or DBNull ? 0 : Convert.ToInt64(o[0].Value);
            var item = new FullItemRow(0, inven, slot, unchecked((ushort)o[1].AsInt()), o[2].AsByte(), o[3].AsByte(), o[4].AsByte(), o[5].AsUInt(),
                o[6].AsUInt(), o[7].AsByte(), ToTime64(o[8].Value), o[9].AsByte(), magic, value, ext, o[28].AsByte(),
                unchecked((ushort)o[29].AsInt()), dl);
            return (ret.AsInt(), item);
        }
        catch (SqlException) { return null; }
    }

    public async Task<CashBuyResult?> CashItemBuyAsync(uint userId, uint charId, ushort cashItemId, byte buyType, string target)
    {
        try
        {
            await using var c = await OpenAsync(default);
            var ret = SqlProc.Ret();
            var cash = SqlProc.Out("@c", SqlDbType.Int); var bonus = SqlProc.Out("@b", SqlDbType.Int); var dl = SqlProc.Out("@dl", SqlDbType.BigInt);
            var targetId = SqlProc.Out("@tc", SqlDbType.Int); var present = SqlProc.Out("@p", SqlDbType.VarChar, 50);
            await SqlProc.ExecAsync(c, "TCashItemBuy", ret, new[]
            {
                cash, bonus, dl, targetId, present,
                SqlProc.In("@u", SqlDbType.Int, unchecked((int)userId)), SqlProc.In("@ch", SqlDbType.Int, unchecked((int)charId)),
                SqlProc.In("@ci", SqlDbType.SmallInt, unchecked((short)cashItemId)), SqlProc.In("@bt", SqlDbType.TinyInt, buyType),
                SqlProc.In("@tg", SqlDbType.VarChar, target, 50),
            }, default);
            return new CashBuyResult(ret.AsInt(), cash.AsUInt(), bonus.AsUInt(), dl.Value is null or DBNull ? 0 : Convert.ToInt64(dl.Value),
                targetId.AsUInt(), present.AsString());
        }
        catch (SqlException) { return null; }
    }

    public async Task<(int Ret, long Time, uint Cash, uint Bonus)?> CashCabinetBuyAsync(uint charId, ushort shopItemId)
    {
        try
        {
            await using var c = await OpenAsync(default);
            var ret = SqlProc.Ret();
            var time = SqlProc.Out("@t", SqlDbType.SmallDateTime); var cash = SqlProc.Out("@c", SqlDbType.Int); var bonus = SqlProc.Out("@b", SqlDbType.Int);
            await SqlProc.ExecAsync(c, "TCashCabinetBuy", ret, new[]
            {
                time, cash, bonus,
                SqlProc.In("@si", SqlDbType.SmallInt, unchecked((short)shopItemId)), SqlProc.In("@ch", SqlDbType.Int, unchecked((int)charId)),
            }, default);
            return (ret.AsInt(), ToTime64(time.Value), cash.AsUInt(), bonus.AsUInt());
        }
        catch (SqlException) { return null; }
    }

    public async Task<(int Ret, uint NewId)?> CashItemPutinAsync(uint charId, byte inven, byte slot)
    {
        try
        {
            await using var c = await OpenAsync(default);
            var ret = SqlProc.Ret();
            var id = SqlProc.Out("@n", SqlDbType.Int);
            await SqlProc.ExecAsync(c, "TCashItemPutin", ret, new[]
            {
                id, SqlProc.Out("@c", SqlDbType.Int), SqlProc.Out("@b", SqlDbType.Int),
                SqlProc.In("@ch", SqlDbType.Int, unchecked((int)charId)), SqlProc.In("@in", SqlDbType.TinyInt, inven),
                SqlProc.In("@sl", SqlDbType.TinyInt, slot),
            }, default);
            return (ret.AsInt(), id.AsUInt());
        }
        catch (SqlException) { return null; }
    }

    public async Task<int?> GainCashBonusAsync(uint charId, byte inven, byte slot, uint bonus)
    {
        try
        {
            await using var c = await OpenAsync(default);
            var ret = SqlProc.Ret();
            await SqlProc.ExecAsync(c, "TGainCashBonus", ret, new[]
            {
                SqlProc.In("@ch", SqlDbType.Int, unchecked((int)charId)), SqlProc.In("@in", SqlDbType.TinyInt, inven),
                SqlProc.In("@sl", SqlDbType.TinyInt, slot), SqlProc.In("@b", SqlDbType.Int, unchecked((int)bonus)),
            }, default);
            return ret.AsInt();
        }
        catch (SqlException) { return null; }
    }
}
