import { mkdir, writeFile } from 'node:fs/promises'
import { join } from 'node:path'
import type { MarkdownRenderer } from 'vitepress'

export const hostname = 'https://jtenniswood.github.io/espcontrol/'

// GitHub Pages cannot configure HTTP redirects. These HTML fallbacks preserve
// published links while keeping retired pages out of search and the sitemap.
export const redirects: Record<string, string> = {
  'features/backlight': `${hostname}settings/display#backlight`,
  'features/rotation': `${hostname}settings/display#rotation`,
  'features/language': `${hostname}settings/preferences#language`,
  'features/clock': `${hostname}settings/preferences#time`,
  'features/temperature': `${hostname}settings/preferences#temperature`,
  'features/device-name': `${hostname}settings/maintenance#device-name`,
  'features/backup': `${hostname}settings/maintenance#backup`,
  'features/firmware-updates': `${hostname}settings/maintenance#firmware`,
  'features/home-assistant': `${hostname}settings/maintenance#home-assistant-settings`,
  'features/battery': `${hostname}settings/maintenance#battery`,
  'features/restart': `${hostname}settings/maintenance#restart`,
  'features/media-cover-art': `${hostname}settings/sleep#cover-art-screen-saver`,
  'features/screensaver': `${hostname}settings/sleep#screensaver`,
  'features/screen-schedule': `${hostname}settings/sleep#night-schedule`,
  'settings/active-colour': `${hostname}features/appearance#active-colour`,
  'settings/card-style': `${hostname}features/appearance#card-style`,
  'settings/theme': `${hostname}features/appearance#theme`,
  'features/idle': `${hostname}features/appearance#idle`,
  'features/clock-bar': `${hostname}features/appearance#clock-bar`,
  'reference/card-capabilities': `${hostname}card-types/`,
  'reference/request-device-support': `${hostname}screens/`,
  'card-types/weather-forecast': `${hostname}card-types/weather#setting-up-a-weather-card`,
  'generated/cards/capabilities': `${hostname}card-types/`,
  'generated/cards/runtime-coverage':
    `${hostname}card-types/`,
  'generated/screens/4848s040-grid': `${hostname}screens/4848s040#card-grid`,
  'generated/screens/4848s040-install': `${hostname}screens/4848s040#install`,
  'generated/screens/jc1060p470-grid': `${hostname}screens/jc1060p470#card-grid`,
  'generated/screens/jc1060p470-install': `${hostname}screens/jc1060p470-v1#install`,
  'generated/screens/jc1060p470-v2-grid': `${hostname}screens/jc1060p470#card-grid`,
  'generated/screens/jc1060p470-v2-install': `${hostname}screens/jc1060p470-v2#install`,
  'generated/screens/jc4880p443-grid': `${hostname}screens/jc4880p443#card-grid`,
  'generated/screens/jc4880p443-v3-grid': `${hostname}screens/jc4880p443#card-grid`,
  'generated/screens/jc4880p443-v3-install': `${hostname}screens/jc4880p443-v3#install`,
  'generated/screens/jc4880p443-install': `${hostname}screens/jc4880p443#install`,
  'generated/screens/jc8012p4a1-grid': `${hostname}screens/jc8012p4a1`,
  'generated/screens/jc8012p4a1-install': `${hostname}screens/jc8012p4a1-v1#install`,
  'generated/screens/jc8012p4a1-v2-grid': `${hostname}screens/jc8012p4a1`,
  'generated/screens/jc8012p4a1-v2-install': `${hostname}screens/jc8012p4a1-v2#install`,
  'generated/screens/jc8012p4a1-v3-grid': `${hostname}screens/jc8012p4a1`,
  'generated/screens/jc8012p4a1-v3-install': `${hostname}screens/jc8012p4a1-v3#install`,
  'generated/screens/p4-86-grid': `${hostname}screens/p4-86#card-grid`,
  'generated/screens/p4-86-install': `${hostname}screens/p4-86#install`,
}

