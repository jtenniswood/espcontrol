import type { HomeAssistantEntityPage, HomeAssistantEntityRecord } from "../model/entity_catalog";

export interface EntityCatalogPairing {
    baseUrl: string;
    deviceId: string;
    token: string;
}

export interface EntityCatalogClient {
    pairing(): EntityCatalogPairing | null;
    savePairing(value: EntityCatalogPairing): void;
    importPairing(value: string): boolean;
    search(query: string, domains?: string[]): Promise<HomeAssistantEntityRecord[]>;
}

const STORAGE_KEY = "espcontrol.home-assistant-pairing";

function fieldForDomains(domains: string[]): string {
    const normalized = domains.slice().sort().join(",");
    const fields: Record<string, string> = {
        alarm_control_panel: "alarm",
        automation: "automation",
        binary_sensor: "sensor",
        button: "button",
        camera: "camera",
        climate: "climate",
        cover: "cover",
        fan: "fan",
        image: "camera",
        input_boolean: "switch",
        input_button: "button",
        input_number: "number",
        input_select: "select",
        lawn_mower: "lawn_mower",
        light: "light",
        lock: "lock",
        media_player: "media_player",
        number: "number",
        person: "person",
        scene: "scene",
        select: "select",
        script: "script",
        sensor: "sensor",
        switch: "switch",
        text_sensor: "sensor",
        vacuum: "vacuum",
        weather: "weather",
        device_tracker: "device_tracker",
    };
    const first = domains[0];
    if (domains.length === 1 && first && fields[first]) return fields[first];
    if (normalized === "") return "entity";
    return "entity";
}

export function createEntityCatalogClient(
    storage: Storage | undefined = typeof localStorage === "undefined" ? undefined : localStorage,
    fetchImpl: typeof fetch = fetch,
): EntityCatalogClient {
    let cached: EntityCatalogPairing | null = null;
    function pairing(): EntityCatalogPairing | null {
        if (cached) return cached;
        if (!storage) return null;
        try {
            const parsed = JSON.parse(storage.getItem(STORAGE_KEY) || "{}");
            if (!parsed || typeof parsed.baseUrl !== "string" || typeof parsed.deviceId !== "string" || typeof parsed.token !== "string") return null;
            cached = parsed as EntityCatalogPairing;
            return cached;
        } catch (_) {
            return null;
        }
    }
    function savePairing(value: EntityCatalogPairing): void {
        cached = { baseUrl: value.baseUrl.replace(/\/$/, ""), deviceId: value.deviceId, token: value.token };
        try { storage?.setItem(STORAGE_KEY, JSON.stringify(cached)); } catch (_) { /* memory cache still works */ }
    }
    function importPairing(value: string): boolean {
        try {
            const normalized = value.replace(/-/g, "+").replace(/_/g, "/").padEnd(Math.ceil(value.length / 4) * 4, "=");
            const json = atob(normalized);
            const parsed = JSON.parse(json) as EntityCatalogPairing;
            if (!parsed || typeof parsed.baseUrl !== "string" || typeof parsed.deviceId !== "string" || typeof parsed.token !== "string") return false;
            savePairing(parsed);
            return true;
        } catch (_) {
            return false;
        }
    }
    async function search(query: string, domains: string[] = []): Promise<HomeAssistantEntityRecord[]> {
        const active = pairing();
        if (!active) return [];
        const params = new URLSearchParams({ q: query, field: fieldForDomains(domains), limit: "50" });
        const response = await fetchImpl(`${active.baseUrl}/api/espcontrol/${encodeURIComponent(active.deviceId)}/entities?${params}`, {
            headers: { Authorization: `Bearer ${active.token}` },
            credentials: "omit",
            cache: "no-store",
        });
        if (response.status === 401) {
            cached = null;
            try { storage?.removeItem(STORAGE_KEY); } catch (_) { /* storage is optional */ }
            return [];
        }
        if (!response.ok) return [];
        const page = await response.json() as HomeAssistantEntityPage;
        return Array.isArray(page.entities) ? page.entities : [];
    }
    return { pairing, savePairing, importPairing, search };
}
