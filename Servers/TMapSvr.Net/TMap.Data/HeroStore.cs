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

    /// <summary><c>TTournamentReward</c>: the final's winner rewarded; its fame-list place (C++ <c>CSPTournamentReward</c>).</summary>
    Task<TournamentRewardRow?> TournamentRewardAsync(byte month, ushort tournamentId, byte entryId, uint chiefId, uint winnerId, bool shield);
}

/// <summary>What <c>TTournamentReward</c> gives back: the fame list, its place, and the winner's ranking line.</summary>
public sealed record TournamentRewardRow(byte Type, byte Order, uint MonthRank, uint TotalRank, uint MonthPoint, uint TotalPoint,
    ushort MonthWin, ushort MonthLose, uint TotalWin, uint TotalLose, byte Country, byte Level, byte Class, byte Race, byte Sex, byte Face,
    byte Hair, string Name, string Guild);

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

    public async Task<TournamentRewardRow?> TournamentRewardAsync(byte month, ushort tournamentId, byte entryId, uint chiefId, uint winnerId, bool shield)
    {
        await using var c = await OpenAsync(default);
        var o = new[]
        {
            SqlProc.Out("@t", SqlDbType.TinyInt), SqlProc.Out("@o", SqlDbType.TinyInt), SqlProc.Out("@mr", SqlDbType.Int), SqlProc.Out("@tr", SqlDbType.Int),
            SqlProc.Out("@mp", SqlDbType.Int), SqlProc.Out("@tp", SqlDbType.Int), SqlProc.Out("@mw", SqlDbType.SmallInt), SqlProc.Out("@ml", SqlDbType.SmallInt),
            SqlProc.Out("@tw", SqlDbType.Int), SqlProc.Out("@tl", SqlDbType.Int), SqlProc.Out("@co", SqlDbType.TinyInt), SqlProc.Out("@lv", SqlDbType.TinyInt),
            SqlProc.Out("@cl", SqlDbType.TinyInt), SqlProc.Out("@ra", SqlDbType.TinyInt), SqlProc.Out("@se", SqlDbType.TinyInt), SqlProc.Out("@fa", SqlDbType.TinyInt),
            SqlProc.Out("@ha", SqlDbType.TinyInt), SqlProc.Out("@na", SqlDbType.VarChar, 50), SqlProc.Out("@gu", SqlDbType.VarChar, 50),
        };
        var args = o.Concat(new[]
        {
            SqlProc.In("@m", SqlDbType.TinyInt, month), SqlProc.In("@i", SqlDbType.SmallInt, unchecked((short)tournamentId)),
            SqlProc.In("@e", SqlDbType.TinyInt, entryId), SqlProc.In("@ch", SqlDbType.Int, unchecked((int)chiefId)),
            SqlProc.In("@w", SqlDbType.Int, unchecked((int)winnerId)), SqlProc.In("@s", SqlDbType.TinyInt, shield ? (byte)1 : (byte)0),
        }).ToList();
        await SqlProc.ExecAsync(c, "TTournamentReward", null, args, default);
        return new TournamentRewardRow(o[0].AsByte(), o[1].AsByte(), o[2].AsUInt(), o[3].AsUInt(), o[4].AsUInt(), o[5].AsUInt(),
            (ushort)o[6].AsInt(), (ushort)o[7].AsInt(), o[8].AsUInt(), o[9].AsUInt(), o[10].AsByte(), o[11].AsByte(), o[12].AsByte(),
            o[13].AsByte(), o[14].AsByte(), o[15].AsByte(), o[16].AsByte(), o[17].AsString(), o[18].AsString());
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
