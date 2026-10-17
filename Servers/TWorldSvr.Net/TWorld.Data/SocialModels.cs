namespace TWorld.Data;

/// <summary>A friend-group row (TFRIENDGROUPTABLE): bGroup, szName.</summary>
public readonly record struct FriendGroupRow(byte Group, string Name);

/// <summary>A friend row (TFRIENDTABLE ⋈ TCHARTABLE): friend id, name, group, class, level — and whether the friend has me on its
/// protected list (C++ <c>CSPProtectedSearch(friend, me)</c>), so I see it offline.</summary>
public readonly record struct FriendRow(uint FriendId, string Name, byte Group, byte Class, byte Level, bool Protected = false);

/// <summary>A friend-target row — someone who added me (CTBLFriendTarget): their id + name — and whether I have them on my
/// protected list (C++ <c>CSPProtectedSearch(me, target)</c>), so they are not told I came online.</summary>
public readonly record struct FriendTargetRow(uint FriendId, string Name, bool Protected = false);

/// <summary>Full friend-list load for one char (groups + friends + inbound targets).</summary>
public sealed record FriendLoad(
    List<FriendGroupRow> Groups,
    List<FriendRow> Friends,
    List<FriendTargetRow> Targets);

/// <summary>A soulmate row (TVIEW_SOULMATE): owner, target, name, level, class, time.</summary>
public readonly record struct SoulmateRow(uint CharId, uint Target, string Name, byte Level, byte Class, uint Time);

/// <summary>A PvP-ranking ladder row (CTBLMonthPvPointTable / CTBLPvPointTable / CTBLFirstGradeGroup).</summary>
public sealed record MonthRankRow(
    byte Rank, uint CharId, string Name, uint TotalPoint, uint MonthPoint, ushort MonthWin, ushort MonthLose,
    uint TotalWin, uint TotalLose, byte Country, byte Level, byte Class, byte Race, byte Sex, byte Hair, byte Face,
    string Say, string Guild);
