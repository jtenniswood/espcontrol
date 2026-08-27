import { normalizeCoverArtDelay, normalizeHomeAssistantArtworkEndpointMode, normalizeHomeAssistantArtworkProtocol } from "../model/settings";
import type { EntityStateFeature } from "./entity_state";
import type { ApplicationApiFeature } from "./api";
export interface ArtworkPostApiFeature {
    postPresenceSensorEntity(value?: any): any;
    postMediaPlayerSleepPrevention(on?: any): any;
    postMediaPlayerSleepPreventionEntity(value?: any): any;
    postCoverArtScreensaver(on?: any): any;
    postCoverArtMediaPlayerEntity(value?: any): any;
    postCoverArtSecondaryMediaPlayerEntity(value?: any): any;
    postCoverArtConditions(value?: any): any;
    coverArtHideExternalInputPostUrls(on?: any): any;
    postCoverArtHideExternalInput(on?: any): any;
    coverArtDelayPostUrls(value?: any): any;
    postCoverArtDelay(value?: any): any;
    coverArtTrackOverlayDurationPostUrls(value?: any): any;
    postCoverArtTrackOverlayDuration(value?: any): any;
    homeAssistantArtworkPortPostUrls(value?: any): any;
    postHomeAssistantArtworkPort(value?: any): any;
    postHomeAssistantArtworkProtocol(value?: any): any;
    postHomeAssistantArtworkEndpointMode(value?: any): any;
}

export function createArtworkPostApiFeature(
    entityState: Pick<EntityStateFeature, "entityName" | "entityObjectIds" | "entityPostUrls">,
    requestApi: Pick<ApplicationApiFeature, "post" | "postTextWithObjectIds" | "postSwitchWithObjectIds" | "postSelectWithObjectIds">,
): ArtworkPostApiFeature {
    const { entityName, entityObjectIds, entityPostUrls } = entityState;
    const { post, postTextWithObjectIds, postSwitchWithObjectIds, postSelectWithObjectIds } = requestApi;
    // ── Artwork Post API ──────────────────────────────────────────────────
    function postPresenceSensorEntity(this: any, value?: any) {
        return postTextWithObjectIds(entityName("presence_sensor_entity"), entityObjectIds("presence_sensor_entity"), value);
    }
    function postMediaPlayerSleepPrevention(this: any, on?: any) {
        return postSwitchWithObjectIds(entityName("screen_saver_media_player_sleep_prevention"), entityObjectIds("screen_saver_media_player_sleep_prevention"), on);
    }
    function postMediaPlayerSleepPreventionEntity(this: any, value?: any) {
        return postTextWithObjectIds(entityName("media_player_sleep_prevention_entity"), entityObjectIds("media_player_sleep_prevention_entity"), value);
    }
    function postCoverArtScreensaver(this: any, on?: any) {
        return postSwitchWithObjectIds(entityName("screen_saver_cover_art"), entityObjectIds("screen_saver_cover_art"), on);
    }
    function postCoverArtMediaPlayerEntity(this: any, value?: any) {
        return postTextWithObjectIds(entityName("screen_saver_cover_art_entity"), entityObjectIds("screen_saver_cover_art_entity"), value);
    }
    function postCoverArtSecondaryMediaPlayerEntity(this: any, value?: any) {
        return postTextWithObjectIds(entityName("screen_saver_cover_art_secondary_entity"), entityObjectIds("screen_saver_cover_art_secondary_entity"), value);
    }
    function postCoverArtConditions(this: any, value?: any) {
        return postTextWithObjectIds(entityName("screen_saver_cover_art_conditions"), entityObjectIds("screen_saver_cover_art_conditions"), value);
    }
    function coverArtHideExternalInputPostUrls(this: any, on?: any) {
        return entityPostUrls("switch", entityName("screen_saver_hide_cover_art_external_input"), entityObjectIds("screen_saver_hide_cover_art_external_input"), on ? "turn_on" : "turn_off");
    }
    function postCoverArtHideExternalInput(this: any, on?: any) {
        return post(coverArtHideExternalInputPostUrls(on));
    }
    function coverArtDelayPostUrls(this: any, value?: any) {
        return entityPostUrls("number", entityName("screen_saver_cover_art_delay"), entityObjectIds("screen_saver_cover_art_delay"), "set?value=" + encodeURIComponent(normalizeCoverArtDelay(value)));
    }
    function postCoverArtDelay(this: any, value?: any) {
        return post(coverArtDelayPostUrls(value));
    }
    function coverArtTrackOverlayDurationPostUrls(this: any, value?: any) {
        return entityPostUrls("number", entityName("screen_saver_track_overlay_duration"), entityObjectIds("screen_saver_track_overlay_duration"), "set?value=" + encodeURIComponent(value));
    }
    function postCoverArtTrackOverlayDuration(this: any, value?: any) {
        return post(coverArtTrackOverlayDurationPostUrls(value));
    }
    function homeAssistantArtworkPortPostUrls(this: any, value?: any) {
        return entityPostUrls("number", entityName("home_assistant_artwork_port"), entityObjectIds("home_assistant_artwork_port"), "set?value=" + encodeURIComponent(value));
    }
    function postHomeAssistantArtworkPort(this: any, value?: any) {
        return post(homeAssistantArtworkPortPostUrls(value));
    }
    function postHomeAssistantArtworkProtocol(this: any, value?: any) {
        return postSelectWithObjectIds(entityName("home_assistant_artwork_protocol"), entityObjectIds("home_assistant_artwork_protocol"), normalizeHomeAssistantArtworkProtocol(value));
    }
    function postHomeAssistantArtworkEndpointMode(this: any, value?: any) {
        return postSelectWithObjectIds(entityName("home_assistant_artwork_endpoint_mode"), entityObjectIds("home_assistant_artwork_endpoint_mode"), normalizeHomeAssistantArtworkEndpointMode(value));
    }
    return {
        postPresenceSensorEntity,
        postMediaPlayerSleepPrevention,
        postMediaPlayerSleepPreventionEntity,
        postCoverArtScreensaver,
        postCoverArtMediaPlayerEntity,
        postCoverArtSecondaryMediaPlayerEntity,
        postCoverArtConditions,
        coverArtHideExternalInputPostUrls,
        postCoverArtHideExternalInput,
        coverArtDelayPostUrls,
        postCoverArtDelay,
        coverArtTrackOverlayDurationPostUrls,
        postCoverArtTrackOverlayDuration,
        homeAssistantArtworkPortPostUrls,
        postHomeAssistantArtworkPort,
        postHomeAssistantArtworkProtocol,
        postHomeAssistantArtworkEndpointMode,
    };
}
