import type { HomeAssistantEntityPage, HomeAssistantEntityRecord } from "../model/entity_catalog";

export interface EntityCatalogClient {
    /** Search the HA catalog through the display's native ESPHome connection. */
    search(
        query: string,
        domains?: string[],
        options?: EntityCatalogSearchOptions,
    ): Promise<HomeAssistantEntityRecord[]>;
}

export interface EntityCatalogSearchOptions {
    area?: string;
    deviceId?: string;
    includeHidden?: boolean;
    includeDisabled?: boolean;
    capabilities?: string[];
}

function fieldForDomains(domains: string[]): string {
    const fields: Record<string, string> = {
        alarm_control_panel: "alarm",
        automation: "automation",
        binary_sensor: "binary_sensor",
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
        text_sensor: "sensor",
        switch: "switch",
        vacuum: "vacuum",
        weather: "weather",
        device_tracker: "device_tracker",
    };
    const normalized = new Set(domains);
    const sensorDomains = new Set(["sensor", "binary_sensor", "text_sensor", "input_number"]);
    if (normalized.size > 1 && [...normalized].every((domain) => sensorDomains.has(domain))) {
        return "sensor";
    }
    const actionDomains = new Set([
        "scene", "script", "automation", "button", "input_button", "input_boolean",
        "number", "input_number", "select", "input_select",
    ]);
    if (normalized.size > 1 && [...normalized].every((domain) => actionDomains.has(domain))) {
        return "action";
    }
    const mapped = domains.map((domain) => fields[domain]).filter(Boolean);
    const commonField = mapped[0];
    if (normalized.size > 1 && mapped.length === domains.length &&
        commonField && mapped.every((field) => field === commonField)) {
        return commonField;
    }
    const first = domains[0];
    if (domains.length === 1 && first && fields[first]) return fields[first];
    return "entity";
}

const SEARCH_PATH = "/api/v1/ha/entities/search";
// Keep each native response below HA_ENTITY_CATALOG_MAX_BODY even when HA
// includes long names, areas, devices, states, and capability metadata.
const PAGE_LIMIT = 25;
const POLL_DELAY_MS = 100;
const MAX_POLLS = 150;

interface PendingCatalogResponse {
    status?: string;
    request_id?: number;
    error?: string;
}

function wait(milliseconds: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

export function createEntityCatalogClient(
    _storage?: Storage,
    fetchImpl: typeof fetch = fetch,
): EntityCatalogClient {
    async function search(
        query: string,
        domains: string[] = [],
        options: EntityCatalogSearchOptions = {},
    ): Promise<HomeAssistantEntityRecord[]> {
        const entities: HomeAssistantEntityRecord[] = [];
        let cursor = 0;
        let complete = false;
        for (let pageNumber = 0; pageNumber < 200; pageNumber += 1) {
            const params = new URLSearchParams({
                query,
                field: fieldForDomains(domains),
                limit: String(PAGE_LIMIT),
                cursor: String(cursor),
            });
            if (options.area) params.set("area", options.area);
            if (options.deviceId) params.set("device_id", options.deviceId);
            if (options.includeHidden) params.set("include_hidden", "1");
            if (options.includeDisabled) params.set("include_disabled", "1");
            if (options.capabilities?.length) params.set("capabilities", options.capabilities.join(","));
            const start = await fetchImpl(`${SEARCH_PATH}?${params}`, {
                credentials: "same-origin",
                cache: "no-store",
            });
            const pending = await start.json().catch(() => ({})) as PendingCatalogResponse;
            // Older firmware used 202 for this response, while the ESP-IDF
            // web-server adapter can surface that as 500. The JSON state is
            // authoritative, so accept a pending response from either form.
            if (!start.ok && start.status !== 202 && pending.status !== "pending") {
                throw new Error(pending.error || `Entity catalog request failed (${start.status})`);
            }
            if (typeof pending.request_id !== "number") {
                throw new Error(pending.error || "Entity catalog did not return a request ID");
            }
            let page: HomeAssistantEntityPage | null = null;
            for (let poll = 0; poll < MAX_POLLS; poll += 1) {
                await wait(POLL_DELAY_MS);
                const response = await fetchImpl(`${SEARCH_PATH}?request_id=${pending.request_id}`, {
                    credentials: "same-origin",
                    cache: "no-store",
                });
                const payload = await response.json().catch(() => ({})) as PendingCatalogResponse &
                    Partial<HomeAssistantEntityPage> & { error?: string };
                if (response.status === 202 || payload.status === "pending") continue;
                if (!response.ok) {
                    throw new Error(payload.error || `Entity catalog request failed (${response.status})`);
                }
                page = payload as HomeAssistantEntityPage;
                break;
            }
            if (!page || !Array.isArray(page.entities)) throw new Error("Home Assistant entity catalog timed out");
            entities.push(...page.entities);
            if (page.next_cursor === null || typeof page.next_cursor !== "number" || page.next_cursor <= cursor) {
                complete = true;
                break;
            }
            cursor = page.next_cursor;
        }
        if (!complete) throw new Error("Home Assistant entity catalog exceeded the page limit");
        return entities;
    }
    return { search };
}
