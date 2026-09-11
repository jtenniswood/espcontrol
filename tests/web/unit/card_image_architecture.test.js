"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const root = path.resolve(__dirname, "../../..");
const source = file => fs.readFileSync(path.join(root, file), "utf8");

test("card images retain the generic server and typed web boundaries", () => {
  const server = source("components/web_server_idf/web_server_idf.cpp");
  assert.doesNotMatch(server, /card_asset|api\/card-images/, "generic server must not own product routes");
  assert.match(source("components/espcontrol/espcontrol_app.cpp"), /card_asset_http::register_endpoints/);
  assert.match(server, /r->method == HTTP_DELETE && espcontrol_allow_web_write != nullptr &&\s*!espcontrol_allow_web_write\(r\)/,
    "DELETE must receive the same reset preflight as raw POST and PUT");
  const post = server.slice(server.indexOf("esp_err_t AsyncWebServer::request_post_handler"));
  assert(post.indexOf("!espcontrol_allow_web_write(r)") < post.indexOf("handleRawRequest"),
    "streaming handlers must not bypass mutation preflight");
  const feature = source("src/webserver/features/card_images.ts");
  assert.doesNotMatch(feature, /\/api\/card-images|new Image\(|document\.createElement/,
    "backup features depend on injected transport and optimization");
});
