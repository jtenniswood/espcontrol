/** Safe entity metadata returned by the EspControl Home Assistant integration. */
export interface HomeAssistantEntityRecord {
    entity_id: string;
    domain: string;
    device_id?: string | null;
    name: string;
    device_name?: string | null;
    area_name?: string | null;
    device_class?: string | null;
    icon?: string | null;
    unit?: string | null;
    state?: string;
    available: boolean;
    disabled: boolean;
    hidden: boolean;
    capabilities: string[];
}

export interface HomeAssistantEntityPage {
    protocol_version: number;
    entities: HomeAssistantEntityRecord[];
    next_cursor: number | null;
}
