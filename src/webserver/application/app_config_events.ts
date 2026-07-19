import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppConfigEventsModule(): GlobalDescriptors {
    // ── Config Event Handlers ─────────────────────────────────────────────
    function ensureSubpageRaw(this: any, slot?: any) {
        if (!state.subpageRaw[slot]) {
            state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        }
        return state.subpageRaw[slot];
    }
    function applyButtonConfigStateEvent(this: any, slot?: any, val?: any) {
        var b: any = state.buttons[slot - 1];
        var migrateConfig: any = buttonConfigNeedsMigration(val || "");
        var parsed: any = parseButtonConfig(val || "");
        b.entity = parsed.entity;
        b.label = parsed.label;
        b.icon = parsed.icon;
        b.icon_on = parsed.icon_on;
        b.sensor = parsed.sensor;
        b.unit = parsed.unit;
        b.type = parsed.type;
        b.precision = parsed.precision;
        b.options = parsed.options;
        if (migrateConfig)
            saveButtonConfig(slot);
        scheduleRender();
    }
    function applySubpageConfigStateEvent(this: any, slot?: any, key?: any, val?: any) {
        ensureSubpageRaw(slot)[key] = val || "";
        applySubpageRaw(slot);
    }
    function configEventPatterns(this: any) {
        return [
            {
                re: /^text-button_(\d+)_config$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applyButtonConfigStateEvent(slot, val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "main", val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config_ext$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "ext", val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config_ext_2$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "ext2", val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config_ext_3$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "ext3", val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config_ext_4$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "ext4", val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config_ext_5$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "ext5", val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config_ext_6$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "ext6", val);
                },
            },
            {
                re: /^text-subpage_(\d+)_config_ext_7$/,
                fn: function (this: any, m?: any, val?: any) {
                    var slot: any = parseInt(m[1], 10);
                    if (slot < 1 || slot > TOTAL_SLOTS)
                        return;
                    applySubpageConfigStateEvent(slot, "ext7", val);
                },
            },
        ];
    }
    return {
        "ensureSubpageRaw": staticGlobal(ensureSubpageRaw),
        "applyButtonConfigStateEvent": staticGlobal(applyButtonConfigStateEvent),
        "applySubpageConfigStateEvent": staticGlobal(applySubpageConfigStateEvent),
        "configEventPatterns": staticGlobal(configEventPatterns),
    };
}
