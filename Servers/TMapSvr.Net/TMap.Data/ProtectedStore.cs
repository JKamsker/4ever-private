using System.Data;
using Microsoft.Data.SqlClient;

namespace TMap.Data;

/// <summary>Someone on a character's protected (block) list (C++ <c>TPROTECTED</c>): who, by name, and what is refused from it
/// (a mask of <c>PROTECTED_CHAT</c> 1, <c>INVITE</c> 2, <c>DEAL</c> 4, <c>DUEL</c> 8).</summary>
public readonly record struct ProtectedRow(uint CharId, string Name, byte Option);

/// <summary>
/// The protected list in the database (<c>TPROTECTEDTABLE</c>) — C++ <c>CTBLProtectedTable</c>, <c>CSPProtectedInsert</c>,
/// <c>CSPProtectedDelete</c> and <c>CSPSaveProtectedOption</c>. Behind an interface so the map logic can be tested against an
/// in-memory fake.
/// </summary>
public interface IProtectedStore
{
    Task<List<ProtectedRow>> LoadProtectedAsync(uint charId);

    /// <summary><c>TProtectedInsert</c>: the result (0 done, 1 no such character, 2 already on the list) and the character's id.</summary>
    Task<(int Ret, uint ProtectedId)> ProtectedInsertAsync(uint charId, string name, byte option);

    Task ProtectedDeleteAsync(uint charId, uint protectedId);

    Task SaveProtectedOptionAsync(uint charId, uint protectedId, byte option);
}

public sealed partial class GameDatabase : IProtectedStore
{
    public async Task<List<ProtectedRow>> LoadProtectedAsync(uint charId)
    {
        await using var c = await OpenAsync(default);
        await using var cmd = new SqlCommand("SELECT dwProtected, szName, bOption FROM TPROTECTEDTABLE WHERE dwCharID = @c", c);
        cmd.Parameters.Add(SqlProc.In("@c", SqlDbType.Int, unchecked((int)charId)));
        await using var r = await cmd.ExecuteReaderAsync();
        var list = new List<ProtectedRow>();
        while (await r.ReadAsync()) list.Add(new ProtectedRow(r.GetUIntSafe(0), r.GetStringSafe(1), r.GetByteSafe(2)));
        return list;
    }

    public async Task<(int Ret, uint ProtectedId)> ProtectedInsertAsync(uint charId, string name, byte option)
    {
        await using var c = await OpenAsync(default);
        var ret = SqlProc.Ret();
        var id = SqlProc.Out("@p1", SqlDbType.Int);
        await SqlProc.ExecAsync(c, "TProtectedInsert", ret, new[]
        {
            SqlProc.In("@p0", SqlDbType.Int, unchecked((int)charId)),
            id,
            SqlProc.In("@p2", SqlDbType.VarChar, name, 50),
            SqlProc.In("@p3", SqlDbType.TinyInt, option),
        }, default);
        return (ret.AsInt(), id.Value is DBNull or null ? 0u : unchecked((uint)Convert.ToInt32(id.Value)));
    }

    public async Task ProtectedDeleteAsync(uint charId, uint protectedId)
    {
        await using var c = await OpenAsync(default);
        await SqlProc.ExecAsync(c, "TProtectedDelete", null, new[]
        {
            SqlProc.In("@p0", SqlDbType.Int, unchecked((int)charId)),
            SqlProc.In("@p1", SqlDbType.Int, unchecked((int)protectedId)),
        }, default);
    }

    public async Task SaveProtectedOptionAsync(uint charId, uint protectedId, byte option)
    {
        await using var c = await OpenAsync(default);
        await SqlProc.ExecAsync(c, "TSaveProtectedOption", null, new[]
        {
            SqlProc.In("@p0", SqlDbType.Int, unchecked((int)charId)),
            SqlProc.In("@p1", SqlDbType.Int, unchecked((int)protectedId)),
            SqlProc.In("@p2", SqlDbType.TinyInt, option),
        }, default);
    }
}
