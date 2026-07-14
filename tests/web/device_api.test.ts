import {
  createDeviceApi,
  type FetchLike,
  type FetchRequestInit,
  type FetchResponseLike,
} from "../../src/webserver/api/device_api";
import { requestFailureInfo } from "../../src/webserver/api/request_failure";

function assert(condition: unknown, message: string): asserts condition {
  if (!condition) throw new Error(message);
}

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function response(status: number, data: unknown = {}): FetchResponseLike {
  return {
    ok: status >= 200 && status < 300,
    status,
    json: async () => data,
    blob: async () => new Blob(),
    text: async () => String(data),
  };
}

export async function runDeviceApiTests(): Promise<void> {
  const calls: Array<{ url: string; init?: FetchRequestInit }> = [];
  const responses = new Map<string, FetchResponseLike | Error>([
    ["/first", response(404)],
    ["/second", response(200)],
    ["/offline", new Error("offline")],
    ["/json", response(200, { state: "ON" })],
    ["/bad-json", { ok: true, status: 200, json: async () => { throw new Error("bad json"); } }],
  ]);
  const fetchLike: FetchLike = async (url, init) => {
    calls.push({ url, ...(init ? { init } : {}) });
    const next = responses.get(url) || response(200);
    if (next instanceof Error) throw next;
    return next;
  };
  const delays: number[] = [];
  const api = createDeviceApi(fetchLike, async (milliseconds) => { delays.push(milliseconds); });

  const fallback = await api.postFirstAvailable(["/first", "/second", "/unused"]);
  assert(fallback.ok, "POST fallback succeeds on the first available URL");
  equal(fallback.url, "/second", "POST fallback reports the successful URL");
  equal(fallback.attemptedUrls.join(","), "/first,/second", "POST fallback preserves exact attempt order");
  equal(calls[0]?.init?.method, "POST", "POST fallback uses the POST method");
  equal(calls[0]?.init?.keepalive, undefined, "normal queued POSTs do not enable keepalive");

  const callCountBeforeNetworkFailure = calls.length;
  const offline = await api.postFirstAvailable(["/offline", "/second"]);
  equal(offline.kind, "network-error", "network failures return a typed failure");
  equal(calls.length, callCountBeforeNetworkFailure + 1, "network failures do not change fallback retry behavior");
  const offlineFailure = requestFailureInfo(offline);
  equal(offlineFailure?.message, "Cannot reach device — is it connected?", "network failure message remains exact");
  equal(offlineFailure?.reconnect, true, "network failures still schedule reconnection");
  const httpFailure = requestFailureInfo(await api.postFirstAvailable(["/first"]));
  equal(httpFailure?.message, "Request failed: 404", "HTTP failure status message remains exact");
  const customFailure = requestFailureInfo(await api.postFirstAvailable(["/first"]), "Could not save configuration.");
  equal(customFailure?.message, "Could not save configuration.", "controller-specific failure messages remain exact");

  const quiet = await api.postQuiet("/quiet");
  assert(quiet.ok, "quiet POST returns a typed success");
  const quietCall = calls.find((call) => call.url === "/quiet");
  equal(quietCall?.init?.keepalive, true, "quiet POST preserves keepalive behavior");

  const json = await api.getJson<{ state: string }>("/json");
  assert(json.ok, "JSON requests return typed data");
  equal(json.value.state, "ON", "JSON requests decode the response body");
  const jsonCall = calls.find((call) => call.url === "/json");
  equal(jsonCall?.init?.cache, "no-store", "state loading bypasses the browser cache");

  const invalidJson = await api.getJson("/bad-json");
  equal(invalidJson.kind, "invalid-json", "invalid JSON returns a typed parsing failure");
  const firstJson = await api.getJsonFirst<{ state: string }>(["/first", "/bad-json", "/json"]);
  assert(firstJson?.ok, "JSON fallback continues past HTTP and parsing failures");
  equal(firstJson.value.state, "ON", "JSON fallback returns the first decoded payload");

  api.setPostThrottle(25);
  const queuedFirst = api.enqueuePost(["/queue-1"]);
  api.setPostThrottle(0);
  const queuedSecond = api.enqueuePost(["/queue-2"]);
  const queuedThird = api.enqueuePost(["/queue-3"]);
  await Promise.all([queuedFirst, queuedSecond, queuedThird]);
  const queueCalls = calls.filter((call) => call.url.startsWith("/queue-")).map((call) => call.url);
  equal(queueCalls.join(","), "/queue-1,/queue-2,/queue-3", "queued POSTs execute in submission order");
  equal(delays.join(","), "25,0,0", "each queued POST retains the throttle active when it was submitted");
  const idle = await api.queueIdle();
  equal(idle?.url, "/queue-3", "queueIdle resolves after the final submitted request");
}
