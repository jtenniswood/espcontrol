<script setup lang="ts">
import { computed, onUnmounted, ref } from 'vue'
import catalog from '../../../../product/v2/icons.json'
import categoryGroups from './icon-gallery-categories.json'

function searchKey(value: string) {
  return value.toLowerCase().replace(/[^a-z0-9]/g, '')
}

const categoryBySlug = new Map(
  Object.entries(categoryGroups).flatMap(([category, slugs]) =>
    slugs.map((slug) => [slug, category] as const),
  ),
)

const icons = catalog.icons
  .map(({ name, codepoint, mdi }) => ({
    name,
    category: categoryBySlug.get(mdi) ?? 'General',
    glyph: String.fromCodePoint(Number.parseInt(codepoint, 16)),
    searchKeys: [searchKey(name), searchKey(mdi.replace(/^material-symbol-/, ''))],
  }))
  .sort((first, second) => first.name.localeCompare(second.name, 'en'))

const query = ref('')
const selectedCategory = ref('')
const categories = [...new Set(icons.map(({ category }) => category))].sort()
const visibleIcons = computed(() => {
  const terms = query.value.split(/\s+/).map(searchKey).filter(Boolean)
  return icons.filter(({ category, searchKeys }) =>
    (!selectedCategory.value || category === selectedCategory.value) &&
    terms.every((term) => searchKeys.some((key) => key.includes(term))),
  )
})

const copyFeedback = ref<{ name: string; copied: boolean }>()
let feedbackTimer: ReturnType<typeof setTimeout> | undefined
let copyRequest = 0

function copyFromSelection(name: string) {
  const previousFocus = document.activeElement
  const field = document.createElement('textarea')
  field.value = name
  field.readOnly = true
  field.style.cssText = 'position:fixed;opacity:0;pointer-events:none'
  document.body.appendChild(field)
  try {
    field.focus({ preventScroll: true })
    field.select()
    return document.execCommand('copy')
  } catch {
    return false
  } finally {
    field.remove()
    if (previousFocus instanceof HTMLElement) {
      previousFocus.focus({ preventScroll: true })
    }
  }
}

async function copyIconName(name: string) {
  const request = ++copyRequest
  let copied = false
  if (navigator.clipboard) {
    try {
      await navigator.clipboard.writeText(name)
      copied = true
    } catch {
      // The selection fallback also works on the local HTTP preview.
    }
  }
  if (request !== copyRequest) return
  if (!copied) copied = copyFromSelection(name)
  clearTimeout(feedbackTimer)
  copyFeedback.value = { name, copied }
  feedbackTimer = setTimeout(() => { copyFeedback.value = undefined }, 2000)
}

onUnmounted(() => {
  copyRequest++
  clearTimeout(feedbackTimer)
})
</script>

<template>
  <div class="icon-gallery">
    <div class="icon-gallery__filters">
      <div class="icon-gallery__field">
        <span class="vpi-search icon-gallery__field-icon icon-gallery__search-icon" aria-hidden="true" />
        <input
          id="icon-gallery-search"
          v-model="query"
          type="search"
          aria-label="Search icons"
          placeholder="Try water leak or smart outlet"
          autocomplete="off"
          spellcheck="false"
        >
      </div>
      <div class="icon-gallery__field">
        <select id="icon-gallery-category" v-model="selectedCategory" aria-label="Category">
          <option value="">All categories</option>
          <option v-for="category in categories" :key="category" :value="category">
            {{ category }}
          </option>
        </select>
        <span class="vpi-chevron-down icon-gallery__field-icon icon-gallery__category-icon" aria-hidden="true" />
      </div>
    </div>
    <p class="visually-hidden" role="status">
      {{ visibleIcons.length }} of {{ icons.length }} icons
    </p>
    <p class="visually-hidden" role="status">
      <template v-if="copyFeedback">
        {{ copyFeedback.copied ? `Copied ${copyFeedback.name}` : `Could not copy ${copyFeedback.name}` }}
      </template>
    </p>
    <ul v-if="visibleIcons.length" class="icon-gallery__grid" aria-label="Available icons" role="list">
      <li v-for="icon in visibleIcons" :key="icon.name" class="icon-gallery__item">
        <button
          type="button"
          class="icon-gallery__tile"
          :aria-label="`Copy ${icon.name}`"
          @click="copyIconName(icon.name)"
        >
          <span class="icon-gallery__glyph" aria-hidden="true">{{ icon.glyph }}</span>
          <span v-if="copyFeedback?.name === icon.name">
            {{ copyFeedback.copied ? 'Copied!' : 'Copy failed' }}
          </span>
          <span v-else>{{ icon.name }}</span>
        </button>
      </li>
    </ul>
    <p v-else>No icons match these filters.</p>
  </div>
</template>

<style scoped>
@font-face {
  font-family: 'EspControl Icons';
  src: url('../../../../common/assets/fonts/espcontrol-icons.ttf') format('truetype');
  font-display: block;
}

.icon-gallery {
  margin-top: 1.5rem;
}

.icon-gallery__filters {
  display: grid;
  gap: 0.75rem;
}

.icon-gallery input,
.icon-gallery select {
  display: block;
  width: 100%;
  height: 3.25rem;
  padding: 0 0.75rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 0.5rem;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  font-size: 1.125rem;
  line-height: 1.5;
}

.icon-gallery select {
  appearance: none;
  padding-inline-end: 2.65rem;
  color: color-mix(in srgb, var(--vp-c-text-1) 72%, black);
  font-size: 1rem;
}

.icon-gallery__field {
  position: relative;
}

.icon-gallery__field-icon {
  position: absolute;
  inset-block: 0;
  margin-block: auto;
  color: var(--vp-c-text-2);
  font-size: 1.25rem;
  pointer-events: none;
}

.icon-gallery__search-icon {
  inset-inline-start: 0.85rem;
}

.icon-gallery__category-icon {
  inset-inline-end: 0.85rem;
}

.icon-gallery input {
  padding-inline-start: 2.65rem;
}

.icon-gallery input::placeholder {
  color: var(--vp-c-text-2);
}

.icon-gallery input:focus-visible,
.icon-gallery select:focus-visible,
.icon-gallery__tile:focus-visible {
  outline: 2px solid var(--vp-c-brand-1);
  outline-offset: 2px;
}

.icon-gallery__grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(min(100%, 7.5rem), 1fr));
  gap: 0.625rem;
  margin: 1rem 0 0;
  padding: 0;
  list-style: none;
}

.icon-gallery__item {
  display: grid;
}

.icon-gallery__tile {
  display: grid;
  grid-template-rows: 2rem minmax(2.5em, 1fr);
  place-items: center;
  gap: 0.5rem;
  min-height: 6.5rem;
  padding: 0.75rem 0.5rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 0.5rem;
  background: var(--vp-c-bg-soft);
  font-size: 0.875rem;
  line-height: 1.25;
  text-align: center;
  overflow-wrap: anywhere;
  cursor: pointer;
}

.icon-gallery__tile:hover {
  border-color: var(--vp-c-brand-1);
}

.icon-gallery__item + .icon-gallery__item {
  margin-top: 0;
}

.icon-gallery__glyph {
  font-family: 'EspControl Icons';
  font-size: 2rem;
  font-weight: normal;
  line-height: 1;
}

</style>
