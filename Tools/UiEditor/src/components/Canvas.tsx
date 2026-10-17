import { useRef } from 'react';
import { cssColor, type Sprites } from '../data.ts';
import type { Editor } from '../editor.ts';
import { ALIGN, TYPE_IMAGELIST, isNullImage, type TifFont, type TifNode } from '../tif/tif.ts';
import { Sprite, spriteSize } from './Sprite.tsx';

export interface Selection { node: TifNode; readOnly: boolean }

interface Ctx {
  sprites: Sprites | null;
  fonts: Map<number, TifFont>;
  editor: Editor;
  selected: TifNode | null;
  hidden: Set<TifNode>;
  showBounds: boolean;
  zoom: number;
  onSelect: (s: Selection) => void;
}

export function nodeSize(sprites: Sprites | null, node: TifNode) {
  // TComponent::BuildMe: a zero width/height falls back to the enable image's size
  const s = spriteSize(sprites, node.images[0]);
  return { w: node.width || s?.w || 0, h: node.height || s?.h || 0 };
}

// furthest right/bottom edge of a node and its drawn descendants, in the node's parent space
function extent(sprites: Sprites | null, node: TifNode): { r: number; b: number } {
  const { w, h } = nodeSize(sprites, node);
  let r = w, b = h;
  for (const k of node.type === TYPE_IMAGELIST ? node.children.slice(0, 1) : node.children) {
    const e = extent(sprites, k);
    r = Math.max(r, e.r); b = Math.max(b, e.b);
  }
  return { r: node.x + r, b: node.y + b };
}

type Drag = { node: TifNode; mode: 'move' | 'resize'; x0: number; y0: number; start: TifNode; before: ReturnType<Editor['snapshot']> };

export function Canvas(props: Omit<Ctx, 'onSelect'> & { frame: TifNode; screen: boolean; onSelect: (s: Selection) => void }) {
  const { frame, screen, zoom } = props;
  const drag = useRef<Drag | null>(null);

  const ctx: Ctx = props;

  const begin = (e: React.PointerEvent, node: TifNode, mode: Drag['mode']) => {
    const { w, h } = nodeSize(props.sprites, node);
    drag.current = {
      node, mode, x0: e.clientX, y0: e.clientY,
      start: { ...node, width: w, height: h },
      before: props.editor.snapshot(node, mode === 'move' ? ['x', 'y'] : ['width', 'height']),
    };
    (e.currentTarget as Element).setPointerCapture?.(e.pointerId);
  };

  const onMove = (e: React.PointerEvent) => {
    const d = drag.current;
    if (!d) return;
    const dx = Math.round((e.clientX - d.x0) / zoom), dy = Math.round((e.clientY - d.y0) / zoom);
    if (d.mode === 'move') props.editor.live(d.node, { x: d.start.x + dx, y: d.start.y + dy });
    else props.editor.live(d.node, { width: Math.max(1, d.start.width + dx), height: Math.max(1, d.start.height + dy) });
  };

  const onUp = () => {
    const d = drag.current;
    if (d) props.editor.record(d.node, d.before);
    drag.current = null;
  };

  const ox = screen ? 0 : -frame.x, oy = screen ? 0 : -frame.y;
  const e = extent(props.sprites, frame);
  const w = e.r - frame.x, h = e.b - frame.y;
  const width = screen ? Math.max(1024, e.r) : w, height = screen ? Math.max(768, e.b) : h;

  return (
    <div className="canvas-scroll" onPointerMove={onMove} onPointerUp={onUp} onPointerCancel={onUp}>
      <div className="canvas" style={{ width: width * zoom, height: height * zoom }}>
        <div className={screen ? 'screen' : ''} style={{ width, height, transform: `scale(${zoom})`, transformOrigin: '0 0', position: 'relative' }}>
          <div style={{ position: 'absolute', left: ox, top: oy }}>
            <NodeView node={frame} ctx={ctx} readOnly={false} parentFont={undefined} onBegin={begin} />
          </div>
        </div>
      </div>
    </div>
  );
}

function NodeView({ node, ctx, readOnly, parentFont, onBegin }: {
  node: TifNode; ctx: Ctx; readOnly: boolean; parentFont: TifFont | undefined;
  onBegin: (e: React.PointerEvent, node: TifNode, mode: Drag['mode']) => void;
}) {
  if (ctx.hidden.has(node)) return null;
  const { w, h } = nodeSize(ctx.sprites, node);
  const font = ctx.fonts.get(node.font) ?? parentFont;      // TComponent::BuildFont inherits the parent's
  const selected = ctx.selected === node;
  // a TImageList shows one child at a time; draw the first so 2000-icon lists stay cheap
  const kids = node.type === TYPE_IMAGELIST ? node.children.slice(0, 1) : node.children;
  const kidsReadOnly = readOnly || node.shared !== undefined;
  const image = !isNullImage(node.images[0]) ? node.images[0] : node.images[1];

  const justify = node.align & ALIGN.CENTER ? 'center' : node.align & ALIGN.RIGHT ? 'flex-end' : 'flex-start';

  return (
    <div
      className={'node' + (ctx.showBounds ? ' bounds' : '') + (selected ? ' selected' : '') + (readOnly ? ' ro' : '')}
      style={{ left: node.x, top: node.y, width: w, height: h }}
      onPointerDown={e => {
        if (e.button !== 0) return;
        e.stopPropagation();
        ctx.onSelect({ node, readOnly });
        if (!readOnly) onBegin(e, node, 'move');
      }}
    >
      <Sprite sprites={ctx.sprites} id={image} />
      {node.text && (
        <div
          className="text"
          style={{
            justifyContent: justify,
            padding: `${node.marginV}px ${node.marginH}px`,
            fontFamily: font ? `"${font.face}", Gulim, sans-serif` : undefined,
            fontSize: font ? Math.abs(font.height) || 12 : 12,
            fontWeight: font?.weight || undefined,
            fontStyle: font?.italic ? 'italic' : undefined,
            color: font ? cssColor(font.color) : '#fff',
          }}
        >
          {node.text}
        </div>
      )}
      {kids.map((k, i) => (
        <NodeView key={i} node={k} ctx={ctx} readOnly={kidsReadOnly} parentFont={font} onBegin={onBegin} />
      ))}
      {selected && !readOnly && (
        <div className="handle" onPointerDown={e => { e.stopPropagation(); onBegin(e, node, 'resize'); }} />
      )}
    </div>
  );
}
