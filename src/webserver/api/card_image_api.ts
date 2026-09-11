export interface CardImageHttpResponse {
  readonly ok: boolean;
  readonly status?: number;
  json(): Promise<unknown>;
  text(): Promise<string>;
  arrayBuffer(): Promise<ArrayBuffer>;
}

export interface CardImageHttpRequest {
  readonly method?: string;
  readonly headers?: Readonly<Record<string, string>>;
  readonly body?: unknown;
}

// HTTP routes and body encoding belong to the transport adapter. The injected
// fetch is the application's reset-aware transport; feature code owns recovery.
export function createCardImageApi(
  fetch: (url: string, request?: CardImageHttpRequest) => Promise<CardImageHttpResponse>,
  imageUrl: (id: string) => string,
) {
  const post = (url: string) => fetch(url, { method: "POST" });
  const sessionUrl = (session: string, action: string) =>
    `/api/card-images/restore/${encodeURIComponent(session)}/${action}`;
  return {
    list: () => fetch("/api/card-images"),
    upload: (body: Blob | Uint8Array, session?: string) => fetch(session
      ? `/api/card-images?restore=${encodeURIComponent(session)}` : "/api/card-images", {
      method: "POST", headers: { "Content-Type": "image/jpeg" }, body,
    }),
    rename: (id: string, name: string) => fetch(`/api/card-images/${id}/rename`, {
      method: "POST", headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `name=${encodeURIComponent(name)}`,
    }),
    delete: (id: string) => fetch(`/api/card-images/${id}`, { method: "DELETE" }),
    read: (id: string) => fetch(imageUrl(id)),
    beginRestore: () => post("/api/card-images/restore/begin"),
    commitRestore: (session: string) => post(sessionUrl(session, "commit")),
    rollbackRestore: (session: string) => post(sessionUrl(session, "rollback")),
  };
}
