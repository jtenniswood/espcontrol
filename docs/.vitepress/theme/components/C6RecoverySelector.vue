<template>
  <div class="recovery-selector">
    <div class="device-list" role="listbox" aria-label="P4 recovery device">
      <button
        v-for="device in devices"
        :key="device.slug"
        type="button"
        class="device-card"
        :class="{ selected: selected.slug === device.slug }"
        role="option"
        :aria-selected="selected.slug === device.slug"
        @click="selectDevice(device)"
      >
        <span class="device-name">{{ device.size }}</span>
        <span class="device-copy">
          <strong>{{ device.name }}</strong>
          <small>{{ device.detail }}</small>
        </span>
        <span class="device-check" aria-hidden="true">{{ selected.slug === device.slug ? '✓' : '' }}</span>
      </button>
    </div>

    <div class="selection-warning">
      <strong>Selected: {{ selected.name }}</strong>
      <span>{{ selected.warning }}</span>
    </div>

    <label class="confirmation">
      <input v-model="confirmed" type="checkbox">
      I have checked that this is my exact panel model and revision.
    </label>

    <div class="installer-actions">
      <div v-if="!checked" class="installer-status">Preparing recovery installer...</div>
      <div v-else-if="!supported" class="installer-status warning">
        USB installation requires Chrome or Edge on a desktop computer.
      </div>
      <div v-else-if="loadError" class="installer-status warning">
        {{ loadError }}
      </div>
      <div v-else-if="checkingManifest" class="installer-status">
        Checking the latest recovery image...
      </div>
      <div v-else-if="!manifestAvailable" class="installer-status warning">
        The recovery image for this panel is not published yet. Check again after the next
        EspControl release.
      </div>
      <esp-web-install-button
        v-else
        :key="selected.slug"
        :manifest="manifestUrl"
        class="install-button"
      >
        <button slot="activate" class="brand-button" :disabled="!confirmed">
          Repair C6 and reinstall EspControl
        </button>
      </esp-web-install-button>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'
import { withBase } from 'vitepress'

const devices = [
  {
    slug: 'guition-esp32-p4-jc8012p4a1',
    name: 'JC8012P4A1 original panel',
    size: '10.1 in',
    detail: 'Rear case 2622 or lower',
    warning: 'Use this only for rear case number 2622 or lower.',
  },
  {
    slug: 'guition-esp32-p4-jc8012p4a1-v2',
    name: 'JC8012P4A1 new panel',
    size: '10.1 in',
    detail: 'Rear case 2624 or higher',
    warning: 'Use this only for rear case number 2624 or higher.',
  },
  {
    slug: 'guition-esp32-p4-jc1060p470',
    name: 'JC1060P470',
    size: '7 in',
    detail: '1024 × 600 landscape',
    warning: 'Confirm the back label says JC1060P470.',
  },
  {
    slug: 'guition-esp32-p4-jc4880p443',
    name: 'JC4880P443',
    size: '4.3 in',
    detail: '480 × 800 portrait',
    warning: 'Confirm the back label says JC4880P443.',
  },
  {
    slug: 'esp32-p4-86',
    name: 'ESP32-P4 86 Panel',
    size: '4 in',
    detail: '720 × 720 square, ETH-2RO',
    warning: 'Use this for the ESP32-P4-86-Panel-ETH-2RO model.',
  },
]

const selected = ref(devices[0])
const confirmed = ref(false)
const checked = ref(false)
const supported = ref(false)
const manifestAvailable = ref(false)
const checkingManifest = ref(false)
const loadError = ref('')
let manifestRequest = 0

const manifestUrl = computed(() =>
  withBase(`/firmware/${selected.value.slug}/recovery/manifest.json`),
)

async function checkManifest() {
  const request = ++manifestRequest
  checkingManifest.value = true
  manifestAvailable.value = false
  try {
    const response = await fetch(manifestUrl.value, { cache: 'no-store' })
    if (request === manifestRequest) manifestAvailable.value = response.ok
  } catch {
    if (request === manifestRequest) manifestAvailable.value = false
  } finally {
    if (request === manifestRequest) checkingManifest.value = false
  }
}

function selectDevice(device) {
  selected.value = device
  confirmed.value = false
  const url = new URL(window.location.href)
  url.searchParams.set('device', device.slug)
  window.history.replaceState({}, '', url)
  checkManifest()
}

onMounted(async () => {
  const requested = new URLSearchParams(window.location.search).get('device')
  const matched = devices.find((device) => device.slug === requested)
  if (matched) selected.value = matched

  checked.value = true
  supported.value = 'serial' in navigator
  if (!supported.value) return

  try {
    await import('https://unpkg.com/esp-web-tools@10/dist/web/install-button.js')
    await checkManifest()
  } catch (error) {
    loadError.value = `Failed to load the USB installer. ${error?.message || ''}`.trim()
  }
})
</script>

<style scoped>
.recovery-selector {
  margin: 1.5rem 0;
}

.device-list {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
  gap: 10px;
}

.device-card {
  display: grid;
  grid-template-columns: 56px 1fr 24px;
  gap: 12px;
  align-items: center;
  min-height: 82px;
  padding: 13px;
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  color: var(--vp-c-text-1);
  background: var(--vp-c-bg-soft);
  text-align: left;
  cursor: pointer;
}

.device-card.selected {
  border-color: var(--vp-c-brand-1);
  box-shadow: 0 0 0 1px var(--vp-c-brand-1);
}

.device-name {
  font-weight: 700;
  color: var(--vp-c-brand-1);
}

.device-copy {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.device-copy small,
.selection-warning span {
  color: var(--vp-c-text-2);
}

.device-check {
  color: var(--vp-c-brand-1);
  font-size: 20px;
}

.selection-warning,
.confirmation {
  display: flex;
  gap: 8px;
  margin-top: 14px;
  padding: 12px;
  border-radius: 8px;
  background: var(--vp-c-bg-soft);
}

.selection-warning {
  flex-direction: column;
}

.confirmation {
  align-items: center;
}

.installer-actions {
  margin-top: 16px;
}

.brand-button {
  padding: 12px 20px;
  border: 0;
  border-radius: 8px;
  color: var(--vp-button-brand-text);
  background: var(--vp-button-brand-bg);
  font-weight: 600;
  cursor: pointer;
}

.brand-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.installer-status {
  padding: 12px;
  border-radius: 8px;
  background: var(--vp-c-bg-soft);
}

.installer-status.warning {
  color: var(--vp-c-warning-1);
}
</style>
