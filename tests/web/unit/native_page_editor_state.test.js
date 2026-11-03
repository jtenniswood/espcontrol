const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("native snapshots resist late legacy fragments and read-only pages remain browsable", () => {
  loadTypescriptTest("tests/web/native_page_editor_state.test.ts").runNativeEditorStateTests();
});

test("flat native snapshots own root order and accent while older readers retain legacy events", () => {
  loadTypescriptTest("tests/web/native_page_editor_state.test.ts").runNativeRootSettingsEventTests();
});
