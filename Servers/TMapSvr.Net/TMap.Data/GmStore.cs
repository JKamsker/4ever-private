using System.Data;
using Microsoft.Data.SqlClient;

namespace TMap.Data;

/// <summary>The GM writes the map makes (MapService.Gm.cs).</summary>
public interface IGmStore
{
    /// <summary><c>TCMGiftLog</c> (C++ <c>CSPTCMGiftLog</c>): a gift given (or its error gift), which the world reads back to refuse a
    /// gift that may be taken only so often.</summary>
    Task GiftLogAsync(uint userId, uint charId, ushort giftId, uint gmCharId, ushort errId);
}

public sealed partial class GameDatabase : IGmStore
{
    public async Task GiftLogAsync(uint userId, uint charId, ushort giftId, uint gmCharId, ushort errId)
    {
        await using var c = await OpenAsync(default);
        await SqlProc.ExecAsync(c, "TCMGiftLog", null, new[]
        {
            SqlProc.In("@u", SqlDbType.Int, unchecked((int)userId)), SqlProc.In("@c", SqlDbType.Int, unchecked((int)charId)),
            SqlProc.In("@g", SqlDbType.SmallInt, unchecked((short)giftId)), SqlProc.In("@gm", SqlDbType.Int, unchecked((int)gmCharId)),
            SqlProc.In("@e", SqlDbType.SmallInt, unchecked((short)errId)),
        }, default);
    }
}
