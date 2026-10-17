import { memo } from 'react';
import type { Sprites } from '../data.ts';
import { isNullImage } from '../tif/tif.ts';

// FindSprite(id) -> IMAGESET; components draw frame 0 at native size from their top-left corner
// (TComponent::DrawDefault -> CD3DImage::Render).
export function spriteSize(sprites: Sprites | null, id: number) {
  if (!sprites || isNullImage(id)) return null;
  const img = sprites.images[sprites.sets[id]?.frames[0]];
  return img ? { w: img.w, h: img.h } : null;
}

export const Sprite = memo(function Sprite({ sprites, id, fit }: { sprites: Sprites | null; id: number; fit?: number }) {
  if (!sprites || isNullImage(id)) return null;
  const set = sprites.sets[id];
  const img = set && sprites.images[set.frames[0]];
  if (!img) return null;
  const scale = fit ? Math.min(fit / img.w, fit / img.h, 1) : 1;
  const alpha = (set.color >>> 24) & 255;
  return (
    <div className="sprite" style={{ width: img.w * scale, height: img.h * scale, opacity: alpha ? alpha / 255 : 1 }}>
      <div style={{ width: img.w, height: img.h, transform: `scale(${scale})`, transformOrigin: '0 0', position: 'relative' }}>
        {img.parts.map((p, i) => {
          const [pw, ph] = sprites.pages[p.p] ?? [0, 0];
          const kx = p.sw ? p.w / p.sw : 1, ky = p.sh ? p.h / p.sh : 1;
          return (
            <div
              key={i}
              style={{
                position: 'absolute', left: p.x, top: p.y, width: p.w, height: p.h,
                backgroundImage: `url(/sprites/pages/${p.p}.png)`,
                backgroundPosition: `${-p.sx * kx}px ${-p.sy * ky}px`,
                backgroundSize: `${pw * kx}px ${ph * ky}px`,
              }}
            />
          );
        })}
      </div>
    </div>
  );
});
