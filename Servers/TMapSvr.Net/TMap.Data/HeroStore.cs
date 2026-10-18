using System.Data;

namespace TMap.Data;

/// <summary>The hero and warlord writes (MapService.Heroes.cs): a guild chief names a territory's hero (<c>THeroSelect</c>), a
/// last month's leader leaves a message (<c>TSaveWarlordSay</c>).</summary>
public interface IHeroStore
{
    /// <summary><c>THeroSelect</c>: HERO_SELECT_RESULT (0 done, 2 no such character, 6 war time, 7 a hero is already named).</summary>
    Task<int> HeroSelectAsync(uint charId, uint guildId, ushort zoneId, string hero, long time);

    /// <summary><c>TSaveWarlordSay</c>: 0 saved, 1 not the leader.</summary>
    Task<int> SaveWarlordSayAsync(byte type, byte month, uint charId, string say);
}

public sealed partial class GameDatabase : IHeroStore
{
    public async Task<int> HeroSelectAsync(uint charId, uint guildId, ushort zoneId, string hero, long time)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        await SqlProc.ExecAsync(c, "THeroSelect", ret, new[]
        {
            SqlProc.In("@c", SqlDbType.Int, unchecked((int)charId)), SqlProc.In("@g", SqlDbType.Int, unchecked((int)guildId)),
            SqlProc.In("@z", SqlDbType.SmallInt, unchecked((short)zoneId)), SqlProc.In("@h", SqlDbType.VarChar, hero),
            SqlProc.In("@t", SqlDbType.SmallDateTime, FromTime64(time)),
        }, default);
        return ret.AsInt();
    }

    public async Task<int> SaveWarlordSayAsync(byte type, byte month, uint charId, string say)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        await SqlProc.ExecAsync(c, "TSaveWarlordSay", ret, new[]
        {
            SqlProc.In("@t", SqlDbType.TinyInt, type), SqlProc.In("@m", SqlDbType.TinyInt, month),
            SqlProc.In("@c", SqlDbType.Int, unchecked((int)charId)), SqlProc.In("@s", SqlDbType.VarChar, say),
        }, default);
        return ret.AsInt();
    }
}
