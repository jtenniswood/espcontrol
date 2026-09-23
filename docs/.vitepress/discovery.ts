import { mkdir, writeFile } from 'node:fs/promises'
import { join } from 'node:path'
import type { MarkdownRenderer } from 'vitepress'

export const hostname = 'https://jtenniswood.github.io/espcontrol/'

// GitHub Pages cannot configure HTTP redirects. These HTML fallbacks preserve
// published links while keeping retired pages out of search and the sitemap.
export const redirects: Record<string, string> = {
  'reference/request-device-support': `${hostname}screens/`,
  'card-types/weather-forecast': `${hostname}card-types/weather#temperatures-tomorrow`,
  'generated/cards/capabilities': `${hostname}reference/card-capabilities`,
  'generated/cards/runtime-coverage':
    'https://github.com/jtenniswood/espcontrol/blob/main/dev-docs/generated/card-runtime-coverage.md',
  'generated/screens/4848s040-grid': `${hostname}screens/4848s040#card-grid`,
  'generated/screens/4848s040-install': `${hostname}screens/4848s040#install`,
  'generated/screens/jc1060p470-grid': `${hostname}screens/jc1060p470#card-grid`,
  'generated/screens/jc1060p470-install': `${hostname}screens/jc1060p470-v1#install`,
  'generated/screens/jc1060p470-v2-grid': `${hostname}screens/jc1060p470#card-grid`,
  'generated/screens/jc1060p470-v2-install': `${hostname}screens/jc1060p470-v2#install`,
  'generated/screens/jc4880p443-grid': `${hostname}screens/jc4880p443#card-grid`,
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

export async function writeRedirects(outDir: string) {
  for (const [path, destination] of Object.entries(redirects)) {
    const file = join(outDir, `${path}.html`)
    await mkdir(join(file, '..'), { recursive: true })
    await writeFile(file, `<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>Page moved | EspControl</title>
<meta name="robots" content="noindex,follow">
<link rel="canonical" href="${destination.split('#')[0]}">
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
