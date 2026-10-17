// OfficialPort — pulls content that exists in the official Gameforge FR client (OFFICIAL/) but not in ours (Game/).
//   dotnet run -- analyze          where the costume mesh keys live in both clients
//   dotnet run -- port [--apply]   port the missing costumes (see CostumePort.cs)
using ClassicPort;

string repo = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "../../../../.."));
string game = Path.Combine(repo, "Game"), off = Path.Combine(repo, "OFFICIAL");
string cmd = args.FirstOrDefault() ?? "analyze";

var G = new Client(game, "2_TClientS.IDX", false);
var O = new Client(off, "2_TClientS.IDX", false);

if (cmd == "port")
{
    new CostumePort(game, G, O, Console.Out).Run(args.Contains("--apply"));
    return;
}

if (cmd == "analyze")
{
    uint[] keys = [0x6EFE, 0x6F12, 0x6F1C, 0x6F1A, 0x6F1D, 0x6F1B];
    foreach (var (name, c) in new[] { ("OFF", O), ("OUR", G) })
    {
        int objs = 0;
        foreach (var id in c.Idx[Kind.Obj].ById.Keys)
        {
            var rec = c.Get(Kind.Obj, id)!;
            var cl = ObjCloth.Read(rec.Raw);
            foreach (var k in cl.Kinds) foreach (var cli in k.Cloths) foreach (var m in cli.Meshes)
                if (keys.Contains(m.Key))
                {
                    objs++;
                    Console.WriteLine($"{name} obj {id:X8} clk {k.Id:X} cli {cli.Id:X} key {m.Key:X} mesh {m.MeshId:X8} tex [{string.Join(",", m.TexIds.Select(t => t.ToString("X8")))}] ourHasMesh={G.Has(Kind.Mesh, m.MeshId)} ourHasObj={G.Has(Kind.Obj, id)}");
                }
        }
        Console.WriteLine($"{name}: {objs} hits");
    }
}

