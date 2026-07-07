// ── Page title ─────────────────────────────────────────────────────────
// @web-module-requires: state_loader_api

function applyPageTitle(title) {
  var text = typeof title === "string" ? title.trim() : "";
  document.title = text || "EspControl";
}

function handleWebServerPingEvent(e) {
  var data = null;
  try {
    data = e && e.data ? JSON.parse(e.data) : null;
  } catch (_) {
    applyPageTitle("");
    return;
  }
  if (data && Object.prototype.hasOwnProperty.call(data, "title")) {
    applyPageTitle(data.title);
  }
}

function loadPageTitleFromEventStream() {
  if (eventStreamEnabled() || typeof EventSource !== "function") return;
  var source = new EventSource("/events");
  var closeTimer = setTimeout(function () {
    source.close();
  }, 5000);
  source.addEventListener("ping", function (e) {
    handleWebServerPingEvent(e);
    clearTimeout(closeTimer);
    source.close();
  });
  source.addEventListener("error", function () {
    clearTimeout(closeTimer);
    source.close();
  });
}
