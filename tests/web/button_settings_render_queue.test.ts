import { createButtonSettingsRenderQueueFeature } from "../../src/webserver/application/button_settings_render_queue";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runButtonSettingsRenderQueueTests(): void {
  const listeners: Record<string, (event: any) => void> = {};
  const frames: FrameRequestCallback[] = [];
  let settingsOpen = false;
  let settingsFocused = false;
  let previewRenders = 0;
  let settingsRenders = 0;
  let closes = 0;
  const runtime = {
    els: {
      buttonSettings: { contains: () => false },
      settingsOverlay: { classList: { contains: () => true } },
    },
    isSettingsOpen: () => settingsOpen,
    isSettingsFocused: () => settingsFocused,
  } as any;
  const queue = createButtonSettingsRenderQueueFeature(runtime, {
    document: { addEventListener: (name: string, listener: (event: any) => void) => { listeners[name] = listener; } } as any,
    requestFrame: (callback) => { frames.push(callback); return frames.length; },
    renderPreview: () => { previewRenders += 1; },
    renderButtonSettings: () => { settingsRenders += 1; },
    closeSettings: () => { closes += 1; },
  });

  queue.schedule();
  queue.schedule();
  equal(frames.length, 1, "duplicate renders share one animation frame");
  frames.shift()!(0);
  equal(previewRenders, 1, "scheduled frame refreshes the preview");
  equal(settingsRenders, 1, "unfocused settings render immediately");

  settingsFocused = true;
  queue.schedule();
  frames.shift()!(0);
  equal(settingsRenders, 1, "focused settings are deferred");
  settingsFocused = false;
  listeners["focusout"]!({ relatedTarget: null });
  equal(frames.length, 1, "focusout schedules the deferred settings render");
  frames.shift()!(0);
  equal(settingsRenders, 2, "deferred settings render after focus leaves");

  settingsOpen = true;
  listeners["keydown"]!({ key: "Escape" });
  equal(closes, 1, "Escape closes an open settings overlay");
}
