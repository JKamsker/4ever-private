// The server-side half of the port: Database\migrations\017_genesis_mounts_costumes.sql.
// Same technique as ClassicPort's 010 (mounts) and 014 (costumes): every row CLONES a sibling for the columns the
// client file does not carry, then overwrites the ones it does.
using System.Text;
using ClassicPort;

public static class Sql
{
    const ushort MonSibling = 31025;                                   // Brown Horse (plain recall monster)
    const ushort MountItemSibling = 30261;                             // Polar Battle Bear (Permanent): IT_PET / IK_PET
    const ushort HatSibling = 12059, DressSibling = 12061, WingSibling = 25004;   // Hawaiian Headdress / Clothing, Dark Wings

    static string N(string s) => "N'" + s.Replace("'", "''") + "'";

    /// <summary>TITEMCHART columns taken from the client record (same list as ClassicPort / WEB gen:missing).</summary>
    static string ItemAssignments(ItemRec it)
    {
        byte[] r = it.Raw; int t = it.TailOfs;
        byte B(int o) => r[t + o];
        ushort W(int o) => BitConverter.ToUInt16(r, t + o);
        int D(int o) => BitConverter.ToInt32(r, t + o);      // DWORDs go to int columns: keep the bit pattern (0xFFFFFFFF -> -1)
        return string.Join(", ", new[]
        {
            $"wItemID={it.Id}", $"szNAME={N(it.Name)}", $"bType={it.Type}", $"bKind={it.Kind}", $"wAttrID={BitConverter.ToUInt16(r, 4)}",
            $"wUseValue={it.UseValue}", $"dwSlotID={D(0)}", $"dwClassID={D(4)}", $"bPrmSlotID={B(8)}", $"bSubSlotID={B(9)}",
            $"bLevel={B(10)}", $"bCanRepair={B(11)}", $"dwDuraMax={D(12)}", $"bRefineMax={B(16)}", $"bMinRange={B(25)}",
            $"bMaxRange={B(26)}", $"bStack={B(27)}", $"bSlotCount={B(28)}", $"bCanGamble={B(29)}", $"bGambleProb={B(30)}",
            $"bDestroyProb={B(31)}", $"bCanGrade={B(34)}", $"bCanMagic={B(35)}", $"bCanRare={B(36)}", $"wDelayGroupID={W(37)}",
            $"dwDelay={D(39)}", $"bIsSpecial={B(45)}", $"wUseTime={W(46)}", $"bUseType={B(48)}", $"bCanWrap={B(81)}",
            $"dwCode={D(82)}", $"bCanColor={B(86)}",
        });
    }

    static void Item(StringBuilder sb, ItemRec it, ushort sibling, string note)
    {
        sb.AppendLine($"-- {it.Id}  {it.Name}  ({note}, clone of {sibling})");
        sb.AppendLine($"IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = {it.Id}) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = {sibling})");
        sb.AppendLine("BEGIN");
        sb.AppendLine($"    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = {sibling};");
        sb.AppendLine($"    UPDATE #t SET {ItemAssignments(it)};");
        sb.AppendLine("    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;");
        sb.AppendLine("    DROP TABLE #t;");
        sb.AppendLine("END");
        sb.AppendLine("GO");
    }

    public static string Build(List<ItemRec> costumes, List<Import.MountRow> mounts, List<MonRec> monsters, List<ItemRec> mountItems,
        string title = "017_genesis_mounts_costumes", string source = "4Genesis", string tool = "Tools/GenesisPort `port --apply`")
    {
        var sb = new StringBuilder();
        sb.AppendLine($"-- {title}");
        sb.AppendLine("--");
        sb.AppendLine($"-- Mounts + costumes imported from the {source} client ({tool} - generated, do not edit by hand).");
        sb.AppendLine($"--   TMONSTERCHART: {monsters.Count} mount recall monsters ({string.Join(", ", monsters.Select(m => m.Id))}), clones of {MonSibling} (Brown Horse).");
        sb.AppendLine($"--   TMOUNTCHART:   {mounts.Count} mounts ({mounts.Min(m => m.Id)}-{mounts.Max(m => m.Id)}), plain / saddled recall monster.");
        sb.AppendLine($"--   TITEMCHART:    {mountItems.Count} mount items (clones of {MountItemSibling}) + {costumes.Count} costumes");
        sb.AppendLine($"--                  (hats clone {HatSibling}, outfits {DressSibling}, wings {WingSibling}); client-file fields overwritten.");
        sb.AppendLine("-- NOTE: chart tables load once at startup - RESTART TMapSvr/TWorldSvr after applying.");
        sb.AppendLine("-- Idempotent: every insert is guarded by IF NOT EXISTS.");
        sb.AppendLine("USE [TGame_gsp];");
        sb.AppendLine("GO");
        sb.AppendLine();
        sb.AppendLine("SET NOCOUNT ON;");
        sb.AppendLine("GO");
        sb.AppendLine();
        foreach (var m in monsters)
        {
            sb.AppendLine($"-- {m.Id}  {m.Name}");
            sb.AppendLine($"IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = {m.Id}) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = {MonSibling})");
            sb.AppendLine("BEGIN");
            sb.AppendLine($"    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = {MonSibling};");
            sb.AppendLine($"    UPDATE #m SET wID = {m.Id};");
            sb.AppendLine("    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;");
            sb.AppendLine("    DROP TABLE #m;");
            sb.AppendLine("END");
            sb.AppendLine("GO");
        }
        sb.AppendLine();
        foreach (var m in mounts)
            sb.AppendLine($"IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = {m.Id}) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES ({m.Id}, {m.MonId}, {m.SaddleMonId});   -- {m.Name}");
        sb.AppendLine("GO");
        sb.AppendLine();
        foreach (var it in mountItems) Item(sb, it, MountItemSibling, $"mount {it.UseValue}");
        sb.AppendLine();
        foreach (var it in costumes)
            Item(sb, it, it.Kind == 88 ? HatSibling : it.Kind == 250 ? WingSibling : DressSibling, it.Kind == 88 ? "hat" : it.Kind == 250 ? "wings" : "outfit");
        return sb.ToString();
    }
}
