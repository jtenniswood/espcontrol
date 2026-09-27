// Load only in the browser: ESP Web Tools registers custom elements at import time.
// package.json pins its esptool-js dependency to include the P4 v3.1/v3.2
// flash-power fix (espressif/esptool-js#268). Bundle it with the docs so all
// installer entry points use the same reviewed version, without a runtime CDN.
// Material Web 2.5.0 renames generated stylesheet files imported by ESP Web Tools
// 10.4.0; the Vite aliases in the docs config bridge those import paths.
export async function loadUsbInstaller() {
  await import('esp-web-tools/dist/install-button.js')
}
