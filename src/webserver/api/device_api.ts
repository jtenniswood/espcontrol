export interface FetchRequestInit {
  method?: string;
  cache?: "no-store";
  keepalive?: boolean;
  body?: unknown;
}

export interface FetchResponseLike {
  readonly ok: boolean;
  readonly status: number;
  json(): Promise<unknown>;
  blob?(): Promise<Blob>;
  text?(): Promise<string>;
}

export type FetchLike = (url: string, init?: FetchRequestInit) => Promise<FetchResponseLike>;
export type DelayLike = (milliseconds: number) => Promise<void>;

export type DeviceResult<T = FetchResponseLike> =
  | {
      readonly ok: true;
      readonly kind: "success";
      readonly value: T;
      readonly url: string;
      readonly attemptedUrls: readonly string[];
      readonly status: number;
    }
  | {
      readonly ok: false;
      readonly kind: "http-error";
      readonly value: T;
      readonly url: string;
      readonly attemptedUrls: readonly string[];
      readonly status: number;
    }
  | {
      readonly ok: false;
      readonly kind: "network-error" | "invalid-json";
      readonly error: unknown;
      readonly url: string;
      readonly attemptedUrls: readonly string[];
      readonly status: 0;
    };

export interface DeviceApi {
  request(url: string, init?: FetchRequestInit): Promise<DeviceResult>;
  postQuiet(url: string): Promise<DeviceResult>;
  postFirstAvailable(urls: readonly string[]): Promise<DeviceResult>;
  enqueuePost(urls: readonly string[]): Promise<DeviceResult>;
  getJson<T = unknown>(url: string): Promise<DeviceResult<T>>;
  getJsonFirst<T = unknown>(urls: readonly string[]): Promise<DeviceResult<T> | null>;
  setPostThrottle(milliseconds: number): void;
  queueIdle(): Promise<DeviceResult | null>;
}

function defaultDelay(milliseconds: number): Promise<void> {
  if (milliseconds <= 0) return Promise.resolve();
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

function normalizeUrls(urls: readonly string[]): string[] {
  return urls.map(String).filter(Boolean);
}

export function createDeviceApi(fetchLike: FetchLike, delay: DelayLike = defaultDelay): DeviceApi {
  let postQueue: Promise<DeviceResult | null> = Promise.resolve(null);
  let postThrottleMs = 0;

  async function request(url: string, init?: FetchRequestInit): Promise<DeviceResult> {
    try {
      const response = await fetchLike(url, init);
      const base = { value: response, url, attemptedUrls: [url], status: response.status } as const;
      return response.ok
        ? { ok: true, kind: "success", ...base }
        : { ok: false, kind: "http-error", ...base };
    } catch (error) {
      return { ok: false, kind: "network-error", error, url, attemptedUrls: [url], status: 0 };
    }
  }

  async function postFirstAvailable(urls: readonly string[]): Promise<DeviceResult> {
    const candidates = normalizeUrls(urls);
    if (!candidates.length) {
      return { ok: false, kind: "network-error", error: new Error("No request URL supplied"), url: "", attemptedUrls: [], status: 0 };
    }
    const attemptedUrls: string[] = [];
    for (const url of candidates) {
      attemptedUrls.push(url);
      const result = await request(url, { method: "POST" });
      if (result.kind === "network-error") return { ...result, attemptedUrls };
      if (result.ok || attemptedUrls.length === candidates.length) return { ...result, attemptedUrls };
    }
    throw new Error("Unreachable POST fallback state");
  }

  function enqueuePost(urls: readonly string[]): Promise<DeviceResult> {
    const throttleMs = postThrottleMs;
    const next = postQueue.then(async () => {
      const result = await postFirstAvailable(urls);
      await delay(throttleMs);
      return result;
    });
    postQueue = next;
    return next;
  }

  async function getJson<T = unknown>(url: string): Promise<DeviceResult<T>> {
    const responseResult = await request(url, { cache: "no-store" });
    if (!responseResult.ok) return responseResult as DeviceResult<T>;
    try {
      const value = await responseResult.value.json() as T;
      return { ...responseResult, value };
    } catch (error) {
      return { ok: false, kind: "invalid-json", error, url, attemptedUrls: [url], status: 0 };
    }
  }

  async function getJsonFirst<T = unknown>(urls: readonly string[]): Promise<DeviceResult<T> | null> {
    for (const url of normalizeUrls(urls)) {
      const result = await getJson<T>(url);
      if (result.ok) return result;
    }
    return null;
  }

  return {
    request,
    postQuiet: (url) => request(url, { method: "POST", keepalive: true }),
    postFirstAvailable,
    enqueuePost,
    getJson,
    getJsonFirst,
    setPostThrottle(milliseconds) {
      const parsed = Number.parseInt(String(milliseconds), 10);
      postThrottleMs = Math.max(0, Number.isFinite(parsed) ? parsed : 0);
    },
    queueIdle: () => postQueue,
  };
}
