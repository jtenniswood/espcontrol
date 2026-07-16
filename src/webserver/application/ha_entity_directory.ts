import { state } from "../state/app_instance";
import { staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installHaEntityDirectoryModule(): GlobalDescriptors {
    // ── Home Assistant Entity Directory ────────────────────────────────────
    // Optional autocomplete source: with a long-lived access token saved in
    // this browser, the setup page fetches the full entity list straight from
    // Home Assistant's WebSocket API. Nothing is stored on the panel.
    var HA_URL_STORAGE_KEY: any = "espcontrol.haDirectoryUrl";
    var HA_TOKEN_STORAGE_KEY: any = "espcontrol.haDirectoryToken";
    var haFetchSocket: any = null;
    var haFetchTimer: any = null;
    function readStoredValue(this: any, key?: any) {
        if (typeof localStorage === "undefined")
            return "";
        try {
            return localStorage.getItem(key) || "";
        }
        catch (err) {
            return "";
        }
    }
    function writeStoredValue(this: any, key?: any, value?: any) {
        if (typeof localStorage === "undefined")
            return;
        try {
            if (value)
                localStorage.setItem(key, value);
            else
                localStorage.removeItem(key);
        }
        catch (err) {
            // Storage unavailable (e.g. private browsing) — feature stays off.
        }
    }
    function haDirectorySettings(this: any) {
        return {
            url: readStoredValue(HA_URL_STORAGE_KEY),
            token: readStoredValue(HA_TOKEN_STORAGE_KEY),
        };
    }
    function saveHaDirectorySettings(this: any, url?: any, token?: any) {
        writeStoredValue(HA_URL_STORAGE_KEY, String(url || "").trim());
        writeStoredValue(HA_TOKEN_STORAGE_KEY, String(token || "").trim());
    }
    function renderHaDirectoryStatus(this: any) {
        var el: any = els.haDirectoryStatus;
        if (!el)
            return;
        el.textContent = state.haDirectoryStatus;
        el.className = "sp-fw-status" +
            (state.haDirectoryStatusKind === "ok" ? " sp-update-available" : "") +
            (state.haDirectoryStatusKind === "error" ? " sp-update-error" : "");
        el.style.display = state.haDirectoryStatus ? "" : "none";
    }
    function setHaDirectoryStatus(this: any, kind?: any, text?: any) {
        state.haDirectoryStatusKind = kind || "";
        state.haDirectoryStatus = text || "";
        renderHaDirectoryStatus();
    }
    function closeHaFetchSocket(this: any) {
        if (haFetchTimer) {
            clearTimeout(haFetchTimer);
            haFetchTimer = null;
        }
        if (haFetchSocket) {
            var socket: any = haFetchSocket;
            haFetchSocket = null;
            try {
                socket.close();
            }
            catch (err) {
                // Ignore close failures on already-dead sockets.
            }
        }
    }
    function refreshHaEntityDirectory(this: any) {
        var cfg: any = haDirectorySettings();
        if (!cfg.token) {
            closeHaFetchSocket();
            state.haEntityNames = {};
            setHaDirectoryStatus("", "");
            return;
        }
        if (typeof WebSocket === "undefined")
            return;
        var wsUrl: any = haWebSocketUrl(cfg.url);
        if (!wsUrl) {
            setHaDirectoryStatus("error", "Invalid Home Assistant URL");
            return;
        }
        closeHaFetchSocket();
        setHaDirectoryStatus("pending", "Connecting to Home Assistant…");
        var socket: any;
        try {
            socket = new WebSocket(wsUrl);
        }
        catch (err) {
            setHaDirectoryStatus("error", "Invalid Home Assistant URL");
            return;
        }
        haFetchSocket = socket;
        var done: any = false;
        function finish(this: any, kind?: any, text?: any) {
            if (done || haFetchSocket !== socket)
                return;
            done = true;
            closeHaFetchSocket();
            setHaDirectoryStatus(kind, text);
        }
        haFetchTimer = setTimeout(function (this: any) {
            finish("error", "Could not reach Home Assistant at " + wsUrl.replace("/api/websocket", ""));
        }, 10000);
        socket.addEventListener("message", function (this: any, event?: any) {
            if (done || haFetchSocket !== socket)
                return;
            var msg: any;
            try {
                msg = JSON.parse(event.data);
            }
            catch (err) {
                return;
            }
            if (msg.type === "auth_required") {
                socket.send(JSON.stringify({ type: "auth", access_token: cfg.token }));
            }
            else if (msg.type === "auth_invalid") {
                finish("error", "Authentication failed — check the access token");
            }
            else if (msg.type === "auth_ok") {
                socket.send(JSON.stringify({ id: 1, type: "get_states" }));
            }
            else if (msg.type === "result" && msg.id === 1) {
                if (msg.success) {
                    state.haEntityNames = haEntityNamesFromStates(msg.result);
                    var count: any = Object.keys(state.haEntityNames).length;
                    finish("ok", "Connected — " + count + " " + (count === 1 ? "entity" : "entities"));
                }
                else {
                    finish("error", "Home Assistant rejected the entity request");
                }
            }
        });
        socket.addEventListener("error", function (this: any) {
            finish("error", "Could not reach Home Assistant — check the URL and that Home Assistant is reachable");
        });
        socket.addEventListener("close", function (this: any) {
            finish("error", "Could not reach Home Assistant — check the URL and that Home Assistant is reachable");
        });
    }
    return {
        "haDirectorySettings": staticGlobal(haDirectorySettings),
        "saveHaDirectorySettings": staticGlobal(saveHaDirectorySettings),
        "renderHaDirectoryStatus": staticGlobal(renderHaDirectoryStatus),
        "setHaDirectoryStatus": staticGlobal(setHaDirectoryStatus),
        "refreshHaEntityDirectory": staticGlobal(refreshHaEntityDirectory),
    };
}
