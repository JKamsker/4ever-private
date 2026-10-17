import type { TifNode } from './tif/tif.ts';

export type EditableKey =
  | 'x' | 'y' | 'width' | 'height' | 'marginH' | 'marginV' | 'images' | 'font' | 'tooltipFont'
  | 'style' | 'color' | 'sound' | 'display' | 'align' | 'text' | 'tooltip' | 'menu';
export type Patch = Partial<Pick<TifNode, EditableKey>>;

interface Step { node: TifNode; before: Patch; after: Patch }

// In-place edits on the parsed document with undo/redo. `onChange` re-renders the app.
export class Editor {
  private undoStack: Step[] = [];
  private redoStack: Step[] = [];
  dirty = false;
  private readonly onChange: () => void;
  constructor(onChange: () => void) { this.onChange = onChange; }

  snapshot(node: TifNode, keys: EditableKey[]): Patch {
    const before: Record<string, unknown> = {};
    for (const k of keys) {
      const v = node[k];
      before[k] = Array.isArray(v) ? [...v] : v;
    }
    return before as Patch;
  }

  /** Mutate without recording (live drag); call record() once the gesture ends. */
  live(node: TifNode, patch: Patch) {
    Object.assign(node, patch);
    this.onChange();
  }

  record(node: TifNode, before: Patch) {
    const after = this.snapshot(node, Object.keys(before) as EditableKey[]);
    if (JSON.stringify(after) === JSON.stringify(before)) return;
    this.undoStack.push({ node, before, after });
    this.redoStack = [];
    this.dirty = true;
    this.onChange();
  }

  apply(node: TifNode, patch: Patch) {
    const before = this.snapshot(node, Object.keys(patch) as EditableKey[]);
    Object.assign(node, patch);
    this.record(node, before);
  }

  get canUndo() { return this.undoStack.length > 0; }
  get canRedo() { return this.redoStack.length > 0; }

  undo() {
    const s = this.undoStack.pop();
    if (!s) return;
    Object.assign(s.node, s.before);
    this.redoStack.push(s);
    this.dirty = true;
    this.onChange();
    return s.node;
  }

  redo() {
    const s = this.redoStack.pop();
    if (!s) return;
    Object.assign(s.node, s.after);
    this.undoStack.push(s);
    this.dirty = true;
    this.onChange();
    return s.node;
  }

  saved() { this.dirty = false; this.onChange(); }
}
