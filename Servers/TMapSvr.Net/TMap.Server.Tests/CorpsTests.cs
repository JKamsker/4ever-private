using TMap.Data;
using TMap.Protocol;
using TMap.Server.Map;
using Xunit;

namespace TMap.Server.Tests;

/// <summary>Batches S2/S3: corps and party member recall on the map — requests passed on, the world's news passed back, the recall
/// item and the teleport the world asks for.</summary>
public class CorpsTests
{
    private const uint Ann = 1, Bob = 2;
    private const ushort Evocation = 7604, Teleportation = 7603;   // IK_MEMBERRECALL, IK_MOVETOCHAR
    private static readonly ItemTemplate EvocationTpl = new(Evocation, 0, new[] { 1f, 0f, 0f, 0f }, Type: 7, Kind: 38);
    private static readonly ItemTemplate TeleportTpl = new(Teleportation, 0, new[] { 1f, 0f, 0f, 0f }, Type: 7, Kind: 39);

    private static async Task<(MapTestHarness h, ClientSession a, FakeClientChannel ca, Character ann, ClientSession b, FakeClientChannel cb, Character bob)> Setup()
    {
        var t = new TemplateStore();
        t.Items[Evocation] = EvocationTpl; t.Items[Teleportation] = TeleportTpl;
        var h = new MapTestHarness(t);
        Character Pc(uint id, string name)
        {
            var ch = new Character { CharId = id, Name = name, Level = 20, MaxHp = 100, Hp = 100 };
            ch.Invens.Add(new Inven { InvenId = 0xFF });
            return ch;
        }
        var ann = Pc(Ann, "Ann"); var bob = Pc(Bob, "Bob");
        var (a, ca) = await h.EnterAsync(Ann, 1, 1, x: 100, z: 100, name: "Ann", preSeeded: ann);
        var (b, cb) = await h.EnterAsync(Bob, 2, 2, x: 101, z: 100, name: "Bob", preSeeded: bob);
        ann.Name = "Ann"; bob.Name = "Bob"; ann.Hp = bob.Hp = 100;
        ca.Clear(); cb.Clear(); h.World.Clear();
        return (h, a, ca, ann, b, cb, bob);
    }

    private static byte[] Cs(ushort id, Action<PacketWriter>? body = null) { var w = new PacketWriter(id); body?.Invoke(w); return w.ToArray(); }

    private static byte[] World(ushort id, ClientSession s, Action<PacketWriter> body)
    {
        var w = new PacketWriter(id);
        w.WriteUInt32(s.CharId); w.WriteUInt32(s.Key);
        body(w);
        return w.ToArray();
    }

    private static void Give(Character ch, byte slot, ItemTemplate tpl, byte count = 2)
        => ch.FindInven(0xFF)!.Items.Add(new Item { ItemSlot = slot, TemplateId = tpl.ItemId, Count = count, Template = tpl });