// Keep existing section bookmarks when their guides move to one settings page.
const redirectAnchors: Record<string, string[]> = {
  'features/backlight': [
    'sleep-brightness',
    'home-assistant-control',
  ],
  'features/clock': [
    'if-the-time-is-wrong',
  ],
  'features/temperature': [
    'choose-the-clock-bar-sensor',
  ],
  'features/backup': [
    'save-a-backup',
    'restore-a-backup',
    'compatibility-notes',
    'reset-the-display',
  ],
  'features/firmware-updates': [
    'update-your-panel',
    'stable-releases-development-builds-and-custom-firmware',
    'installing-an-older-version',
    'esp32-c6-wifi-co-processor-updates',
    'if-a-new-option-is-missing',
  ],
  'features/media-cover-art': [
    'set-it-up', 'screensaver-settings', 'external-sources', 'advanced-options',
    'track-details-work-but-artwork-is-missing',
  ],
  'features/screensaver': [
    'screensaver', 'choose-when-it-starts', 'choose-what-it-shows',
    'show-photos-or-a-camera', 'photos-from-immich', 'photo-metadata',
    'wake-from-home-assistant', 'screen-schedule', 'pin-protection-after-wake',
    'start-the-screensaver-with-a-card',
  ],
  'features/screen-schedule': [
    'night-schedule', 'set-the-schedule', 'how-it-works-with-other-settings',
    'manual-wake-and-sleep',
  ],
}

export async function writeRedirects(outDir: string) {
  for (const [path, destination] of Object.entries(redirects)) {
    const anchors = redirectAnchors[path]
    const fragmentRedirect = anchors ? `<script>
if (${JSON.stringify(anchors)}.includes(location.hash.slice(1))) {
  location.replace(${JSON.stringify(destination.split('#')[0])} + location.hash)
}
</script>` : ''
    const file = join(outDir, `${path}.html`)
    await mkdir(join(file, '..'), { recursive: true })
    await writeFile(file, `<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>Page moved | EspControl</title>
<meta name="robots" content="noindex,follow">
<link rel="canonical" href="${destination.split('#')[0]}">
${fragmentRedirect}
<meta http-equiv="refresh" content="0; url=${destination}">
</head><body><h1>Page moved</h1><p><a href="${destination}">Continue to the current guide</a>.</p></body></html>\n`)
  }
}

// Read the same parsed Markdown that produces the visible FAQ. No separately
// maintained answers, and no second parser with different link/format rules.
export function faqSchema(md: MarkdownRenderer) {
  md.core.ruler.push('espcontrol-faq', (state) => {
    if (state.env.relativePath !== 'reference/faq.md') return
    const entries = []
    const tokens = state.tokens
    for (let index = 0; index < tokens.length; index++) {
      if (tokens[index].type !== 'heading_open' || tokens[index].tag !== 'h3') continue
      const heading = tokens[index + 1]
      let end = index + 3
      while (end < tokens.length && !(
        tokens[end].type === 'heading_open' && Number(tokens[end].tag.slice(1)) <= 3
      )) end++
      const name = (heading.children ?? [])
        .filter((token) => token.type === 'text' || token.type === 'code_inline')
        .map((token) => token.content).join('').replace(/\u200b/g, '').trim()
      // Rendering these tokens twice would mutate VitePress link URLs twice.
      // Plain answer text uses the parsed inline tokens, including code spans.
      const text = tokens.slice(index + 3, end)
        .filter((token) => token.type === 'inline')
        .map((token) => (token.children ?? [])
          .filter((child) => ['text', 'code_inline', 'softbreak', 'hardbreak'].includes(child.type))
          .map((child) => child.type.endsWith('break') ? ' ' : child.content).join(''))
        .join(' ').trim()
      if (name && text) entries.push({
        '@type': 'Question', name,
        acceptedAnswer: { '@type': 'Answer', text },
      })
    }
    state.env.frontmatter.faqAnswers = entries
  })
}

export function jsonLd(value: unknown) {
  return JSON.stringify(value).replace(/</g, '\\u003c')
}
