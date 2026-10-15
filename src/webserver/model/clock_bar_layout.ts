import type { ClockBarGroup } from "./clock_bar";
export interface ClockBarMeasuredItem { id: string; group: ClockBarGroup; width: number }
export interface ClockBarPlacement { id: string; x: number; width: number }
/** Preserve outer anchors, drop middle first, and never overlap. Widths include each element's hit box. */
export function placeClockBarItems(input: readonly ClockBarMeasuredItem[], width: number, gap: number, titleWidth = 0): { items: ClockBarPlacement[]; titleWidth: number } {
  const groups = { left: input.filter(i => i.group === "left" && !titleWidth), middle: input.filter(i => i.group === "middle"), right: input.filter(i => i.group === "right") };
  const size = (items: readonly ClockBarMeasuredItem[]) => items.reduce((sum, i) => sum + i.width, 0) + Math.max(0, items.length - 1) * gap;
  const required = () => {
    const widths = [titleWidth || size(groups.left), size(groups.middle), size(groups.right)].filter(w => w > 0);
    return widths.reduce((sum, w) => sum + w, 0) + Math.max(0, widths.length - 1) * gap;
  };
  while (required() > width && groups.middle.length) groups.middle.pop();
  while (required() > width && groups.left.length) groups.left.pop();
  // Keep a readable title beside the right group. Right elements nearest the middle go first.
  while ((size(groups.right) > width || (titleWidth > 0 && size(groups.right) + gap + Math.min(titleWidth, 48) > width)) && groups.right.length) groups.right.shift();
  const rightWidth = size(groups.right);
  const actualTitle = titleWidth ? Math.max(0, Math.min(titleWidth, width - rightWidth - (rightWidth ? gap : 0))) : 0;
  const leftWidth = actualTitle || size(groups.left);
  const middleWidth = size(groups.middle);
  const placements: ClockBarPlacement[] = [];
  const append = (items: ClockBarMeasuredItem[], start: number) => {
    let x = start;
    for (const item of items) { placements.push({ id: item.id, x, width: item.width }); x += item.width + gap; }
  };
  append(groups.left, 0);
  const start = leftWidth ? leftWidth + gap : 0;
  const end = width - rightWidth - (rightWidth ? gap : 0);
  append(groups.middle, start + Math.max(0, end - start - middleWidth) / 2);
  append(groups.right, width - rightWidth);
  return { items: placements, titleWidth: actualTitle };
}
