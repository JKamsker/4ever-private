import { useEffect, useRef, useState } from 'react';
import { hex, type IdNames } from '../data.ts';
import { TYPE_NAMES, type TifNode } from '../tif/tif.ts';
import type { Selection } from './Canvas.tsx';

const LIST_LIMIT = 200;

interface Props {
  frame: TifNode;
  names: IdNames;
  selected: TifNode | null;
  hidden: Set<TifNode>;
  onToggleHidden: (n: TifNode) => void;
  onSelect: (s: Selection) => void;
}

export function Tree(props: Props) {
  // expand the path down to the selection so canvas clicks reveal the row
  const [open, setOpen] = useState(() => new Set<TifNode>([props.frame]));
  useEffect(() => {
    const path: TifNode[] = [];
    const find = (n: TifNode): boolean => {
      if (n === props.selected) return true;
      for (const k of n.children) if (find(k)) { path.push(n); return true; }
      return false;
    };
    if (props.selected && find(props.frame)) setOpen(o => new Set([...o, ...path]));
  }, [props.selected, props.frame]);

  const toggle = (n: TifNode) => setOpen(o => { const s = new Set(o); if (s.has(n)) s.delete(n); else s.add(n); return s; });
  return (
    <div className="tree">
      <Row node={props.frame} depth={0} readOnly={false} open={open} toggle={toggle} {...props} />
    </div>
  );
}

function Row({ node, depth, readOnly, open, toggle, ...p }: Props & { node: TifNode; depth: number; readOnly: boolean; open: Set<TifNode>; toggle: (n: TifNode) => void }) {
  const ref = useRef<HTMLDivElement>(null);
  const selected = p.selected === node;
  useEffect(() => { if (selected) ref.current?.scrollIntoView({ block: 'nearest' }); }, [selected]);

  const isOpen = open.has(node);
  const name = (depth === 0 ? p.names.frame.get(node.id) : p.names.ctrl.get(node.id)) ?? hex(node.id);
  const kidsReadOnly = readOnly || node.shared !== undefined;
  const kids = isOpen ? node.children.slice(0, LIST_LIMIT) : [];

  return (
    <>
      <div
        ref={ref}
        className={'row' + (selected ? ' sel' : '') + (readOnly ? ' ro' : '') + (p.hidden.has(node) ? ' off' : '')}
        style={{ paddingLeft: 6 + depth * 14 }}
        onClick={() => p.onSelect({ node, readOnly })}
      >
        <span className="twisty" onClick={e => { e.stopPropagation(); toggle(node); }}>
          {node.children.length ? (isOpen ? '▾' : '▸') : ''}
        </span>
        <span className="name" title={name}>{name.replace(/^ID_(FRAME|CTRLINST)_/, '')}</span>
        <span className="type">{TYPE_NAMES[node.type] ?? node.type}{node.shared !== undefined ? ` · shared #${node.shared}` : ''}{node.children.length ? ` · ${node.children.length}` : ''}</span>
        <span className="eye" title="Hide in canvas (view only)" onClick={e => { e.stopPropagation(); p.onToggleHidden(node); }}>
          {p.hidden.has(node) ? '◌' : '●'}
        </span>
      </div>
      {kids.map((k, i) => (
        <Row key={i} node={k} depth={depth + 1} readOnly={kidsReadOnly} open={open} toggle={toggle} {...p} />
      ))}
      {isOpen && node.children.length > LIST_LIMIT && (
        <div className="row more" style={{ paddingLeft: 20 + depth * 14 }}>… {node.children.length - LIST_LIMIT} more</div>
      )}
    </>
  );
}
