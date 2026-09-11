import { createReconnectController, type EventSourceLike } from "../../src/webserver/features/reconnect";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

class FakeEventSource implements EventSourceLike {
  readyState = 1;
  closed = false;
  private readonly listeners = new Map<string, Array<(event: { data?: string }) => void>>();
  close(): void { this.closed = true; }
  addEventListener(name: string, listener: (event: { data?: string }) => void): void {
    const handlers = this.listeners.get(name) || [];
    handlers.push(listener);
    this.listeners.set(name, handlers);
  }
  emit(name: string, data?: string): void {
    const event = data === undefined ? {} : { data };
    for (const listener of this.listeners.get(name) || []) listener(event);
  }
}

export function runReconnectControllerTests(): void {
  let active: EventSourceLike | null = new FakeEventSource();
  const previous = active as FakeEventSource;
  const created: FakeEventSource[] = [];
  const scheduled: Array<{ callback: () => void; delay: number }> = [];
  const calls: string[] = [];
  let streamsEnabled = true;
  const controller = createReconnectController<string>({
    eventStreamEnabled: () => streamsEnabled,
    loadInitialState: (onState, onLoaded) => { calls.push("load"); onState("initial"); onLoaded(); },
    createEventSource: () => { const source = new FakeEventSource(); created.push(source); return source; },
    getActiveSource: () => active,
    setActiveSource: (source) => { active = source; },
    schedule: (callback, delay) => { scheduled.push({ callback, delay }); },
  });
  const handlers = {
    onConnected: () => calls.push("connected"),
    onDisconnected: () => calls.push("disconnected"),
    onPing: () => calls.push("ping"),
    parseState: (event: { data?: string }) => event.data || null,
    onState: (state: string) => calls.push(`state:${state}`),
  };
  controller.connect(handlers);
  equal(previous.closed, true, "connecting closes the previous stream");
  const first = created[0]!;
  first.emit("open"); first.emit("ping"); first.emit("state", "kitchen"); first.emit("state", "");
  equal(calls.join(","), "connected,ping,state:kitchen", "stream events retain their dispatch contract");
  first.readyState = 2; first.emit("error");
  equal(first.closed, true, "a terminal stream is closed before retrying");
  equal(active, null, "a terminal stream is cleared before retrying");
  equal(scheduled[0]?.delay, 5000, "a terminal stream retries after five seconds");
  scheduled[0]!.callback();
  equal(created.length, 2, "the scheduled retry creates a replacement stream");
  streamsEnabled = false; controller.connect(handlers);
  equal(calls.includes("load"), true, "unsupported streams fall back to the initial state load");
  equal(calls.includes("state:initial"), true, "the fallback retains state delivery");
}
