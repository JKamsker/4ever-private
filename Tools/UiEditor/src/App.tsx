import { useCallback, useEffect, useMemo, useReducer, useState } from 'react';
import { Canvas, nodeSize, type Selection } from './components/Canvas.tsx';
import { PropsPanel } from './components/Props.tsx';
import { Tree } from './components/Tree.tsx';
import { hex, loadIdNames, loadSprites, loadTif, saveTif, type IdNames, type Sprites } from './data.ts';
import { Editor } from './editor.ts';
import type { TifDoc, TifNode } from './tif/tif.ts';

const uids = new WeakMap<TifNode, number>();
let nextUid = 0;
const uid = (n: TifNode) => { if (!uids.has(n)) uids.set(n, ++nextUid); return uids.get(n)!; };

export function App() {
  const [doc, setDoc] = useState<TifDoc | null>(null);
  const [sprites, setSprites] = useState<Sprites | null>(null);
  const [names, setNames] = useState<IdNames>({ frame: new Map(), ctrl: new Map() });
  const [error, setError] = useState('');
  const [status, setStatus] = useState('');
  const [, rerender] = useReducer((x: number) => x + 1, 0);
  const editor = useMemo(() => new Editor(rerender), []);

  const [frameIdx, setFrameIdx] = useState(0);
  const [sel, setSel] = useState<Selection | null>(null);
  const [filter, setFilter] = useState('');
  const [zoom, setZoom] = useState(1);
  const [screen, setScreen] = useState(false);
  const [showBounds, setShowBounds] = useState(true);
  const [hidden, setHidden] = useState(() => new Set<TifNode>());

  useEffect(() => {
    Promise.all([loadTif(), loadSprites(), loadIdNames()])
      .then(([d, s, n]) => {
        // ?frame=<id> opens that frame
        const want = Number(new URLSearchParams(location.search).get('frame'));
        const i = d.frames.findIndex(f => f.id === want);
        if (i >= 0) setFrameIdx(i);
        setDoc(d); setSprites(s); setNames(n);
      })
      .catch(e => setError(String(e)));
  }, []);

  const fonts = useMemo(() => new Map((doc?.fonts ?? []).map(f => [f.id, f])), [doc]);
  const frame = doc?.frames[frameIdx];

  const save = useCallback(async () => {
    if (!doc) return;
    try {
      setStatus('Saving…');
      setStatus(await saveTif(doc));
      editor.saved();
    } catch (e) {
      setStatus('Save failed: ' + (e as Error).message);
    }
  }, [doc, editor]);

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const inField = (e.target as HTMLElement).closest('input, textarea, select');
      if (e.ctrlKey && e.key === 's') { e.preventDefault(); save(); return; }
      if (inField) return;
      if (e.ctrlKey && e.key === 'z') { e.preventDefault(); editor.undo(); return; }
      if (e.ctrlKey && (e.key === 'y' || (e.shiftKey && e.key === 'Z'))) { e.preventDefault(); editor.redo(); return; }
      if (sel && !sel.readOnly && e.key.startsWith('Arrow')) {
        e.preventDefault();
        const step = e.shiftKey ? 10 : 1;
        const dx = e.key === 'ArrowLeft' ? -step : e.key === 'ArrowRight' ? step : 0;
        const dy = e.key === 'ArrowUp' ? -step : e.key === 'ArrowDown' ? step : 0;
        editor.apply(sel.node, { x: sel.node.x + dx, y: sel.node.y + dy });
      }
    };
    const onBeforeUnload = (e: BeforeUnloadEvent) => { if (editor.dirty) e.preventDefault(); };
    window.addEventListener('keydown', onKey);
    window.addEventListener('beforeunload', onBeforeUnload);
    return () => { window.removeEventListener('keydown', onKey); window.removeEventListener('beforeunload', onBeforeUnload); };
  }, [sel, editor, save]);

  const frames = useMemo(() => {
    if (!doc) return [];
    const q = filter.trim().toLowerCase();
    return doc.frames
      .map((f, i) => ({ f, i, name: names.frame.get(f.id) ?? '' }))
      .filter(({ f, name }) => !q || name.toLowerCase().includes(q) || String(f.id) === q || hex(f.id).toLowerCase().includes(q));
  }, [doc, names, filter]);

  if (error) return <div className="fatal">Could not load the UI file: {error}</div>;
  if (!doc || !frame) return <div className="fatal">Loading TClientCmd.tif…</div>;

  const { w, h } = nodeSize(sprites, frame);
  return (
    <div className="app">
      <header>
        <b>4Story UI Editor</b>
        <span className="muted">{doc.frames.length} frames · {doc.fonts.length} fonts</span>
        <div className="spacer" />
        <label className="check"><input type="checkbox" checked={showBounds} onChange={e => setShowBounds(e.target.checked)} />bounds</label>
        <label className="check"><input type="checkbox" checked={screen} onChange={e => setScreen(e.target.checked)} />screen 1024×768</label>
        <label className="check">zoom
          <select value={zoom} onChange={e => setZoom(Number(e.target.value))}>
            {[0.5, 0.75, 1, 1.5, 2, 3].map(z => <option key={z} value={z}>{z * 100}%</option>)}
          </select>
        </label>
        <button disabled={!editor.canUndo} onClick={() => editor.undo()} title="Ctrl+Z">Undo</button>
        <button disabled={!editor.canRedo} onClick={() => editor.redo()} title="Ctrl+Y">Redo</button>
        <button className={editor.dirty ? 'primary' : ''} disabled={!editor.dirty} onClick={save} title="Ctrl+S">Save{editor.dirty ? ' •' : ''}</button>
      </header>
      {!sprites && <div className="banner">No sprites found. Run <code>npm run sprites</code> to extract them from Game/Data/Img, then reload.</div>}
      {status && <div className="banner info" onClick={() => setStatus('')}>{status}</div>}

      <aside className="frames">
        <input className="search" placeholder="Filter frames (name or id)…" value={filter} onChange={e => setFilter(e.target.value)} />
        <div className="list">
          {frames.map(({ f, i, name }) => (
            <div key={i} className={'row' + (i === frameIdx ? ' sel' : '')} onClick={() => { setFrameIdx(i); setSel({ node: f, readOnly: false }); }}>
              <span className="name">{name.replace(/^ID_FRAME_/, '') || hex(f.id)}</span>
              <span className="type">{f.id}</span>
            </div>
          ))}
        </div>
      </aside>

      <main>
        <div className="frame-title">
          {names.frame.get(frame.id) ?? hex(frame.id)} <span className="muted">· {w}×{h} at ({frame.x}, {frame.y})</span>
        </div>
        <Canvas
          frame={frame} screen={screen} zoom={zoom} sprites={sprites} fonts={fonts} editor={editor}
          selected={sel?.node ?? null} hidden={hidden} showBounds={showBounds} onSelect={setSel}
        />
      </main>

      <aside className="side">
        <section className="tree-wrap">
          <Tree
            key={uid(frame)} frame={frame} names={names} selected={sel?.node ?? null} hidden={hidden} onSelect={setSel}
            onToggleHidden={n => setHidden(s => { const c = new Set(s); if (c.has(n)) c.delete(n); else c.add(n); return c; })}
          />
        </section>
        <section className="props-wrap">
          {sel
            ? <PropsPanel key={uid(sel.node)} node={sel.node} readOnly={sel.readOnly} isFrame={sel.node === frame} names={names} fonts={doc.fonts} sprites={sprites} editor={editor} />
            : <div className="muted pad">Select a component in the canvas or the tree.</div>}
        </section>
      </aside>
    </div>
  );
}
