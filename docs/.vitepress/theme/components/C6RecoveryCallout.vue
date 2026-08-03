<template>
  <div class="c6-callout">
    <strong>Having unreliable Wi‑Fi?</strong>
    <p>
      This P4 panel uses a separate ESP32-C6 Wi‑Fi processor. If it repeatedly
      disconnects, disappears from Home Assistant, or cannot finish initial setup, use
      the USB C6 recovery installer.
    </p>
    <div class="links">
      <a :href="primaryUrl">{{ primaryLabel }}</a>
      <a v-if="secondarySlug" :href="secondaryUrl">{{ secondaryLabel }}</a>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { withBase } from 'vitepress'

const props = defineProps({
  slug: { type: String, required: true },
  primaryLabel: { type: String, default: 'Open C6 recovery installer' },
  secondarySlug: { type: String, default: '' },
  secondaryLabel: { type: String, default: 'Open recovery for the other revision' },
})

const recoveryUrl = (slug) =>
  `${withBase('/getting-started/c6-recovery')}?device=${encodeURIComponent(slug)}`
const primaryUrl = computed(() => recoveryUrl(props.slug))
const secondaryUrl = computed(() => recoveryUrl(props.secondarySlug))
</script>

<style scoped>
.c6-callout {
  margin: 18px 0;
  padding: 16px;
  border-left: 4px solid var(--vp-c-warning-1);
  border-radius: 8px;
  background: var(--vp-c-bg-soft);
}

.c6-callout p {
  margin: 8px 0;
}

.links {
  display: flex;
  flex-wrap: wrap;
  gap: 8px 18px;
}
</style>
