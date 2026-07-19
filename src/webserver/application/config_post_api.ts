import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installConfigPostApiModule(): GlobalDescriptors {
    // ── Config Post API ───────────────────────────────────────────────────
    function saveButtonConfig(this: any, slot?: any) {
        var b: any = state.buttons[slot - 1];
        postText(entityNameForSlot("button_config", slot), serializeButtonConfig(b));
    }
    function subpageEntityKeys(this: any) {
        var keys: any = ENTITY_CATALOG.groups.subpage_slot || [];
        var count: any = (CFG.features && CFG.features.subpageConfigChunks) || keys.length;
        count = Math.max(1, Math.min(keys.length, parseInt(count, 10) || keys.length));
        return keys.slice(0, count);
    }
    var SUBPAGE_RAW_CHUNK_FIELDS: any = ["main", "ext", "ext2", "ext3", "ext4", "ext5", "ext6", "ext7"];
    function subpageChunkShouldPost(this: any, slot?: any, keys?: any, chunks?: any, index?: any, previousPendingChunks?: any) {
        if (chunks[index] || index === 0)
            return true;
        var chunkName: any = entityNameForSlot(keys[index], slot);
        if (hasRememberedPostPath("text", chunkName, []))
            return true;
        var raw: any = state.subpageRaw[slot];
        var rawField: any = SUBPAGE_RAW_CHUNK_FIELDS[index];
        return !!((raw && rawField && raw[rawField]) ||
            (previousPendingChunks && previousPendingChunks[index]));
    }
    function saveSubpageEntity(this: any, slot?: any) {
        var sp: any = state.subpages[slot];
        var full: any = sp ? serializeSubpageConfig(sp) : "";
        var keys: any = subpageEntityKeys();
        var chunks: any = EspControlModel.splitSubpageConfigChunks(full, keys.length, 255);
        if (!chunks) {
            showBanner("Subpage is too large to save. Shorten labels or entity IDs.", "error");
            return;
        }
        var previousPendingChunks: any = EspControlModel.splitSubpageConfigChunks(state.subpageSavePending[slot] || "", keys.length, 255) || [];
        state.subpageSavePending[slot] = full;
        for (var ki: any = 0; ki < keys.length; ki++) {
            var chunkName: any = entityNameForSlot(keys[ki], slot);
            var chunk: any = chunks[ki] || "";
            if (!subpageChunkShouldPost(slot, keys, chunks, ki, previousPendingChunks))
                continue;
            postText(chunkName, chunk);
        }
    }
    function scheduleSliderSubpageMigration(this: any, slot?: any) {
        pendingSliderSubpageMigrations[slot] = true;
        clearTimeout(sliderMigrationTimer);
        sliderMigrationTimer = setTimeout(function (this: any) {
            var pending: any = pendingSliderSubpageMigrations;
            pendingSliderSubpageMigrations = {};
            for (var key in pending) {
                if (state.subpages[key])
                    saveSubpageEntity(key);
            }
        }, 5000);
    }
    return {
        "saveButtonConfig": staticGlobal(saveButtonConfig),
        "subpageEntityKeys": staticGlobal(subpageEntityKeys),
        "SUBPAGE_RAW_CHUNK_FIELDS": liveGlobal(() => SUBPAGE_RAW_CHUNK_FIELDS, (value?: any) => { SUBPAGE_RAW_CHUNK_FIELDS = value; }),
        "subpageChunkShouldPost": staticGlobal(subpageChunkShouldPost),
        "saveSubpageEntity": staticGlobal(saveSubpageEntity),
        "scheduleSliderSubpageMigration": staticGlobal(scheduleSliderSubpageMigration),
    };
}
