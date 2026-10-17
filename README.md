# 4Dev

A private, self-hosted **4Story** MMO: a modernized game client and servers rewritten in .NET, built
on the original RageZone "4Story 5.0" release.

👉 **New here? Follow the [Installation guide](INSTALLATION.md).**

## What's inside

| Folder | What it is |
|---|---|
| `Client/` | Game client source (C++, rendered with bgfx, windowed with SDL3) |
| `Game/` | The runnable game: client exe, assets and `config.ini` |
| `Servers/` | Game servers. The `*.Net` folders are the ones in use (Login, World, Map, Patch, Control) and the original C++ servers are kept for reference |
| `Database/` | Database setup and migrations, applied automatically by Docker |
| `Lib/` | Engine, UI and network libraries, plus bundled third-party code |
| `Tools/` | Launcher, UI editor, asset import tools… |

## Quick start

```powershell
git lfs pull                       # fetch the big game files
docker compose up -d --build       # start the database + all servers
.\Client\build.ps1 -Deploy         # build the game client
cd Game; .\TClient.exe 127.0.0.1 4816
```

Log in with **`test`** / **`test123`**. Before the first run, create a `.env` file with your database
password. All the details are in [INSTALLATION.md](INSTALLATION.md).

---

Thanks Zemi, thanks Araz <3
