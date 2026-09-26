import { defineConfig } from 'vitepress'
import { faqSchema, hostname, jsonLd, writeRedirects } from './discovery'

const defaultImage = {
  url: `${hostname}images/home_screen_hero.jpg`,
  width: '1024',
  height: '798',
  type: 'image/jpeg',
}

const pageImages: Record<string, typeof defaultImage> = {
  'screens/4848s040.md': {
    url: `${hostname}images/4848s040-hero.jpg`,
    width: '1024',
    height: '999',
    type: 'image/jpeg',
  },
  'screens/jc1060p470.md': {
    url: `${hostname}images/jc1060p470-hero.jpg`,
    width: '1024',
    height: '798',
    type: 'image/jpeg',
  },
  'screens/jc4880p443.md': {
    url: `${hostname}images/jc4880p443-hero.jpg`,
    width: '800',
    height: '1024',
    type: 'image/jpeg',
  },
  'features/setup.md': {
    url: `${hostname}images/screen-setup.png`,
    width: '625',
    height: '1024',
    type: 'image/png',
  },
  'features/subpages.md': {
    url: `${hostname}images/screen-subpage.png`,
    width: '1024',
    height: '606',
    type: 'image/png',
  },
  'features/relays.md': {
    url: `${hostname}images/relay-controls.svg`,
    width: '646',
    height: '786',
    type: 'image/svg+xml',
  },
  'card-types/buttons.md': {
    url: `${hostname}images/card-button.png`,
    width: '370',
    height: '336',
    type: 'image/png',
  },
  'card-types/sensors.md': {
    url: `${hostname}images/card-sensor.png`,
    width: '368',
    height: '340',
    type: 'image/png',
  },
  'card-types/switches.md': {
    url: `${hostname}images/card-toggle.png`,
    width: '366',
    height: '340',
    type: 'image/png',
  },
  'getting-started/home-assistant-actions.md': {
    url: `${hostname}images/ha-actions-step-1.png`,
    width: '684',
    height: '508',
    type: 'image/png',
  },
}

const screenProducts: Record<string, Record<string, string>> = {
  'screens/4848s040.md': {
    name: 'Guition 4848S040',
    model: '4848S040',
    size: '4 inches',
    resolution: '480 x 480',
    processor: 'ESP32-S3',
  },
  'screens/jc1060p470.md': {
    name: 'Guition JC1060P470',
    model: 'JC1060P470',
    size: '7 inches',
    resolution: '1024 x 600',
    processor: 'ESP32-P4',
  },
  'screens/jc1060p470-v1.md': {
    name: 'Guition JC1060P470 V1',
    model: 'JC1060P470 V1',
    size: '7 inches',
    resolution: '1024 x 600',
    processor: 'ESP32-P4',
  },
  'screens/jc1060p470-v2.md': {
    name: 'Guition JC1060P470 V2',
    brand: 'Guition',
    model: 'JC1060P470 V2',
    size: '7 inches',
    resolution: '1024 x 600',
    processor: 'ESP32-P4',
  },
  'screens/jc4880p443.md': {
    name: 'Guition JC4880P443',
    model: 'JC4880P443',
    size: '4.3 inches',
    resolution: '480 x 800',
    processor: 'ESP32-P4',
  },
  'screens/jc8012p4a1.md': {
    name: 'Guition JC8012P4A1',
    brand: 'Guition',
    model: 'JC8012P4A1',
    size: '10.1 inches',
    resolution: '1280 x 800',
    processor: 'ESP32-P4',
  },
  'screens/jc8012p4a1-v1.md': {
    name: 'Guition JC8012P4A1 V1',
    model: 'JC8012P4A1 V1',
    size: '10.1 inches',
    resolution: '1280 x 800',
    processor: 'ESP32-P4',
  },
  'screens/jc8012p4a1-v2.md': {
    name: 'Guition JC8012P4A1 V2',
    brand: 'Guition',
    model: 'JC8012P4A1 V2',
    size: '10.1 inches',
    resolution: '1280 x 800',
    processor: 'ESP32-P4',
  },
  'screens/jc8012p4a1-v3.md': {
    name: 'Guition JC8012P4A1 V3',
    model: 'JC8012P4A1 V3',
    size: '10.1 inches',
    resolution: '1280 x 800',
    processor: 'ESP32-P4',
  },
  'screens/p4-86.md': {
    name: 'ESP32-P4 86',
    brand: 'Waveshare',
    model: 'ESP32-P4-86-Panel-ETH-2RO',
    size: '4 inches',
    resolution: '720 x 720',
    processor: 'ESP32-P4',
  },
}