    [Fact]
    public async Task AReport_GoesToTheWorld_AsTheClientSentIt()
    {
        var (h, a, _, _, _, _, _) = await Setup();

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_CORPSHP_REQ, w => { w.WriteUInt16(0x501); w.WriteUInt32(Ann); w.WriteUInt32(100); w.WriteUInt32(40); }));

        var r = new PacketReader(h.World.Last(Msg.MW_CORPSHP_ACK)!);
        Assert.Equal((Ann, a.Key, (ushort)0x501, Ann, 100u, 40u), (r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt16(), r.ReadUInt32(), r.ReadUInt32(), r.ReadUInt32()));
    }

    [Fact]
    public async Task AnOrder_HasItsMapMovedFirst()
    {
        var (h, a, _, _, _, _, _) = await Setup();

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_CORPSCMD_REQ, w =>
        {
            w.WriteUInt16(0x501); w.WriteUInt32(Bob); w.WriteUInt16(7); w.WriteByte(3); w.WriteUInt32(99); w.WriteByte(1); w.WriteUInt16(10); w.WriteUInt16(20);
        }));

        var r = new PacketReader(h.World.Last(Msg.MW_CORPSCMD_ACK)!);
        r.ReadUInt32(); r.ReadUInt32();
        Assert.Equal(((ushort)7, (ushort)0x501, Bob, (byte)3, 99u, (byte)1, (ushort)10, (ushort)20),
            (r.ReadUInt16(), r.ReadUInt16(), r.ReadUInt32(), r.ReadByte(), r.ReadUInt32(), r.ReadByte(), r.ReadUInt16(), r.ReadUInt16()));
    }

    [Fact]
    public async Task ACorpsInvitation_FromSomeoneBlocked_IsDropped()
    {
        var (h, _, _, _, b, cb, bob) = await Setup();

        await h.Service.DispatchWorldAsync(World(Msg.MW_CORPSASK_REQ, b, w => { w.WriteUInt32(Ann); w.WriteString("Ann"); }));
        Assert.Equal("Ann", new PacketReader(cb.Last(Msg.CS_CORPSASK_ACK)!).ReadString());

        cb.Clear();
        bob.Protected[Ann] = new ProtectedEntry { Name = "Ann", Option = 2 };
        await h.Service.DispatchWorldAsync(World(Msg.MW_CORPSASK_REQ, b, w => { w.WriteUInt32(Ann); w.WriteString("Ann"); }));
        Assert.False(cb.Has(Msg.CS_CORPSASK_ACK));
    }

    [Fact]
    public async Task JoiningACorps_SetsTheCommandingParty()
    {
        var (h, a, ca, ann, _, _, _) = await Setup();

        await h.Service.DispatchWorldAsync(World(Msg.MW_CORPSJOIN_REQ, a, w => { w.WriteUInt16(0x700); w.WriteUInt16(0x502); }));

        Assert.Equal((ushort)0x502, ann.CommanderId);
        var r = new PacketReader(ca.Last(Msg.CS_CORPSJOIN_ACK)!);
        Assert.Equal(((ushort)0x700, (ushort)0x502), (r.ReadUInt16(), r.ReadUInt16()));
    }

    [Fact]
    public async Task ARecall_NeedsAParty()
    {
        var (h, a, ca, ann, _, _, _) = await Setup();
        Give(ann, 40, EvocationTpl);

        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_PARTYMEMBERRECALL_REQ, w => { w.WriteByte(0xFF); w.WriteByte(40); w.WriteString("Bob"); }));
        var r = new PacketReader(ca.Last(Msg.CS_PARTYMEMBERRECALL_ACK)!);
        Assert.Equal(((byte)7, (byte)0, "Bob"), (r.ReadByte(), r.ReadByte(), r.ReadString()));        // IU_NOTPARTY, TP_RECALL

        ann.PartyId = 5;
        await h.Service.DispatchClientAsync(a, Cs(Msg.CS_PARTYMEMBERRECALL_REQ, w => { w.WriteByte(0xFF); w.WriteByte(40); w.WriteString("Bob"); }));
        var mw = new PacketReader(h.World.Last(Msg.MW_PARTYMEMBERRECALL_ACK)!);
        mw.ReadUInt32(); mw.ReadUInt32();
        Assert.Equal(((byte)0xFF, (byte)40, "Ann", "Bob"), (mw.ReadByte(), mw.ReadByte(), mw.ReadString(), mw.ReadString()));
    }

    [Fact]
    public async Task TheMemberAsked_AnswersWithWhereItStands()
    {
        var (h, _, _, _, b, cb, bob) = await Setup();

        await h.Service.DispatchWorldAsync(World(Msg.MW_PARTYMEMBERRECALLANS_REQ, b, w => { w.WriteString("Ann"); w.WriteByte(0); w.WriteByte(0xFF); w.WriteByte(40); }));
        var ask = new PacketReader(cb.Last(Msg.CS_PARTYMEMBERRECALLANS_ACK)!);
        Assert.Equal(("Ann", (byte)0xFF, (byte)40), (ask.ReadString(), ask.ReadByte(), ask.ReadByte()));

        await h.Service.DispatchClientAsync(b, Cs(Msg.CS_PARTYMEMBERRECALLANS_REQ, w => { w.WriteByte(0); w.WriteString("Ann"); w.WriteByte(0xFF); w.WriteByte(40); }));
        var r = new PacketReader(h.World.Last(Msg.MW_PARTYMEMBERRECALLANS_ACK)!);
        Assert.Equal(((byte)0, "Ann", "Bob", (byte)0, (byte)0xFF, (byte)40), (r.ReadByte(), r.ReadString(), r.ReadString(), r.ReadByte(), r.ReadByte(), r.ReadByte()));
        r.ReadByte(); r.ReadUInt16();
        Assert.Equal((bob.PosX, bob.PosZ), (r.ReadFloat(), (r.ReadFloat(), r.ReadFloat()).Item2));
    }

    [Fact]
    public async Task TheWorldsYes_UsesTheItem_AndAsksTheWorldToMoveTheMember()
    {
        var (h, a, ca, ann, _, _, _) = await Setup();
        Give(ann, 40, EvocationTpl);

        await h.Service.DispatchWorldAsync(World(Msg.MW_PARTYMEMBERRECALL_REQ, a, w =>
        {
            w.WriteByte(0); w.WriteString("Bob"); w.WriteByte(0); w.WriteByte(0xFF); w.WriteByte(40); w.WriteByte(1);
            w.WriteUInt16(0); w.WriteFloat(0); w.WriteFloat(0); w.WriteFloat(0);
        }));

        Assert.Equal(1, ann.FindInven(0xFF)!.FindItem(40)!.Count);
        var move = new PacketReader(h.World.Last(Msg.CT_USERMOVE_ACK)!);
        Assert.Equal("Bob", move.ReadString());
        move.ReadByte(); move.ReadUInt16();
        Assert.Equal((ann.PosX, ann.PosZ), (move.ReadFloat(), (move.ReadFloat(), move.ReadFloat()).Item2));
        Assert.Equal(0, new PacketReader(ca.Last(Msg.CS_PARTYMEMBERRECALL_ACK)!).ReadByte());
    }

    [Fact]
    public async Task TheWorldsMove_TeleportsThePlayer()
    {
        var (h, _, _, _, b, cb, _) = await Setup();

        await h.Service.DispatchWorldAsync(World(Msg.CT_USERMOVE_ACK, b, w =>
        {
            w.WriteByte(b.Channel); w.WriteUInt16(0); w.WriteFloat(110); w.WriteFloat(0); w.WriteFloat(105); w.WriteUInt16(0);
        }));

        Assert.True(cb.Has(Msg.CS_TELEPORT_ACK));
    }
}
