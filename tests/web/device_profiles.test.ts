import { createInitialState } from "../../src/webserver/state/app_state";
import type { DeviceConfig } from "../../src/webserver/state/types";

interface DeviceProfileFixture {
  readonly requiredSlugs: readonly string[];
}

interface ManifestDevice {
  readonly slots: number;
  readonly capabilities: { readonly imageSlots: number };
  readonly public: { readonly screenSize: string };
  readonly layout: { readonly cols: number; readonly rows: number };
  readonly rotation: { readonly enabled: boolean; readonly options: readonly string[] };
  readonly web: {
    readonly dragMode: "displace" | "swap";
    readonly dragAnimation: boolean;
    readonly screen: { readonly width: string; readonly aspect: string };
  };
}

interface DeviceManifest {
  readonly devices: Readonly<Record<string, ManifestDevice>>;
}

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runDeviceProfileTests(manifest: DeviceManifest, fixture: DeviceProfileFixture): void {
  for (const slug of fixture.requiredSlugs) {
    if (!manifest.devices[slug]) throw new Error(`${slug}: required compatibility profile is missing`);
  }

  for (const [slug, profile] of Object.entries(manifest.devices)) {
    equal(profile.layout.cols * profile.layout.rows, profile.slots, `${slug} grid capacity`);
    const deviceConfig: DeviceConfig = {
      slots: profile.slots,
      cols: profile.layout.cols,
      rows: profile.layout.rows,
      screenSize: profile.public.screenSize,
      dragMode: profile.web.dragMode,
      dragAnimation: profile.web.dragAnimation,
      imageSlotCapacity: profile.capabilities.imageSlots,
      screen: profile.web.screen,
      features: profile.rotation.enabled ? {
        screenRotation: true,
        screenRotationDefault: profile.rotation.options[0] || "0",
        screenRotationOptions: [...profile.rotation.options],
      } : {},
    };
    const state = createInitialState(deviceConfig);
    equal(state.grid.length, profile.slots, `${slug} initial grid slots`);
    equal(state.buttons.length, profile.slots, `${slug} initial card slots`);
    equal(state.screenRotationInitialReady, !profile.rotation.enabled, `${slug} rotation readiness`);
  }
}