export default defineConfig({
  title: 'EspControl',
  description:
    'Touchscreen control panel for Home Assistant on supported ESP32 panels — card-based controls, web configuration, automatic updates.',
  base: '/espcontrol/',
  lang: 'en-US',
  cleanUrls: true,
  lastUpdated: true,
  srcExclude: ['generated/**'],
  markdown: { config: faqSchema },
  buildEnd: ({ outDir }) => writeRedirects(outDir),

  sitemap: {
    hostname,
    transformItems: (items) => items.filter((item) => item.url !== '404' && item.url !== '/404'),
  },

  head: [
    ...(process.env.GOOGLE_SITE_VERIFICATION
      ? [['meta', { name: 'google-site-verification', content: process.env.GOOGLE_SITE_VERIFICATION }] as [string, Record<string, string>]]
      : []),
    ['link', { rel: 'icon', type: 'image/svg+xml', href: '/espcontrol/favicon.svg' }],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:locale', content: 'en_US' }],
    ['meta', { property: 'og:site_name', content: 'EspControl' }],
    ['meta', { name: 'twitter:card', content: 'summary_large_image' }],
    [
      'style',
      {},
      '.sp-support-btn{position:fixed;right:28px;bottom:28px;z-index:150;display:inline-block;line-height:0}.sp-support-btn img{height:60px;display:block;border-radius:999px}',
    ],
    [
      'script',
      {},
      `document.addEventListener('DOMContentLoaded',function(){if(document.querySelector('.sp-support-btn'))return;var link=document.createElement('a');link.className='sp-support-btn';link.href='https://www.buymeacoffee.com/jtenniswood';link.target='_blank';link.rel='noopener';link.innerHTML='<img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" height="60" style="border-radius:999px;">';document.body.appendChild(link);});`,
    ],
    [
      'script',
      { type: 'application/ld+json' },
      jsonLd({
        '@context': 'https://schema.org',
        '@graph': [
          {
            '@type': 'WebSite',
            '@id': `${hostname}#website`,
            url: hostname,
            name: 'EspControl',
            description:
              'ESPHome firmware for supported ESP32 touchscreens: Home Assistant card controls, web UI, OTA updates.',
            inLanguage: 'en-US',
          },
          {
            '@type': 'SoftwareApplication',
            '@id': `${hostname}#software`,
            name: 'EspControl',
            applicationCategory: 'UtilitiesApplication',
            operatingSystem: 'ESP32',
            description:
              'Home Assistant control panel firmware for supported ESP32 touchscreens. Configure cards and display from the built-in web UI.',
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
    const image = pageImages[pageData.relativePath] ?? defaultImage

    pageData.frontmatter.head ??= []
    pageData.frontmatter.head.push(
      ['link', { rel: 'canonical', href: canonicalUrl }],
      ['meta', { property: 'og:title', content: title }],
      ['meta', { property: 'og:description', content: description }],
      ['meta', { property: 'og:url', content: canonicalUrl }],
      ['meta', { property: 'og:image', content: image.url }],
      ['meta', { property: 'og:image:width', content: image.width }],
      ['meta', { property: 'og:image:height', content: image.height }],
      ['meta', { property: 'og:image:type', content: image.type }],
      ['meta', { name: 'twitter:title', content: title }],
      ['meta', { name: 'twitter:description', content: description }],
      ['meta', { name: 'twitter:image', content: image.url }],
    )

    if (pageData.relativePath === '404.md') {
      pageData.frontmatter.head.push(['meta', { name: 'robots', content: 'noindex' }])
    }

    if (
      pageData.relativePath !== 'index.md' &&
      pageData.relativePath !== '404.md' &&
      title &&
      description
    ) {
      const articleSchema: Record<string, unknown> = {
        '@context': 'https://schema.org',
        '@type': 'TechArticle',
        name: title,
        description,
        url: canonicalUrl,
        isPartOf: { '@id': `${hostname}#website` },
        author: { '@type': 'Person', name: 'jtenniswood', url: 'https://github.com/jtenniswood' },
      }
      if (pageData.lastUpdated) {
        articleSchema.dateModified = new Date(pageData.lastUpdated).toISOString()
      }
      if (pageData.relativePath === 'reference/faq.md') {
        articleSchema['@type'] = 'FAQPage'
        articleSchema.mainEntity = pageData.frontmatter.faqAnswers
        delete pageData.frontmatter.faqAnswers
      }
      const screenProduct = screenProducts[pageData.relativePath]
      if (screenProduct) {
        articleSchema.about = {
          '@type': 'Product',
          name: screenProduct.name,
          brand: { '@type': 'Brand', name: screenProduct.brand ?? 'Guition' },
          model: screenProduct.model,
          category: 'ESP32 touchscreen panel',
          url: canonicalUrl,
          additionalProperty: [
            { '@type': 'PropertyValue', name: 'Screen size', value: screenProduct.size },
            { '@type': 'PropertyValue', name: 'Resolution', value: screenProduct.resolution },
            { '@type': 'PropertyValue', name: 'Processor', value: screenProduct.processor },
          ],
        }
      }
      pageData.frontmatter.head.push([
        'script',
        { type: 'application/ld+json' },
        jsonLd(articleSchema),
      ])
    }
  },

  themeConfig: {
    logo: '/images/espcontrol-logo.svg',
    siteTitle: 'EspControl',
    nav: [
      { text: 'Choose a Screen', link: '/screens/' },
      { text: 'Install', link: '/getting-started/install' },
      { text: 'Guides', link: '/guides/' },
      { text: 'FAQ', link: '/reference/faq' },
      { text: 'GitHub', link: 'https://github.com/jtenniswood/espcontrol' },
    ],

    sidebar: [
      {
        text: 'Getting Started',
        items: [
          { text: 'Overview', link: '/' },
          { text: 'Choose a Screen', link: '/screens/' },
          { text: 'Install', link: '/getting-started/install' },
          { text: 'Enable Actions', link: '/getting-started/home-assistant-actions' },
          { text: 'Configure', link: '/features/setup' },
          { text: 'Guides', link: '/guides/' },
          { text: 'Troubleshooting', link: '/getting-started/troubleshooting' },
        ],
      },
      {
        text: 'Supported Screens',
        items: [
          {
            text: '10.1-inch JC8012P4A1',
            link: '/screens/jc8012p4a1',
            collapsed: true,
            items: [
              { text: 'V3', link: '/screens/jc8012p4a1-v3' },
              { text: 'V2', link: '/screens/jc8012p4a1-v2' },
              { text: 'V1', link: '/screens/jc8012p4a1-v1' },
            ],
          },
          {
            text: '7-inch JC1060P470',
            link: '/screens/jc1060p470',
            collapsed: true,
            items: [
              { text: 'V2', link: '/screens/jc1060p470-v2' },
              { text: 'V1', link: '/screens/jc1060p470-v1' },
            ],
          },
          { text: '4.3-inch JC4880P443', link: '/screens/jc4880p443' },
          { text: '4-inch ESP32-P4 86', link: '/screens/p4-86' },
          { text: '4-inch 4848S040', link: '/screens/4848s040' },
          { text: 'Printable Stands', link: '/reference/3d-printable-stands' },
        ],
      },
      {
        text: 'Configuring',
        items: [
          { text: 'Setup', link: '/features/setup' },
          { text: 'Subpages', link: '/features/subpages' },
          { text: 'Speaker Groups', link: '/features/speaker-groups' },
        ],
      },
      {
        text: 'Card Types',
        items: [
          { text: 'Overview', link: '/card-types/' },
          { text: 'Action', link: '/card-types/actions' },
          { text: 'Alarm', link: '/card-types/alarms' },
          { text: 'Camera', link: '/card-types/cameras' },
          { text: 'Climate', link: '/card-types/climate' },
          { text: 'Cover', link: '/card-types/covers' },
          { text: 'Date & Time', link: '/card-types/calendar' },
          { text: "Timer", link: "/card-types/timers" },
          { text: 'Doors & Windows', link: '/card-types/doors-windows' },
          { text: 'Fans', link: '/card-types/fans' },
          { text: 'Garage Door', link: '/card-types/garage-doors' },
          { text: 'Gate', link: '/card-types/gates' },
          { text: 'Internal', link: '/card-types/internal-relays' },
          { text: 'Lawn Mower', link: '/card-types/lawn-mower' },
          { text: 'Lights', link: '/card-types/lights' },
          { text: 'Local Action', link: '/card-types/local-actions' },
          { text: 'Lock', link: '/card-types/locks' },
          { text: 'Media', link: '/card-types/media' },
          { text: 'Option Select', link: '/card-types/option-select' },
          { text: 'Presence', link: '/card-types/presence' },
          { text: 'Screen Lock', link: '/card-types/screen-lock' },
          { text: 'Sensor', link: '/card-types/sensors' },
          { text: 'Local Sensor', link: '/card-types/local-sensors' },
          { text: 'Slider', link: '/card-types/sliders' },
          { text: 'Subpage', link: '/features/subpages' },
          { text: 'Switch', link: '/card-types/switches' },
          { text: 'Trigger', link: '/card-types/buttons' },
          { text: 'Weather', link: '/card-types/weather' },
          { text: 'Webhook', link: '/card-types/webhooks' },
          { text: 'Wifi Sharing', link: '/card-types/wifi-share' },
          { text: 'World Clock', link: '/card-types/timezones' },
        ],
      },
      {
        text: 'Settings',
        items: [
          { text: '<span class="sidebar-static-header">Display</span>' },
          { text: 'Appearance', link: '/features/appearance' },
          { text: 'Backlight', link: '/features/backlight' },
          { text: 'Clock Bar', link: '/features/clock-bar' },
          { text: 'Battery', link: '/features/battery' },
          { text: 'Rotation', link: '/features/rotation' },
          { text: '<span class="sidebar-static-header">Sleep & Schedule</span>' },
          { text: 'Idle', link: '/features/idle' },
          { text: 'Screensaver', link: '/features/screensaver' },
          { text: 'Media Cover Art', link: '/features/media-cover-art' },
          { text: 'Night Schedule', link: '/features/screen-schedule' },
          { text: '<span class="sidebar-static-header">System</span>' },
          { text: 'Language', link: '/features/language' },
          { text: 'Time Settings', link: '/features/clock' },
          { text: 'Temperature Settings', link: '/features/temperature' },
          { text: 'Device Name', link: '/features/setup#naming-your-panel' },
          { text: 'Backup', link: '/features/backup' },
          { text: 'Factory Reset', link: '/features/backup#reset-the-display' },
          { text: 'Firmware', link: '/features/firmware-updates' },
          { text: 'Built-in Relays', link: '/features/relays' },
          { text: 'Voice Control', link: '/features/voice-control' },
        ],
      },
      {
        text: 'Immich Photos',
        items: [
          { text: 'Overview', link: '/immich/' },
          { text: 'Installation', link: '/immich/installation' },
          { text: 'Connect to EspControl', link: '/immich/display-setup' },
          { text: 'Using Your Frame', link: '/immich/using-your-frame' },
          { text: 'Settings Reference', link: '/immich/settings-reference' },
          { text: 'Home Assistant Entities', link: '/immich/entities' },
          { text: 'Browser & API Add-on', link: '/immich/add-on' },
          { text: 'Compatibility', link: '/immich/compatibility' },
        ],
      },
      {
        text: 'Advanced',
        items: [
          { text: 'Manual Setup', link: '/getting-started/manual-esphome-setup' },
          { text: 'Wifi Issues', link: '/getting-started/c6-recovery' },
        ],
      },
      {
        text: 'Reference',
        items: [
          { text: 'FAQ', link: '/reference/faq' },
          { text: 'Card Capabilities', link: '/reference/card-capabilities' },
          { text: 'Icon Reference', link: '/reference/icons' },
          { text: 'Language Support', link: '/reference/language-support' },
        ],
      },
      {
        text: 'Community',
        items: [
          {
            text: 'Contributing',
            link: '/reference/contributing',
            collapsed: true,
            items: [
              { text: 'Collect USB Logs', link: '/reference/collect-usb-logs' },
            ],
          },
        ],
      },
      {
        text: 'About',
        items: [
          { text: 'Commercial Partnerships', link: '/reference/partnerships' },
          { text: 'Privacy Policy', link: '/reference/privacy' },
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
