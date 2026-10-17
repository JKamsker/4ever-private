# UiEditor

Web editor for the compiled client UI, `Game/TClientCmd.tif` (**v2 format only**; convert old v1 files with
`Tools/TifDedupe` first).

```sh
npm install
npm run sprites     # once (and whenever Game/Data/Img changes): bakes the UI sprite pool into public/sprites/
npm run dev         # http://localhost:5173
npm run roundtrip   # parse + rewrite the .tif, must be byte-identical
```

- The dev server reads and **saves `Game/TClientCmd.tif` in place** (`GAME_DIR` overrides the Game folder).
  It is tracked in git (LFS), so `git restore Game/TClientCmd.tif` undoes a save.
- Names come from `Client/History Files/TClientID.h` (`ID_FRAME_*`, `ID_CTRLINST_*`).
- `?frame=<id>` opens a frame directly.
- Editing: click to select, drag to move, corner handle to resize, arrows nudge (Shift = 10px),
  Ctrl+Z / Ctrl+Y, Ctrl+S saves. The properties panel edits layout, images, text/tooltip (CP949), fonts,
  align/display flags, style, colour, sound and the 11 command (menu) ids.
- Nodes inside **shared image lists** (v2 dedupe) are read-only: one list is reused by many slots.
- An image list draws only its first child, as the game shows one at a time.

| Path | What |
|------|------|
| `src/tif/tif.ts` | v2 reader/writer (mirrors `TCMLParser::Load`) |
| `src/tif/cp949.ts` | CP949 encode/decode |
| `scripts/extract-sprites.ts` | `TClientI.IDX` + `*Img.TIM` + `*Src.TIS` → PNG pages + `sprites.json` |
| `vite.config.ts` | `/api/tif` (GET/PUT) and `/api/ids` |
