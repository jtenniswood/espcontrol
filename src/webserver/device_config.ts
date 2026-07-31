import type { DeviceConfig } from "./state/types";

declare const __ESPCONTROL_DEFAULT_DEVICE_ID__: string;
declare const __ESPCONTROL_DEVICE_PROFILES__: Readonly<
  Record<string, DeviceConfig>
>;
declare const __ESPCONTROL_TIMEZONE_OPTIONS__: readonly string[];

export let deviceId = "";
export let deviceConfig: DeviceConfig;

function configuredDeviceId(): string {
  const runtimeDeviceId = (
    globalThis as typeof globalThis & { __ESPCONTROL_DEVICE_PROFILE__?: string }
  ).__ESPCONTROL_DEVICE_PROFILE__;
  if (runtimeDeviceId) return runtimeDeviceId;
  if (typeof document !== "undefined") {
    const script =
      document.currentScript ||
      (typeof document.querySelector === "function"
        ? document.querySelector(
            "script[src*='webserver'][src*='www.js'], script[src='/0.js']",
          )
        : null);
    const src = script && script.getAttribute("src");
    if (src) {
      try {
        const scriptDeviceId =
          new URL(src, window.location.href).searchParams.get("device") || "";
        if (scriptDeviceId) return scriptDeviceId;
      } catch (_error) {
        // Fall through to the test default or local firmware metadata.
      }
    }
  }
  return __ESPCONTROL_DEFAULT_DEVICE_ID__;
}

function selectDeviceProfile(slug: string): void {
  const profile = __ESPCONTROL_DEVICE_PROFILES__[slug];
  if (!profile)
    throw new Error(
      `Unsupported EspControl device profile: ${slug || "missing"}`,
    );
  deviceId = slug;
  deviceConfig = {
    ...profile,
    timezoneOptions: __ESPCONTROL_TIMEZONE_OPTIONS__,
  };
}

export function initializeDeviceConfig(): void | Promise<void> {
  const configured = configuredDeviceId();
  if (configured) {
    selectDeviceProfile(configured);
    return;
  }
  return fetch("/espcontrol/version.json", { cache: "no-store" })
    .then((response) => {
      if (!response.ok)
        throw new Error(`Unable to identify this display (${response.status})`);
      return response.json() as Promise<{ device_slug?: string }>;
    })
    .then((metadata) =>
      selectDeviceProfile(String(metadata.device_slug || "")),
    );
}
