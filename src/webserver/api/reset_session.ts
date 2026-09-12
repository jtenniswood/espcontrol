export type ResetMode = "customization" | "factory";
export interface ResetStatus { modes: ResetMode[]; epoch: number; pending: boolean }

/** One epoch per page lifetime. Never silently give stale editors a new epoch. */
export class ResetSession {
  private status: ResetStatus | null = null;
  private discovery: Promise<ResetStatus | null> | null = null;
  private blocked = false;
  constructor(private readonly transport: typeof fetch, private readonly onStale: () => void = () => {}) {}

  discover(): Promise<ResetStatus | null> {
    if (!this.discovery) this.discovery = this.discoverStatus().then(status => {
      this.status = status;
      this.blocked = !!status?.pending;
      return status;
    }).catch(error => { this.discovery = null; throw error; });
    return this.discovery;
  }
  private async discoverStatus(): Promise<ResetStatus | null> {
    const response = await this.transport("/api/v1/capabilities", { credentials: "include", cache: "no-store" });
    if (response.status === 404) return null;
    if (!response.ok) throw new Error("Could not check device capabilities. Try again.");
    const value = await response.json();
    if (!value || typeof value !== "object" || Array.isArray(value) ||
        value.api?.version !== 1) throw new Error("Invalid device capabilities");
    if (!("reset" in value)) return null;
    const reset = value.reset;
    if (!reset || reset.status !== "/api/v1/reset" || !Array.isArray(reset.modes) ||
        !reset.modes.length || !reset.modes.every((mode: unknown) => mode === "customization" || mode === "factory"))
      throw new Error("Invalid device reset capabilities");
    return this.loadStatus();
  }
  async loadStatus(): Promise<ResetStatus | null> {
    const response = await this.transport("/api/v1/reset", { credentials: "include", cache: "no-store" });
    if (!response.ok) throw new Error("Could not check device reset status. Reload the page and try again.");
    const value = await response.json();
    if (!value || !Number.isInteger(value.epoch) || value.epoch < 0 || !Array.isArray(value.modes) ||
        !value.modes.every((mode: unknown) => mode === "customization" || mode === "factory") ||
        typeof value.pending !== "boolean") throw new Error("Invalid device reset status");
    return value as ResetStatus;
  }
  async fetch(input: RequestInfo | URL, init?: RequestInit): Promise<Response> {
    const method = (init?.method || "GET").toUpperCase();
    if (method === "GET" || method === "HEAD") return this.transport(input, init);
    const status = await this.discover();
    if (this.blocked) { this.onStale(); throw new Error("Device reset or stale session: reload before changing settings."); }
    const headers = new Headers(init?.headers);
    if (status) headers.set("X-EspControl-Epoch", String(status.epoch));
    const response = await this.transport(input, { ...init, headers });
    if (status && (response.status === 428 || response.status === 409)) {
      // A native config generation conflict is recoverable without reload;
      // check the epoch before deciding whether this entire session is stale.
      const current = await this.loadStatus();
      if (current?.pending || current?.epoch !== status.epoch) { this.blocked = true; this.onStale(); }
    }
    return response;
  }
  async reset(mode: ResetMode): Promise<void> {
    const status = await this.discover();
    if (!status?.modes.includes(mode)) throw new Error("This firmware does not support this reset.");
    if (this.blocked && !status.pending) throw new Error("Reload the page before resetting.");
    this.blocked = true;
    // A network error may mean the accepted request's response was lost. Keep
    // writes blocked until reload instead of replaying queued customization.
    const response = await this.transport("/api/v1/reset", {
      method: "POST", credentials: "include",
      headers: { "Content-Type": "application/json", "X-EspControl-Request": "reset", "X-EspControl-Epoch": String(status.epoch) },
      body: JSON.stringify({ mode }),
    });
    if (response.status !== 202) {
      const detail = await response.json().catch(() => null);
      throw new Error(detail?.error || "Reset was not accepted. Reload the page before trying again.");
    }
  }
  async restarted(): Promise<boolean> {
    const current = await this.loadStatus();
    return !!current && !current.pending && !!this.status && current.epoch > this.status.epoch;
  }
}
let browserSession: ResetSession | null = null;
export function resetSession(): ResetSession {
  if (!browserSession) browserSession = new ResetSession(globalThis.fetch.bind(globalThis), () => {
    document.dispatchEvent(new Event("espcontrol-reset-stale"));
  });
  return browserSession;
}
export function resetAwareFetch(input: RequestInfo | URL, init?: RequestInit): Promise<Response> {
  return resetSession().fetch(input, init);
}
