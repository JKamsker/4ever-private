using System.Data;

namespace TMap.Data;

/// <summary>The cash writes the map makes (MapService.ItemLeftovers.cs).</summary>
public interface ICashStore
{
    /// <summary><c>TStopTheClock</c> (C++ <c>CSPStopTheClock</c>): buys a cash item's days for a timed item or bag — 0 done,
    /// otherwise not enough cash. Null when the call failed.</summary>
    Task<int?> StopTheClockAsync(uint charId, ushort shopItemId);
}

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
        catch (Microsoft.Data.SqlClient.SqlException) { return null; }
    }
}
