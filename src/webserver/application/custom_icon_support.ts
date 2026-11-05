// Custom icons only work on firmware compiled with extra glyphs in the icon
// font. The firmware reports that in /api/v1/capabilities; official prebuilt
// firmware, older firmware and unreachable panels all count as "no".
export interface CustomIconSupport {
  available(): boolean;
  ready: Promise<boolean>;
}

const RETRY_DELAY_MS = 2000;
const MAX_ATTEMPTS = 4;

export function createCustomIconSupport(
  request: typeof fetch,
  delay: (callback: () => void, milliseconds: number) => unknown = (callback, milliseconds) =>
    setTimeout(callback, milliseconds),
): CustomIconSupport {
  let available = false;
  const ready = (async () => {
    for (let attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
      try {
        const response = await request("/api/v1/capabilities", { credentials: "include", cache: "no-store" });
        // 503 means the firmware is still starting; anything else is final.
        if (response.status === 503 && attempt < MAX_ATTEMPTS) {
          await new Promise<void>((resolve) => { delay(resolve, RETRY_DELAY_MS); });
          continue;
        }
        if (!response.ok) return false;
        const capabilities = await response.json();
        available = capabilities?.icons?.custom === true;
        return available;
      } catch {
        return false;
      }
    }
    return false;
  })();
  return { available: () => available, ready };
}
