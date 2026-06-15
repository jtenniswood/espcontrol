<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'

const repoUrl = 'https://github.com/jtenniswood/espcontrol'
const apiUrl = 'https://api.github.com/repos/jtenniswood/espcontrol'
const stars = ref<number | null>(null)

const starLabel = computed(() => {
  if (stars.value == null) return '...'
  return new Intl.NumberFormat('en', { notation: 'compact' }).format(stars.value)
})

onMounted(async () => {
  try {
    const response = await fetch(apiUrl, {
      headers: { Accept: 'application/vnd.github+json' },
    })
    if (!response.ok) return
    const repo = (await response.json()) as { stargazers_count?: number }
    if (typeof repo.stargazers_count === 'number') {
      stars.value = repo.stargazers_count
    }
  } catch {
    stars.value = null
  }
})
</script>

<template>
  <a
    class="github-stars"
    :href="repoUrl"
    target="_blank"
    rel="noopener"
    aria-label="Star EspControl on GitHub"
    title="Star EspControl on GitHub"
  >
    <svg class="github-stars__icon" aria-hidden="true" viewBox="0 0 24 24">
      <path
        d="m12 2.5 2.9 5.9 6.5.9-4.7 4.6 1.1 6.5L12 17.3l-5.8 3.1 1.1-6.5-4.7-4.6 6.5-.9L12 2.5Z"
      />
    </svg>
    <span class="github-stars__text">Star</span>
    <span class="github-stars__count">{{ starLabel }}</span>
  </a>
</template>
