<script setup lang="ts">
import { computed, ref } from 'vue'
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
</script>

<template>
  <div class="icon-gallery">
    <div class="icon-gallery__filters">
      <div>
        <label for="icon-gallery-search">Search icons</label>
        <input
          id="icon-gallery-search"
          v-model="query"
          type="search"
          placeholder="Try water leak or smart outlet"
          autocomplete="off"
          spellcheck="false"
        >
      </div>
      <div>
        <label for="icon-gallery-category">Category</label>
        <select id="icon-gallery-category" v-model="selectedCategory">
          <option value="">All categories</option>
          <option v-for="category in categories" :key="category" :value="category">
            {{ category }}
          </option>
        </select>
      </div>
    </div>
    <p class="icon-gallery__count" role="status">
      {{ visibleIcons.length }} of {{ icons.length }} icons
    </p>
    <ul v-if="visibleIcons.length" class="icon-gallery__grid" aria-label="Available icons" role="list">
      <li v-for="icon in visibleIcons" :key="icon.name" class="icon-gallery__item">
        <span class="icon-gallery__glyph" aria-hidden="true">{{ icon.glyph }}</span>
        <span>{{ icon.name }}</span>
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
  grid-template-columns: minmax(0, 3fr) minmax(12rem, 2fr);
  gap: 0.75rem;
  max-width: 40rem;
}

.icon-gallery label {
  display: block;
  font-weight: 600;
}

.icon-gallery input,
.icon-gallery select {
  width: 100%;
  min-height: 2.875rem;
  margin-top: 0.4rem;
  padding: 0.6rem 0.75rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 0.5rem;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
}

.icon-gallery select {
  appearance: auto;
}

.icon-gallery input::placeholder {
  color: var(--vp-c-text-2);
}

.icon-gallery input:focus-visible,
.icon-gallery select:focus-visible {
  outline: 2px solid var(--vp-c-brand-1);
  outline-offset: 2px;
}

.icon-gallery__count {
  color: var(--vp-c-text-2);
  font-size: 0.875rem;
}

.icon-gallery__grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(min(100%, 7.5rem), 1fr));
  gap: 0.625rem;
  margin: 0;
  padding: 0;
  list-style: none;
}

.icon-gallery__item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
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

@media (max-width: 560px) {
  .icon-gallery__filters {
    grid-template-columns: 1fr;
  }
}
</style>
