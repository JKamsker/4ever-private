# TBot — headless test client

A console "fake client" that drives the live server cluster end-to-end for automated testing:
**(optional) create account → login → reuse/create character → enter the first zone → walk around.**
No MFC/DirectX `TClient` needed.

It reuses the login server's wire toolkit (`TLogin.Protocol`: the asymmetric session cipher, packet
framing, `PacketWriter`/`PacketReader`, `Msg`/`Proto`) and adds the client-facing game-plane (`CS_MAP`)
packets the bot needs (`GamePackets.cs`).

## How it talks to the cluster

```
TBot ──CS_LOGIN plane──▶ login server (C++ TLoginSvr or .NET TLogin.Server, :4816)
     ◀── CS_START_ACK gives the game-server address
TBot ──CS_MAP plane────▶ game server (C++ TMapSvr; coordinates with TWorldSvr behind the scenes)
```

The cipher is asymmetric and **the same for both connections** (TMapSvr also marks client sessions
`SESSION_CLIENT` + `m_bUseCrypt`): send = XOR body+header then RC4 (outermost); recv = XOR only.
The map↔world handshake (`ENTERSVR → CHARINFO/ROUTE → CHARDATA → ENTERCHAR → CHECKMAIN → CONRESULT`)
runs server-side; the bot just waits for `CS_CHARINFO_ACK`, which carries the spawn `mapId`/position.

## Configure

Edit `appsettings.json` (or override on the command line with `--Bot:Key=Value`). Key settings:
`LoginHost`/`LoginPort`, `Account`/`Password`, `GroupId`/`Channel`, the `Char*` template (used only if
the chosen slot is empty), and the `Move*` params. `MoveSpeed` is hard-capped at 3.40 — the map server's
anti-cheat closes the session above that.

Accounts can't be made over the protocol. With `CreateAccount=true` and a `GlobalConnectionString`, TBot
inserts the account into `TGlobal_gsp.dbo.TACCOUNT_PW` (UPPER SHA-1 of `Password`) before logging in —
the same row the seed migration writes (`Database/migrations/001_seed_test_account.sql`).

## Run

```bash
cd Servers/TBot
dotnet run -- --Bot:LoginHost=192.168.1.37 --Bot:Account=test --Bot:Password=test123
# provision a fresh account first:
dotnet run -- --Bot:CreateAccount=true --Bot:Account=bot1 --Bot:Password=pw123 \
              --Bot:GlobalConnectionString="Server=localhost,11433;Database=TGlobal_gsp;User ID=sa;Password=...;TrustServerCertificate=True;Encrypt=False"
```

Prerequisites for a full run: a login server, **TWorldSvr**, and the **C++ TMapSvr** all running and
routed (restore the `.bak` DBs and run `Database/run-migrations.sh` for a local LAN setup).

## Scenario: two bots, every feature

`--Bot:Scenario=features` logs two accounts in side by side and drives party (invite, join, HP bars, loot type,
leader change, kick), mail (send, notify, list, open, take money, delete), hotkeys, bags and a far NPC-portal
teleport against the live cluster. Every reply is parsed the way TClient reads it and must be used up to its last
byte; every packet must decode with a valid checksum; after logout the saved rows (money, position, hotkeys, bag)
are checked in the DB. The characters' money, HP, position and test items are put back at the end.

```bash
dotnet run -- --Bot:Scenario=features --Bot:Account=tbot27 --Bot:Account2=tbot28 --Bot:Password=...               --Bot:Channel=1 --Bot:NoCrypt=false --Bot:MapNoCrypt=false               --Bot:GameConnectionString="Server=localhost,11433;Database=TGame_gsp;User ID=sa;Password=...;TrustServerCertificate=True;Encrypt=False"
```

Exit code 0 when every check passes.

## Scenario: castle (a real castle war)

