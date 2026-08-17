export interface EventSourceLike {
  readonly readyState: number;
  close(): void;
  addEventListener(name: string, listener: (event: { data?: string }) => void): void;
}

export interface ReconnectHandlers<State> {
  readonly onConnected: () => void;
  readonly onDisconnected: () => void;
  readonly onPing: (event: { data?: string }) => void;
  readonly parseState: (event: { data?: string }) => State | null;
  readonly onState: (state: State) => void;
}

export interface ReconnectControllerOptions<State> {
  readonly eventStreamEnabled: () => boolean;
  readonly loadInitialState: (onState: (state: State) => void, onLoaded: () => void) => void;
  readonly createEventSource: () => EventSourceLike;
  readonly getActiveSource: () => EventSourceLike | null;
  readonly setActiveSource: (source: EventSourceLike | null) => void;
  readonly schedule: (callback: () => void, delayMs: number) => unknown;
}

/** Owns the editor's SSE connection and retry lifecycle. */
export class ReconnectController<State> {
  constructor(private readonly options: ReconnectControllerOptions<State>) {}

  connect(handlers: ReconnectHandlers<State>): void {
    const previous = this.options.getActiveSource();
    if (previous) previous.close();
    this.options.setActiveSource(null);
    if (!this.options.eventStreamEnabled()) {
      this.options.loadInitialState(handlers.onState, handlers.onConnected);
      return;
    }
    const source = this.options.createEventSource();
    this.options.setActiveSource(source);
    source.addEventListener("open", handlers.onConnected);
    source.addEventListener("ping", handlers.onPing);
    source.addEventListener("state", (event) => {
      const state = handlers.parseState(event);
      if (state !== null) handlers.onState(state);
    });
    source.addEventListener("error", () => {
      handlers.onDisconnected();
      if (source.readyState !== 2) return;
      source.close();
      if (this.options.getActiveSource() !== source) return;
      this.options.setActiveSource(null);
      this.options.schedule(() => this.connect(handlers), 5000);
    });
  }
}

export function createReconnectController<State>(options: ReconnectControllerOptions<State>): ReconnectController<State> {
  return new ReconnectController(options);
}
