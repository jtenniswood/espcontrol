<template>
  <div class="esp-install-wrapper">
    <div v-if="!checked" class="installer-status">
      Preparing installer...
    </div>
    <div v-else-if="!supported" class="installer-status warning">
      Your browser does not support WebSerial. Use Chrome or Edge on desktop.
    </div>
    <div v-else-if="loadError" class="installer-status warning">
      {{ loadError }}
    </div>
    <div v-else-if="checkingManifest" class="installer-status">
      Checking the latest firmware...
    </div>
    <div v-else-if="!manifestAvailable" class="installer-status warning">
      WebInstall firmware for this panel has not been published yet. Use the manual ESPHome
      setup below or check again after the next EspControl release.
    </div>
    <div v-else-if="!ready" class="installer-status">
      Loading installer...
    </div>
    <div v-else class="install-button">
      <esp-web-install-button :manifest="manifestUrl">
        <button slot="activate" class="brand-button">Install Espcontrol</button>
      </esp-web-install-button>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { withBase } from 'vitepress'

const props = defineProps({
  slug: { type: String, default: 'guition-esp32-p4-jc1060p470' }
})
const manifestUrl = withBase(`/firmware/${props.slug}/manifest.json`)
const checked = ref(false)
const supported = ref(false)
const checkingManifest = ref(false)
const manifestAvailable = ref(false)
const ready = ref(false)
const loadError = ref('')

onMounted(async () => {
  checked.value = true
  supported.value = 'serial' in navigator
  if (!supported.value) return

  checkingManifest.value = true
  try {
    const response = await fetch(manifestUrl, { cache: 'no-store' })
    manifestAvailable.value = response.ok
  } catch {
    manifestAvailable.value = false
  } finally {
    checkingManifest.value = false
  }

  if (!manifestAvailable.value) return

  try {
    await import('https://unpkg.com/esp-web-tools@10/dist/web/install-button.js')
    ready.value = true
  } catch (err) {
    loadError.value = `Failed to load the USB installer. ${err?.message || ''}`.trim()
  }
})
</script>

<style scoped>
.esp-install-wrapper {
  margin: 1.5rem 0;
}

.brand-button {
  display: inline-block;
  border: 1px solid transparent;
  text-align: center;
  font-weight: 600;
  white-space: nowrap;
  transition: color 0.25s, border-color 0.25s, background-color 0.25s;
  border-radius: 20px;
  padding: 0 20px;
  line-height: 38px;
  font-size: 14px;
  color: var(--vp-button-brand-text);
  background-color: var(--vp-button-brand-bg);
  cursor: pointer;
}

.brand-button:hover {
  background-color: var(--vp-button-brand-hover-bg);
}

.installer-status {
  padding: 12px 16px;
  border-radius: 8px;
  background-color: var(--vp-c-default-soft);
  color: var(--vp-c-text-2);
  font-size: 14px;
}

.installer-status.warning {
  background-color: var(--vp-c-warning-soft);
  color: var(--vp-c-warning-1);
}
</style>
