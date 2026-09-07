export interface CompanionAction { readonly id: string; readonly label: string; }

export function createCompanionCatalogue(fetchImpl: typeof fetch) {
  let cached: readonly CompanionAction[] | null = null;
  let pending: Promise<readonly CompanionAction[]> | null = null;
  async function fetchActions(): Promise<readonly CompanionAction[]> {
    const response = await fetchImpl("/companion/actions", { cache: "no-store" });
    if (!response.ok) throw new Error("Companion actions unavailable");
    const data: unknown = await response.json();
    if (!Array.isArray(data)) return [];
    return data.filter((item): item is CompanionAction =>
      typeof item?.id === "string" && typeof item?.label === "string");
  }
  return {
    load(refresh = false): Promise<readonly CompanionAction[]> {
      if (pending) return pending;
      if (!refresh && cached) return Promise.resolve(cached);
      const request = fetchActions().then((actions) => { cached = actions; return actions; });
      pending = request;
      void request.then(() => { if (pending === request) pending = null; },
        () => { if (pending === request) pending = null; });
      return request;
    },
  };
}