`--Bot:Scenario=castle` (same arguments as `features`) plays a castle war on Chesed end to end. The database has no
guilds, so it inserts two (A chief of the defenders, B chief of the attackers) and **restarts the world container**
(`docker compose restart worldsvr`) so it loads them; it names them Chesed's sides (`CT_CASTLEGUILDCHG_REQ`), the
chiefs sign up, and the bots come in, carry, mount and knock off god balls, and the war ends on time — result, news,
save, reward mail, everyone sent out. Everything is put back after, and the world and map containers are restarted.
Needs the docker stack, and no fort war due in the next 3 minutes (castle sign-up closes once a fort's war comes first).

## Scenario: guild (a guild's life)

`--Bot:Scenario=guild` (same arguments) drives the map's guild relay against the live world: A (set to level 20)
founds "TbotGuild", invites B who accepts, opens the guild window, makes B vice-chief, gives a peerage; posts, edits
and removes a board article, contributes silver, tries a fame mark (refused: no guild points), puts a stack in the
guild cabinet (refused at guild level 1, then accepted once the test raises the cabinet size) and takes part back out,
opens the point log and PvP record; puts B out, posts a wanted ad that B applies to and A accepts, then disbands. A disbanded guild stays 7 days in the world, so the scenario deletes the guild rows and **restarts the
world container** at the end; the bots' level and guild-leave marks are put back.

A second part (`GuildTacticsScenario.cs`) needs a level-5 guild with money and PvP points, so it inserts "TbotTactics"
(A chief, 1 gold, 1000 points, 1 guild stat point, a run-out Battle Cry) and restarts the world to load it. A renews the
guild skill (300 PvP points), buys a level with the guild's stat point and casts it (the guild's cooldown comes back). A posts a mercenary ad, B applies and is taken (a
welcome letter), B leaves its contract (the guild is paid back), A invites B as a mercenary and B accepts, A fires B
(B gets its points and a letter with its pay); B joins the guild and A gives it 50 guild points; A buys a castle guard
post from the mercenary merchant (23100) with the guild's money. The contract, treasury and mails are checked in the
database. Then everything is removed, the bots' PvP points are restored, and the world **and the map** are restarted (the
bought guards go with the map). The first part also checks the founder's 10 guild skills, the second the guild falling a
level when a member joins below its level's minimum. 59 checks in all.

## Scenario: social (friends, messenger, block list, soulmates)

