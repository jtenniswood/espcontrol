import { clearScreensaverPinForRestore, loadScreensaverPinSettings, screensaverPinRequest } from "../../src/webserver/application/screensaver_pin";

function assert(value: unknown, message: string): asserts value { if (!value) throw new Error(message); }
export async function runScreensaverPinTests(): Promise<void> {
  const calls: RequestInit[] = [];
  let enabled = false, pin = "", fail = false, supported = true;
  let randomize = false;
  const request = (async (url: any, options: RequestInit) => {
    assert(url === "/api/v1/screensaver-pin" || url === "/api/v1/capabilities", "use device-local settings API");
    assert(options.credentials === "include" && options.cache === "no-store", "authenticated uncached requests");
    calls.push(options);
    if (url === "/api/v1/capabilities") return Response.json({ screensaver_pin: supported ? { version: 1 } : undefined });
    if (!supported) return new Response("Not found", { status: 404 });
    if (fail) return new Response("Failed", { status: 500 });
    if (options.method === "POST") {
      const settings = JSON.parse(String(options.body));
      enabled = settings.enabled;
      if (typeof settings.pin === "string") pin = settings.pin;
      if (typeof settings.randomize_keypad === "boolean") randomize = settings.randomize_keypad;
    }
    return Response.json({ enabled, has_pin: !!pin, pin_length: pin.length, randomize_keypad: randomize });
  }) as typeof fetch;
  assert((await screensaverPinRequest(request))?.has_pin === false, "empty PIN status");
  const saved = await screensaverPinRequest(request, { enabled: true, pin: "0042" });
  assert(saved?.enabled && saved.has_pin && saved.pin_length === 4, "save and enable reports digit count");
  assert(!JSON.stringify(saved).includes("0042"), "response holds no secret");
  const layout = await screensaverPinRequest(request, { enabled: true, randomize_keypad: false });
  assert(layout?.randomize_keypad === false && pin === "0042", "layout updates preserve the saved PIN");
  await screensaverPinRequest(request, { enabled: false });
  assert(pin === "0042", "disabling keeps PIN");
  await screensaverPinRequest(request, { enabled: true });
  await screensaverPinRequest(request, { enabled: true, pin: "9876" });
  assert(String(pin) === "9876", "change PIN");
  await clearScreensaverPinForRestore(request);
  assert(String(pin) === "" && !enabled, "import clears destination PIN and protection");
  assert(JSON.parse(String(calls.at(-1)?.body)).pin === "", "restore explicitly clears, never imports a secret");
  assert((await screensaverPinRequest(request))?.pin_length === 0, "cleared PIN reports zero digits");
  fail = true;
  let rejected = false;
  try { await screensaverPinRequest(request, { enabled: true }); } catch { rejected = true; }
  assert(rejected, "save failure is reported");
  fail = false;
  supported = false;
  assert(await screensaverPinRequest(request) === null, "older firmware hides controls");
  const before = calls.length;
  await clearScreensaverPinForRestore(request);
  assert(calls.length === before + 1, "legacy restore does not post unsupported settings");

  const originalTimeout = globalThis.setTimeout;
  globalThis.setTimeout = ((callback: () => void) => { queueMicrotask(callback); return 0; }) as typeof setTimeout;
  try {
    for (const statuses of [[404, 503, 200], [404, 404, 404, 404], [503, 503, 503, 503]]) {
      let attempts = 0, pinReads = 0;
      const bootRequest = (async (url: string) => {
        if (url === "/api/v1/screensaver-pin") {
          pinReads++;
          return Response.json({ enabled: false, has_pin: false });
        }
        const status = statuses[attempts++];
        assert(status !== undefined, "capability discovery stays within its retry limit");
        return status === 200 ? Response.json({ screensaver_pin: { version: 1 } })
          : new Response("Not ready", { status });
      }) as typeof fetch;
      let failed = false, info = null;
      try { info = await loadScreensaverPinSettings(bootRequest); } catch { failed = true; }
      assert(attempts === statuses.length, "startup errors use bounded retries");
      if (statuses.at(-1) === 200) assert(info && pinReads === 1 && !failed, "startup 404/503 recovers PIN controls");
      else if (statuses[0] === 404) assert(info === null && pinReads === 0 && !failed, "legacy 404 hides controls after retries");
      else assert(failed && pinReads === 0, "persistent service failure remains an error");
    }
  } finally { globalThis.setTimeout = originalTimeout; }
}
