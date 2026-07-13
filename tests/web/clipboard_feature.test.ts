import { createClipboardEntry, planClipboardPaste } from "../../src/webserver/features/clipboard";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

export function runClipboardFeatureTests(): void {
  const entries = [
    createClipboardEntry({ entity: "light.kitchen", label: "Kitchen", type: "light_brightness" }, 3),
    createClipboardEntry({ entity: "scene.movie", label: "Movie", type: "action" }, 1, "subpage-data"),
  ];
  equal(entries[0]?.icon, "Auto", "clipboard entries normalize missing card fields");
  const plan = planClipboardPaste(entries, [1, 0, 0, 2, 0, 0], {}, 1, [3, 4], 6, 3);
  deepEqual(plan.grid, [1, 3, -1, 2, 4, 0], "paste plan reserves card spans and places entries in order");
  equal(plan.sizes["3"], 3, "paste plan retains the copied card size");
  equal(plan.placements[1]?.subpageConfig, "subpage-data", "paste plan carries nested page data");

  const fallback = planClipboardPaste(
    [createClipboardEntry({ entity: "light.one" }, 3)],
    [1, 0, 2, 3, 4, 5],
    {},
    1,
    [6],
    6,
    3,
  );
  equal(fallback.placements[0]?.size, 1, "paste plan falls back to a single card when the span will not fit");
}