`--Bot:Scenario=social` (same arguments) needs no restart. A asks B to be friends; A makes a friend group, puts B in it,
renames it and deletes it (refused while B is in it). They talk in the messenger; B leaves and A's next line brings B
back. Then B blocks A: the friendship breaks, A's whisper, trade, duel and party invite are refused, and B can't ask A to
be friends. B logs in again: its block list comes back, and A is not told B came online. B unblocks A and the whisper
gets through again. Last, A searches for a soulmate (finds B, pays the level's fee, saved in `TSOULMATETABLE`), ends the
bond and must wait a day. The bots' friend, group, block-list and soulmate rows and A's money are put back. 37 checks.

## Scenario: corps (four bots: parties at war, member recall)

`--Bot:Scenario=corps` also needs `--Bot:Account3` and `--Bot:Account4` (same country). A and B, then C and D, make
parties (each newcomer is shown its party as a squad). A asks C to form a corps; A's order reaches all four, C's HP report
reaches its party and A (not B). A hands command to C's party, and C takes A's party out, which ends the corps. Then a
recall: D's answer for B is ignored, B's yes brings B to A (A's Evocation Scroll is used), and C's Scroll of Teleportation
takes C to A. The scrolls and B's and C's positions are put back. 14 checks.

## Scenario: auction (the auction house)

`--Bot:Scenario=auction` (same arguments as `features`) uses the Craxion auctioneer (NPC 20841). A puts a sword up (2%
of the direct price paid), B finds it, bids and can't bid over itself, and A cancels: A gets the sword back by mail, B its
bid. A puts a second sword up and B buys it at the direct price: A is paid less 3% by mail, B gets the sword by mail. A
third lot with no time and no bidder ends on its own and goes back to A. Needs migration 020 (`TSaveTax` overflowed on
the baseline's tax totals). The swords, letters, lots and both bots' money are put back. 15 checks.

## Scenario: minigame (gamble, magic shop, soul lottery, rock-paper-scissors)

`--Bot:Scenario=minigame` (same arguments; only A plays) has its gamble fee quoted at Naria (22228) and gambles a sealed potion
(6704, gamble level 40: 2842 copper) that always turns into another (8614) — the change chance needs a real NPC id, as in the
C++. A buys a sabre at the magic shop (NPC 26319, 10 000 copper, 3 days), asks for the soul lottery with an empty bar (no
answer), then plays rock-paper-scissors with tickets (9416) until it wins once and cashes out: 2 silver by the operator's mail.
The potion, sabre, tickets, letter and A's money are put back.

A second part (`MinigameKillScenario.cs`) fights on the field by spawn 4200. A is set to level 1 and kills level-2 monsters
(770) until its soul bar is full, then draws the soul lottery. A and B form a lottery party; monster 770's drop of item 9407 is
made sure (its `TMONITEMCHART` odds raised, the map restarted to load it). A takes two drops: B says no the first time (A gets
it at once) and nothing the second (the corpse's AI draws after its 15 seconds). The odds, A's level, exp and skill points,
both positions and the drawn items are put back, and the map restarted again. 30 checks in all.

## Scenario: drops (monster drops with the real chart)

`--Bot:Scenario=drops` (same arguments; only A) puts A by spawn 4200 (map 0) and kills up to 20 level-2 monsters (770). Their
drop rows are almost all ranged (no item: a random one of the row's group, drop levels and id range), so the run checks that
items drop, that each fits one of the monster's rows (checked against `TITEMCHART`), and that the first one, taken, is saved
after logout. Nothing is changed in the charts. The item and A's position are put back. 4 checks.

## Scenario: events (the operator's events)

`--Bot:Scenario=events` (same arguments; only A) pushes events to the world the way the control tool does (`WorldTool`:
`CT_EVENTUPDATE_REQ`, `CT_EVENTMSG_REQ`, `SM_EVENTQUARTER_REQ`). A (set to level 5) stands by spawn 4200 and kills a monster 770
for its plain exp; a 100% exp event doubles the next kill's exp, and it is plain again once the event ends. A monster-regen event
places a 770 next to A, and its end kills it. An event message reaches A as a world chat line, and the lucky-ticket draw is shown.
Every event is ended and A's level, exp and position are put back. 6 checks.

## Scenario: balance (the kingdom balance)

`--Bot:Scenario=balance` (same arguments) logs A in: its character info comes with the operator's world chat line on the kingdom
balance. A's war-balance window gets no answer — the window counts levels 130 and up, and this game's level chart stops at 125
(the C++ does the same). After the world's next minute tick (up to a minute), B logs in and its line counts A and B, both
Craxion ("Derion" 100%, stage 3). Nothing is changed in the database. 3 checks.

## Scenario: heroes (a fort's hero)

`--Bot:Scenario=heroes` (same arguments) gives the test guild "TBotBlue" (A its chief) fort 1, its next defence in three days,
and **restarts the world and the map** to load it. A reads the hero list (the forts, then the castles), is refused for fort 2
(not its guild's), and names B the fort's hero: saved in `TLOCALTABLE` and shown in the list once the world has told the map.
Needs migration 021 (`THeroSelect` read a missing `TCHARTABLE_PW`). The fort table and the guild are put back and the world and
the map restarted again. 5 checks.

## Scenario: arena (two against two)

`--Bot:Scenario=arena` (needs `--Bot:Account3/4`) puts the four bots by the 3-against-3 arena (map 0, NPC 26156). A and B make
a party, C and D another. A signs up (50 000 copper), then C: the arena is ready, and a minute later the battle starts — a hit
before that does nothing. A and B bring C and D down: C's side is sent out, and A gets the fee by mail. The info window shows
A's side, the ranking answers, and A gives up the waiting arena — out, with the fee back by mail. Positions, money, HP and the
letters are put back. Takes about two minutes. 15 checks.

## Scenario: tournament (the windows)

`--Bot:Scenario=tournament` (same arguments; only A) asks for the tournament schedule (the world's real steps, relayed by the
map), then for the sign-up window — answered as the world's step allows (sign-up info, party list or match list) — and is
refused when signing up for an entry that does not exist. Nothing is written. 3 checks. The matches (batch S6e-2) follow the
world's step clock, days apart, so they are covered by the map's unit tests (`TournamentMatchTests`), not live.

## Scenario: battlequeue (the BoW and BR queues)

`--Bot:Scenario=battlequeue` (A and B) asks to join the BoW queue, leaves it, and asks to join the BR queue: each is answered with
the world's verdict (outside the sign-up time, a refusal) — except BoW when it is off (no `TBOWSETTINGSCHART` row): then, as in the
C++, joining it is not answered, while leaving still is (it is also the BR queue's). A invites B into a BR team: B gets the invite with A's name and refuses,
which comes back to A; inviting someone who isn't online is answered "not found". Nothing is written. 6 checks.

## Scenario: itemleftovers (falls, act items, medals, waste refine, rewards)

`--Bot:Scenario=itemleftovers` (A and B) takes a real act item, medal package, refine stone and special box from the charts. A falls
(B sees it) and is refused when reporting B's fall; A sets off the act item (B sees the cast), uses a medal package (its medals go
up), turns the stone into its item, and opens the box's rewards window. The items are put in A's backpack before and removed after,
with A's HP and medals put back. Stop-the-clock spends cash, so it is left to the unit tests. 6 checks.

## Scenario: stress (hundreds of bots)

`--Bot:Scenario=stress` runs `StressCount` bots in one process, each on its own thread and account
(`StressPrefix` + number, created up front), ramped in at `StressRampPerSec`. Each bot does the normal flow
(login → create char → enter the map → walk `MoveDurationSec`), on a square of a different radius (20..`StressMaxRadius`)
so the crowd spreads over several cells — set `StressMaxRadius=20` to pack everyone into the same cells.

Every 5 s it prints how many bots are connecting / in world / done / kicked / failed, plus the map server's RSS and CPU
(read from `/proc/1` inside the `StressDockerContainer`, default `araz-mapsvr`; empty = don't sample). The summary gives
login and enter latency (p50/p95/max), kicks with the socket error, and failures grouped by message.

```bash
TBot.exe --Bot:Scenario=stress --Bot:StressPrefix=st --Bot:StressCount=400 --Bot:MoveDurationSec=90 \
  --Bot:Password=... --Bot:CreateAccount=true --Bot:GlobalConnectionString="...TGlobal_gsp..." \
  --Bot:Class=3 --Bot:Country=4 --Bot:Hair=4 --Bot:Face=5 --Bot:Channel=1 --Bot:NoCrypt=false --Bot:MapNoCrypt=false
```

Use a **new prefix every run**: an account that was in the world logs in as "Duplicate" until the servers restart.
Exit code 0 when every bot completed its walk.

## Tests

`TBot.Tests` verifies the new wire format **without a live server** by pairing the bot's `ClientCipher`
with the repo's server `SessionCipher`: a `CS_MOVE_REQ` the bot encodes decrypts cleanly server-side, a
server-encoded `CS_CHARINFO_ACK` parses to the right spawn, and the connect checksum matches.

```bash
dotnet test TBot.Tests/TBot.Tests.csproj
```

## Scope

Minimal single-bot proof-of-concept. The `BotConnection`/`ClientCipher`/`BotRunner` split leaves room to
spawn many bots later for load/smoke testing.
