import type { HomeAssistantEntityPage, HomeAssistantEntityRecord } from "../model/entity_catalog";

import { CATALOG_FIELD_DOMAINS, CATALOG_PICKER_FIELDS, CATALOG_PICKER_GROUPS, CATALOG_PROTOCOL_VERSION, CATALOG_TRANSPORTS } from "../generated/ha_catalog_contract";

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
    const fields: Readonly<Record<string, string>> = CATALOG_PICKER_FIELDS;
    const normalized = new Set(domains);
    if (normalized.size > 1) {
        for (const field of CATALOG_PICKER_GROUPS) {
            const accepted: readonly string[] = CATALOG_FIELD_DOMAINS[field];
            if ([...normalized].every((domain) => accepted.includes(domain))) return field;
        }
    }
    const mapped = domains.map((domain) => fields[domain]).filter(Boolean);
    const commonField = mapped[0];
    if (normalized.size > 1 && mapped.length === domains.length &&
        commonField && mapped.every((field) => field === commonField)) {
        return commonField;
    }
    const first = domains[0];
    return domains.length === 1 && first ? fields[first] || "entity" : "entity";
}

const SEARCH_PATH = "/api/v1/ha/entities/search";
// Keep each native response below HA_ENTITY_CATALOG_MAX_BODY even when HA
// includes long names, areas, devices, states, and capability metadata.
const PAGE_LIMIT = CATALOG_TRANSPORTS.native.default_limit;
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
            if (page.protocol_version !== CATALOG_PROTOCOL_VERSION) {
                throw new Error("Home Assistant entity catalog uses an unsupported protocol version");
            }
            entities.push(...page.entities);
            if (page.next_cursor === null) {
                complete = true;
                break;
            }
            if (!Number.isInteger(page.next_cursor) || page.next_cursor <= cursor ||
                page.next_cursor > CATALOG_TRANSPORTS.native.max_cursor) {
                throw new Error("Home Assistant entity catalog returned invalid pagination");
            }
            cursor = page.next_cursor;
        }
        if (!complete) throw new Error("Home Assistant entity catalog exceeded the page limit");
        return entities;
    }
    return { search };
}
