import { defineConfig } from 'vitepress'

const hostname = 'https://jtenniswood.github.io/espcontrol/'

export default defineConfig({
  title: 'Espcontrol',
  description:
    'Touchscreen control panel for Home Assistant on Guition ESP32-P4 — up to 20 buttons, web configuration, automatic updates.',
  base: '/espcontrol/',
  lang: 'en-US',
  cleanUrls: true,
  lastUpdated: true,

  sitemap: {
    hostname,
  },

  head: [
    ['link', { rel: 'icon', type: 'image/svg+xml', href: '/espcontrol/favicon.svg' }],
    [
      'meta',
      {
        name: 'keywords',
        content:
          'Espcontrol, ESPHome, Home Assistant, ESP32-P4, Guition, LVGL, touchscreen, control panel',
      },
    ],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:locale', content: 'en_US' }],
    ['meta', { property: 'og:site_name', content: 'Espcontrol' }],
    [
      'script',
      { type: 'application/ld+json' },
      JSON.stringify({
        '@context': 'https://schema.org',
        '@graph': [
          {
            '@type': 'WebSite',
            '@id': `${hostname}#website`,
            url: hostname,
            name: 'Espcontrol',
            description:
              'ESPHome firmware for a 7-inch Guition ESP32-P4 touchscreen: up to 20 Home Assistant entity buttons, web UI, OTA updates.',
            inLanguage: 'en-US',
          },
          {
            '@type': 'SoftwareApplication',
            '@id': `${hostname}#software`,
            name: 'Espcontrol',
            applicationCategory: 'UtilitiesApplication',
            operatingSystem: 'ESP32',
            description:
              'Home Assistant control panel firmware for Guition ESP32-P4 JC1060P470. Configure buttons and display from the built-in web UI.',
            url: hostname,
            author: {
              '@type': 'Person',
              name: 'jtenniswood',
              url: 'https://github.com/jtenniswood',
            },
            offers: { '@type': 'Offer', price: '0', priceCurrency: 'USD' },
          },
        ],
      }),
    ],
  ],

  transformPageData(pageData) {
    const canonicalUrl = `${hostname}${pageData.relativePath}`
      .replace(/index\.md$/, '')
      .replace(/\.md$/, '')

    const rawTitle = pageData.frontmatter.title ?? pageData.title
    const title =
      typeof rawTitle === 'string' ? rawTitle : rawTitle != null ? String(rawTitle) : ''
    const description = String(pageData.frontmatter.description ?? '')

    pageData.frontmatter.head ??= []
    pageData.frontmatter.head.push(
      ['link', { rel: 'canonical', href: canonicalUrl }],
      ['meta', { property: 'og:title', content: title }],
      ['meta', { property: 'og:description', content: description }],
      ['meta', { property: 'og:url', content: canonicalUrl }],
      ['meta', { name: 'twitter:title', content: title }],
      ['meta', { name: 'twitter:description', content: description }],
    )

    if (pageData.relativePath !== 'index.md' && title && description) {
      const isHowTo = pageData.relativePath === 'install.md'
      const articleSchema: Record<string, unknown> = {
        '@context': 'https://schema.org',
        '@type': isHowTo ? 'HowTo' : 'TechArticle',
        name: title,
        description,
        url: canonicalUrl,
        isPartOf: { '@id': `${hostname}#website` },
        author: { '@type': 'Person', name: 'jtenniswood', url: 'https://github.com/jtenniswood' },
      }
      if (isHowTo) {
        articleSchema.step = [
          { '@type': 'HowToStep', name: 'Flash firmware from your browser' },
          { '@type': 'HowToStep', name: 'Connect to WiFi' },
          { '@type': 'HowToStep', name: 'Add to Home Assistant' },
          { '@type': 'HowToStep', name: 'Configure buttons from the web page' },
        ]
      }
      pageData.frontmatter.head.push([
        'script',
        { type: 'application/ld+json' },
        JSON.stringify(articleSchema),
      ])
    }
  },

  themeConfig: {
    nav: [
      { text: 'Install', link: '/install' },
      { text: 'Docs', link: '/' },
      { text: 'GitHub', link: 'https://github.com/jtenniswood/espcontrol' },
    ],

    sidebar: [
      {
        text: 'Getting Started',
        items: [
          { text: 'Overview', link: '/' },
          { text: 'Install', link: '/install' },
          { text: 'Web UI', link: '/web-ui' },
        ],
      },
      {
        text: 'Features',
        items: [
          { text: 'Buttons & Icons', link: '/buttons-and-icons' },
          { text: 'Display & Screensaver', link: '/display-screensaver' },
          { text: 'Backlight Schedule', link: '/backlight-schedule' },
          { text: 'Firmware Updates', link: '/firmware-updates' },
        ],
      },
      {
        text: 'Reference',
        items: [
          { text: 'Specifications', link: '/hardware-architecture' },
          { text: 'FAQ', link: '/faq' },
          { text: 'Roadmap', link: '/roadmap' },
        ],
      },
      {
        text: 'Advanced',
        items: [
          { text: 'ESPHome Manual Setup', link: '/esphome-manual-setup' },
        ],
      },
      {
        text: 'Developer',
        collapsed: true,
        items: [
          { text: 'Package Layout', link: '/package-layout' },
          { text: 'External Component', link: '/external-component' },
        ],
      },
    ],

    editLink: {
      pattern: 'https://github.com/jtenniswood/espcontrol/edit/main/docs/:path',
      text: 'Edit this page on GitHub',
    },

    socialLinks: [{ icon: 'github', link: 'https://github.com/jtenniswood/espcontrol' }],

    search: {
      provider: 'local',
    },
  },
})
