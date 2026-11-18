<script setup lang="ts">
import { computed, ref } from 'vue'
import catalog from '../../../../product/v2/icons.json'

function searchKey(value: string) {
  return value.toLowerCase().replace(/[^a-z0-9]/g, '')
}

const icons = catalog.icons
  .map(({ name, codepoint, mdi }) => ({
    name,
    glyph: String.fromCodePoint(Number.parseInt(codepoint, 16)),
    searchKeys: [searchKey(name), searchKey(mdi.replace(/^material-symbol-/, ''))],
  }))
  .sort((first, second) => first.name.localeCompare(second.name, 'en'))

const query = ref('')
const visibleIcons = computed(() => {
  const terms = query.value.split(/\s+/).map(searchKey).filter(Boolean)
  return icons.filter(({ searchKeys }) => terms.every((term) =>
    searchKeys.some((key) => key.includes(term)),
  ))
})
</script>

<template>
  <div class="icon-gallery">
    <label for="icon-gallery-search">Search icons</label>
    <input
      id="icon-gallery-search"
      v-model="query"
      type="search"
      placeholder="Try water leak or smart outlet"
      autocomplete="off"
      spellcheck="false"
    >
    <p class="icon-gallery__count" role="status">
      {{ visibleIcons.length }} of {{ icons.length }} icons
    </p>
    <ul v-if="visibleIcons.length" class="icon-gallery__grid" aria-label="Available icons" role="list">
      <li v-for="icon in visibleIcons" :key="icon.name" class="icon-gallery__item">
        <span class="icon-gallery__glyph" aria-hidden="true">{{ icon.glyph }}</span>
        <span>{{ icon.name }}</span>
      </li>
    </ul>
    <p v-else>No icons match your search.</p>
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

.icon-gallery label {
  display: block;
  font-weight: 600;
}

.icon-gallery input {
  width: 100%;
  max-width: 28rem;
  margin-top: 0.4rem;
  padding: 0.6rem 0.75rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 0.5rem;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
}

.icon-gallery input::placeholder {
  color: var(--vp-c-text-2);
}

.icon-gallery input:focus-visible {
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
</style>
