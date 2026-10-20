# 4Dev — Installation

Get 4Dev running on one Windows PC in **4 steps**: install the tools, get the files, start the
servers, build and launch the game.

- The **servers** all run in **Docker**, so you don't build them yourself.
- The **game client** is built on your machine: with Visual Studio on Windows, or with clang on Linux.

---

## Step 1 — Install the tools

| Tool | Why you need it |
|---|---|
| **Windows 10/11 (x64)** | Runs the servers (Docker) and the game client. The client also runs on Linux: see Step 4. |
| **[Docker Desktop](https://www.docker.com/products/docker-desktop/)** | Runs the database and every game server. |
| **[Git](https://git-scm.com/) + [Git LFS](https://git-lfs.com/)** | Downloads the project, including the big game files. |
| **Visual Studio 2022 or newer** (Community or Build Tools) with **Desktop development with C++** | Builds the game client. CMake and Ninja come with it. |

> Tip: from an **administrator** PowerShell, `scripts\install-build-deps.ps1` installs the Visual Studio
> part for you.

Everything else (DirectX SDK, SDL3, bgfx, zlib…) is already included in the repo.

---

## Step 2 — Get the files

```powershell
git clone git@github.com:Pi-Bouf/4ever-private.git
cd 4ever-private
git lfs install
git lfs pull
```

**Don't skip `git lfs pull`.** Without it, some game files are just tiny placeholders and the game
crashes as soon as it starts. To check, look at this file's size. It should be around **146 MB**, not
a few bytes:

```powershell
(Get-Item "Game\Data\Mesh\Character.TMH").Length
```

---

## Step 3 — Start the servers

1. **Choose a database password.** Create a file named **`.env`** at the root of the project
   (next to `docker-compose.yml`) containing one line:

   ```
   SA_PASSWORD=YourStrong!Passw0rd
   ```

   SQL Server needs a strong password (8+ characters, with upper case, lower case, digits and symbols).
   This file is private, so never commit it.

2. **Set your IP address.** Open `Database\migrations\002_local_network_config.sql` and replace the IP
   in `SET szIPAddr = '...'` with:
   - `127.0.0.1` if you play on this same PC, or
   - this PC's LAN IP (e.g. `192.168.1.20`) if other PCs on your network will connect.

3. **Start everything:**

   ```powershell
   docker compose up -d --build
   ```

   The first start takes a few minutes. Docker creates the databases, sets them up, then starts the
   five game servers.

4. **Check that everything is up:**

   ```powershell
   docker compose ps
   ```

   `mssql-importer` should say **Exited (0)**. That's normal: it only runs once. All the other
   containers should say **Up**.

### What's running

| Server | Service | Port |
|---|---|---|
| Database (SQL Server 2022) | `mssql` | 11433 |
| Login (the game connects here first) | `loginsvr` | 4816 |
| World | `worldsvr` | 3816 |
| Map (the game world itself) | `mapsvr` | 5816 |
| Patch | `patchsvr` | 3716 |
| Control | `controlsvr` | 3615 |

---

## Step 4 — Build and launch the game

The game client runs on **Windows** and on **Linux**. Pick your section.

### Windows

1. **Build the client** (from the project root):

   ```powershell
   .\Client\build.ps1 -Deploy
   ```

   This builds the game and copies `TClient.exe` into the `Game\` folder.

2. **Launch it.** The game needs the server address and port:

   ```powershell
   cd Game
   .\TClient.exe 127.0.0.1 4816
   ```

   Windows asks for **administrator** permission. Accept it.

3. **Log in** with the test account: **`test`** / **`test123`** 🎉

> The window size is set in `Game\config.ini` (`ScreenX` / `ScreenY`, 1600×1200 by default).

### Linux

Tested on Ubuntu 24.04 (and in WSL). Debian 13 works the same way; on Debian 12, add the
[apt.llvm.org](https://apt.llvm.org/) repository first, since its own clang is too old.

1. **Install the build tools** (clang 18, CMake, Ninja and the SDL3 headers). This uses `sudo`:

   ```bash
   ./Client/build-linux.sh --deps
   ```

2. **Build the client:**

   ```bash
   ./Client/build-linux.sh
   ```

   The game is built in `~/build/tclient-linux/bin/TClient`. To use another clang version, prefix
   both commands with it, e.g. `LLVM=19 ./Client/build-linux.sh`.

3. **Launch it** from the `Game` folder, with a `config.ini` next to the binary:

   ```bash
   cp Game/config.ini ~/build/tclient-linux/bin/
   cd Game
   ~/build/tclient-linux/bin/TClient 127.0.0.1 4816
   ```

   Use the IP of the PC running the servers if it's another machine.

4. **Log in** with **`test`** / **`test123`** 🐧

> In WSL the game runs on your GPU through Windows, and picks the right settings by itself. It is
> slower than native Linux, and turning the camera with the mouse is limited.

### Choosing the renderer (`BGFXRenderer`)

The game draws through **bgfx**, which can talk to your graphics card in several ways. You choose
in `config.ini`, in the `[Settings]` section:

```ini
BGFXRenderer=Auto
```

| Value | What it uses |
|---|---|
| `Auto` (default) | The best one for your system. Keep this unless something looks wrong. |
| `D3D11` / `D3D12` | Direct3D (Windows only). |
| `Vulkan` | Vulkan (Windows and Linux). |
| `OpenGL` | OpenGL 4.3 or newer (Windows and Linux; used in WSL). |

If the value isn't available on your PC, the game falls back to `Auto`. For a quick test without
touching the file, set the `TCLIENT_RENDERER` environment variable to one of these values.

---

## Everyday use

| I want to… | Command |
|---|---|
| Start the servers | `docker compose up -d` |
| Stop the servers | `docker compose down` |
| See a server's logs | `docker compose logs mapsvr` (or any service name) |
| Rebuild the servers after a code change | `docker compose up -d --build` |
| Reset the databases to a clean state | `docker compose down -v` then `docker compose up -d` |

> `down -v` **deletes all game data** (characters, items…) and rebuilds the databases from scratch.

---

## Something's wrong?

| Problem | Fix |
|---|---|
| The game crashes right at startup | The big files are missing. Run `git lfs pull` (Step 2). |
| The game opens then closes | Launch it with the address: `TClient.exe 127.0.0.1 4816`. |
| Login works but the game hangs or can't enter the world | The IP in migration `002` is wrong (Step 3.2). Fix it, then reset: `docker compose down -v` and `docker compose up -d`. |
| Servers don't start | Check the database setup: `docker compose logs mssql-importer`. Usually fixed by `docker compose down -v` and `docker compose up -d`. |
| Nothing works after a reboot | Docker Desktop isn't running, or the containers are stopped. Start Docker, then `docker compose up -d`. |
| Can't log in | Wait 1–2 minutes after `docker compose up`, since the servers load game data at boot. Check with `docker compose logs loginsvr`. |

---

Thanks Zemi, thanks Araz <3
