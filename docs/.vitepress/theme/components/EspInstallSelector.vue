<template>
  <div class="esp-install-selector">
    <div class="device-list" role="listbox" aria-label="Target install device">
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
        <span
          class="device-screen"
          :class="device.shape"
          :style="{
            '--screen-aspect': device.aspect,
            '--grid-cols': device.cols,
            '--grid-rows': device.rows
          }"
          aria-hidden="true"
        >
          <span class="screen-grid">
            <span v-for="slot in device.slots" :key="slot"></span>
          </span>
        </span>
        <span class="device-copy">
          <span class="device-name">{{ device.size }}</span>
          <span class="device-meta">{{ device.name }} - {{ device.resolution }}</span>
          <span class="device-tags">
            <span>{{ device.orientation }}</span>
            <span>{{ device.slots }} buttons</span>
          </span>
        </span>
        <span class="device-check" aria-hidden="true"></span>
      </button>
    </div>

    <div class="installer-actions">
      <div v-if="!checked" class="installer-status">
        Preparing installer...
      </div>
      <div v-else-if="!supported" class="installer-status warning">
        Your browser does not support WebSerial. Use Chrome or Edge on desktop.
      </div>
      <div v-else-if="loadError" class="installer-status warning">
        Failed to load installer. {{ loadError }}
      </div>
      <div v-else-if="!ready" class="installer-status">
        Loading installer...
      </div>
      <esp-web-install-button
        v-else
        :key="selected.slug"
        :manifest="manifestUrl"
        class="install-button"
      >
        <button slot="activate" class="brand-button">
          Install Espcontrol
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
    name: 'JC8012P4A1',
    size: '10.1 in',
    resolution: '1280 x 800',
    orientation: 'Landscape',
    slots: 20,
    cols: 5,
    rows: 4,
    aspect: '1280 / 800',
    shape: 'landscape'
  },
  {
    slug: 'guition-esp32-p4-jc1060p470',
    name: 'JC1060P470',
    size: '7 in',
    resolution: '1024 x 600',
    orientation: 'Landscape',
    slots: 15,
    cols: 5,
    rows: 3,
    aspect: '1024 / 600',
    shape: 'landscape'
  },
  {
    slug: 'guition-esp32-p4-jc4880p443',
    name: 'JC4880P443',
    size: '4.3 in',
    resolution: '480 x 800',
    orientation: 'Portrait',
    slots: 6,
    cols: 2,
    rows: 3,
    aspect: '480 / 800',
    shape: 'portrait'
  },
  {
    slug: 'esp32-p4-86',
    name: 'ESP32-P4 86 Panel',
    size: '4 in',
    resolution: '720 x 720',
    orientation: 'Square',
    slots: 9,
    cols: 3,
    rows: 3,
    aspect: '1 / 1',
    shape: 'square'
  },
  {
    slug: 'guition-esp32-s3-4848s040',
    name: '4848S040',
    size: '4 in',
    resolution: '480 x 480',
    orientation: 'Square',
    slots: 9,
    cols: 3,
    rows: 3,
    aspect: '1 / 1',
    shape: 'square'
  }
]

const selected = ref(devices[0])
const checked = ref(false)
const supported = ref(false)
const ready = ref(false)
const loadError = ref(null)

const manifestUrl = computed(() => withBase(`/firmware/${selected.value.slug}/manifest.json`))

function selectDevice(device) {
  selected.value = device
}

onMounted(async () => {
  checked.value = true
  supported.value = 'serial' in navigator
  if (!supported.value) return

  try {
    await import('https://unpkg.com/esp-web-tools@10/dist/web/install-button.js')
    ready.value = true
  } catch (err) {
    loadError.value = err?.message || 'Network or script load error.'
  }
})
</script>

<style scoped>
.esp-install-selector {
  margin: 1.5rem 0;
}

.device-list {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

.device-card {
  position: relative;
  display: grid;
  grid-template-columns: 74px 1fr 22px;
  gap: 14px;
  align-items: center;
  width: 100%;
  min-height: 124px;
  padding: 14px;
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  color: var(--vp-c-text-1);
  background: var(--vp-c-bg-soft);
  text-align: left;
  cursor: pointer;
  transition:
    background-color 0.2s,
    border-color 0.2s,
    box-shadow 0.2s,
    transform 0.2s;
}

.device-card:hover {
  border-color: var(--vp-c-brand-2);
  transform: translateY(-1px);
}

.device-card:focus-visible {
  outline: 2px solid var(--vp-c-brand-1);
  outline-offset: 3px;
}

.device-card.selected {
  border-color: var(--vp-c-brand-1);
  background: var(--vp-c-brand-soft);
  box-shadow: 0 10px 28px rgba(0, 0, 0, 0.12);
}

.device-screen {
  display: grid;
  justify-self: center;
  width: 66px;
  aspect-ratio: var(--screen-aspect);
  padding: 5px;
  border: 2px solid color-mix(in srgb, var(--vp-c-text-1) 18%, transparent);
  border-radius: 7px;
  background: #121820;
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.06);
}

.device-screen.portrait {
  width: 44px;
}

.device-screen.square {
  width: 58px;
}

.screen-grid {
  display: grid;
  grid-template-columns: repeat(var(--grid-cols), 1fr);
  grid-template-rows: repeat(var(--grid-rows), 1fr);
  gap: 2px;
}

.screen-grid span {
  min-width: 0;
  border-radius: 2px;
  background: rgba(255, 255, 255, 0.18);
}

.device-copy {
  display: grid;
  gap: 5px;
  min-width: 0;
}

.device-name {
  font-size: 16px;
  font-weight: 700;
  line-height: 1.2;
}

.device-meta {
  color: var(--vp-c-text-2);
  font-size: 13px;
  line-height: 1.3;
}

.device-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  font-size: 12px;
  color: var(--vp-c-text-2);
}

.device-tags span {
  padding: 2px 7px;
  border: 1px solid var(--vp-c-divider);
  border-radius: 999px;
  background: var(--vp-c-bg);
}

.device-check {
  display: grid;
  place-items: center;
  width: 20px;
  height: 20px;
  border: 1px solid var(--vp-c-divider);
  border-radius: 50%;
}

.device-card.selected .device-check {
  border-color: var(--vp-c-brand-1);
  background: var(--vp-c-brand-1);
}

.device-card.selected .device-check::after {
  width: 7px;
  height: 11px;
  border: solid var(--vp-c-white);
  border-width: 0 2px 2px 0;
  content: "";
  transform: rotate(45deg) translate(-1px, -1px);
}

.installer-actions {
  margin-top: 16px;
}

.brand-button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 170px;
  border: 1px solid transparent;
  border-radius: 20px;
  padding: 0 20px;
  color: var(--vp-button-brand-text);
  background-color: var(--vp-button-brand-bg);
  font-size: 14px;
  font-weight: 600;
  line-height: 38px;
  white-space: nowrap;
  cursor: pointer;
  transition:
    color 0.25s,
    border-color 0.25s,
    background-color 0.25s;
}

.brand-button:hover {
  background-color: var(--vp-button-brand-hover-bg);
}

.installer-status {
  padding: 10px 14px;
  border-radius: 8px;
  background-color: var(--vp-c-default-soft);
  color: var(--vp-c-text-2);
  font-size: 14px;
}

.installer-status.warning {
  background-color: var(--vp-c-warning-soft);
  color: var(--vp-c-warning-1);
}

@media (max-width: 640px) {
  .device-list {
    grid-template-columns: 1fr;
  }

  .device-card {
    grid-template-columns: 64px 1fr 22px;
  }

  .brand-button {
    width: 100%;
  }
}
</style>
