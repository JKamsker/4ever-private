import { useState } from 'react';
import { hex, type IdNames, type Sprites } from '../data.ts';
import type { Editor, Patch } from '../editor.ts';
import { ALIGN, BLT, ID_NULL, TYPE_NAMES, type TifFont, type TifNode } from '../tif/tif.ts';
import { Sprite } from './Sprite.tsx';

interface Props {
  node: TifNode;
  readOnly: boolean;
  isFrame: boolean;
  names: IdNames;
  fonts: TifFont[];
  sprites: Sprites | null;
  editor: Editor;
}

const MENU_EVENTS = ['—', 'LClick', 'RClick', 'DblClick', 'LineUp', 'LineDown', 'VScroll', 'HScroll', 'SelChange', 'TabActive', 'Enter', 'Esc'];

const parseNum = (s: string) => {
  const t = s.trim();
  if (!/^-?(0x[0-9a-f]+|\d+)$/i.test(t)) return null;
  const v = t.startsWith('-') ? -Number(t.slice(1)) : Number(t);
  return Number.isFinite(v) ? v : null;
};

// Text input that commits on Enter/blur; re-keyed per node so it never shows a stale value.
function Field({ value, onCommit, disabled, multiline }: { value: string; onCommit: (s: string) => boolean | void; disabled?: boolean; multiline?: boolean }) {
  const [draft, setDraft] = useState<string | null>(null);
  const commit = () => {
    if (draft !== null && draft !== value && onCommit(draft) === false) return;
    setDraft(null);
  };
  const common = {
    value: draft ?? value,
    disabled,
    className: draft !== null && draft !== value ? 'editing' : '',
    onChange: (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => setDraft(e.target.value),
    onBlur: commit,
    onKeyDown: (e: React.KeyboardEvent) => {
      if (e.key === 'Enter' && !(multiline && e.shiftKey)) { e.preventDefault(); commit(); }
      if (e.key === 'Escape') setDraft(null);
    },
  };
  return multiline ? <textarea rows={2} {...common} /> : <input {...common} />;
}

export function PropsPanel({ node, readOnly, isFrame, names, fonts, sprites, editor }: Props) {
  const set = (patch: Patch) => editor.apply(node, patch);
  const num = (key: 'x' | 'y' | 'width' | 'height' | 'marginH' | 'marginV' | 'sound', label: string) => (
    <label>
      <span>{label}</span>
      <Field key={key} disabled={readOnly} value={String(node[key])} onCommit={s => { const v = parseNum(s); if (v === null) return false; set({ [key]: v }); }} />
    </label>
  );
  const hexField = (key: 'style' | 'color', label: string) => (
    <label>
      <span>{label}</span>
      <Field key={key} disabled={readOnly} value={hex(node[key])} onCommit={s => { const v = parseNum(s); if (v === null) return false; set({ [key]: v >>> 0 }); }} />
    </label>
  );
  const flags = (key: 'align' | 'display', defs: Record<string, number>) => (
    <div className="flags">
      {Object.entries(defs).map(([k, bit]) => (
        <label key={k} className="check">
          <input type="checkbox" disabled={readOnly} checked={(node[key] & bit) !== 0} onChange={e => set({ [key]: e.target.checked ? node[key] | bit : node[key] & ~bit })} />
          {k.toLowerCase()}
        </label>
      ))}
    </div>
  );
  const fontSelect = (key: 'font' | 'tooltipFont', label: string) => (
    <label>
      <span>{label}</span>
      <select disabled={readOnly} value={node[key]} onChange={e => set({ [key]: Number(e.target.value) })}>
        {!fonts.some(f => f.id === node[key]) && <option value={node[key]}>{node[key] === 0 ? '(inherit)' : `#${node[key]} (missing)`}</option>}
        {fonts.map(f => <option key={f.id} value={f.id}>#{f.id} {f.face} {Math.abs(f.height)}px{f.weight >= 700 ? ' bold' : ''}</option>)}
      </select>
    </label>
  );
  const image = (i: 0 | 1, label: string) => (
    <div className="image-field">
      <label>
        <span>{label}</span>
        <Field key={'img' + i} disabled={readOnly} value={node.images[i] === ID_NULL ? 'null' : String(node.images[i])} onCommit={s => {
          const v = s.trim().toLowerCase() === 'null' ? ID_NULL : parseNum(s);
          if (v === null) return false;
          const images: [number, number] = [...node.images];
          images[i] = v >>> 0;
          set({ images });
        }} />
      </label>
      <div className="preview"><Sprite sprites={sprites} id={node.images[i]} fit={96} /></div>
    </div>
  );

  const name = (isFrame ? names.frame.get(node.id) : names.ctrl.get(node.id)) ?? '';
  return (
    <div className="props">
      <h3>{name || hex(node.id)}</h3>
      <div className="meta">
        id {node.id} ({hex(node.id)}) · {TYPE_NAMES[node.type] ?? `type ${node.type}`}
        {node.shared !== undefined && ` · children = shared list #${node.shared}`}
      </div>
      {readOnly && <div className="warn">This node sits inside a shared image list (reused by several slots), so it is read-only for now.</div>}

      <fieldset>
        <legend>Layout</legend>
        <div className="grid2">{num('x', 'X')}{num('y', 'Y')}{num('width', 'Width')}{num('height', 'Height')}{num('marginH', 'Margin H')}{num('marginV', 'Margin V')}</div>
        <div className="hint">Width/height 0 = size of the enable image.</div>
      </fieldset>

      <fieldset>
        <legend>Images</legend>
        {image(0, 'Enable')}
        {image(1, 'Disable')}
        <div className="sub">Display</div>
        {flags('display', BLT)}
      </fieldset>

      <fieldset>
        <legend>Text</legend>
        <label><span>Text</span><Field key="text" multiline disabled={readOnly} value={node.text} onCommit={s => set({ text: s })} /></label>
        <label><span>Tooltip</span><Field key="tooltip" multiline disabled={readOnly} value={node.tooltip} onCommit={s => set({ tooltip: s })} /></label>
        {fontSelect('font', 'Font')}
        {fontSelect('tooltipFont', 'Tip font')}
        <div className="sub">Align</div>
        {flags('align', ALIGN)}
      </fieldset>

      <fieldset>
        <legend>Misc</legend>
        <div className="grid2">{hexField('style', 'Style')}{hexField('color', 'Color')}{num('sound', 'Sound')}</div>
      </fieldset>

      <fieldset>
        <legend>Commands (menu)</legend>
        <div className="grid2">
          {node.menu.map((m, i) => i === 0 ? null : (
            <label key={i}>
              <span>{MENU_EVENTS[i]}</span>
              <Field key={'m' + i} disabled={readOnly} value={m ? hex(m, 4) : '0'} onCommit={s => {
                const v = parseNum(s);
                if (v === null) return false;
                const menu = [...node.menu];
                menu[i] = v >>> 0;
                set({ menu });
              }} />
            </label>
          ))}
        </div>
      </fieldset>
    </div>
  );
}
