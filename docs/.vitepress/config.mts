import { defineConfig } from 'vitepress'

const hostname = 'https://jtenniswood.github.io/espcontrol/'

export default defineConfig({
  title: 'Espcontrol',
  description:
    'Touchscreen control panel for Home Assistant on Guition ESP32 — up to 24 buttons, web configuration, automatic updates.',
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
          'Espcontrol, ESPHome, Home Assistant, ESP32-P4, ESP32-S3, Guition, LVGL, touchscreen, control panel',
      },
    ],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:locale', content: 'en_US' }],
    ['meta', { property: 'og:site_name', content: 'Espcontrol' }],
    ['meta', { property: 'og:image', content: `${hostname}images/home_screen_hero.jpg` }],
    ['meta', { property: 'og:image:width', content: '1024' }],
    ['meta', { property: 'og:image:height', content: '902' }],
    ['meta', { property: 'og:image:type', content: 'image/jpeg' }],
    ['meta', { name: 'twitter:card', content: 'summary_large_image' }],
    ['meta', { name: 'twitter:image', content: `${hostname}images/home_screen_hero.jpg` }],
    ['script', {
      'data-name': 'BMC-Widget',
      'data-cfasync': 'false',
      src: 'https://cdnjs.buymeacoffee.com/1.0.0/widget.prod.min.js',
      'data-id': 'jtenniswood',
      'data-description': 'Support me on Buy me a coffee!',
      'data-message': '',
      'data-color': '#FFDD00',
      'data-position': 'Right',
      'data-x_margin': '18',
      'data-y_margin': '18',
    }],
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
              'ESPHome firmware for Guition ESP32 touchscreens: up to 24 Home Assistant entity buttons, web UI, OTA updates.',
            inLanguage: 'en-US',
          },
          {
            '@type': 'SoftwareApplication',
            '@id': `${hostname}#software`,
            name: 'Espcontrol',
            applicationCategory: 'UtilitiesApplication',
            operatingSystem: 'ESP32',
            description:
              'Home Assistant control panel firmware for Guition ESP32 touchscreens. Configure buttons and display from the built-in web UI.',
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
      const isHowTo =
        pageData.relativePath === 'getting-started/install.md' ||
        pageData.relativePath === 'getting-started/manual-esphome-setup.md'
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
        articleSchema.step =
          pageData.relativePath === 'getting-started/manual-esphome-setup.md'
            ? [
                { '@type': 'HowToStep', name: 'Choose the correct ESPHome package file' },
                { '@type': 'HowToStep', name: 'Create the device in ESPHome Device Builder' },
                { '@type': 'HowToStep', name: 'Install by USB or OTA' },
                { '@type': 'HowToStep', name: 'Add the display to Home Assistant' },
              ]
            : [
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
      { text: 'Install', link: '/getting-started/install' },
      { text: 'Docs', link: '/' },
      { text: 'GitHub', link: 'https://github.com/jtenniswood/espcontrol' },
    ],

    sidebar: [
      {
        text: 'Getting Started',
        items: [
          { text: 'Overview', link: '/' },
          { text: 'Install', link: '/getting-started/install' },
          { text: 'Manual ESPHome Setup', link: '/getting-started/manual-esphome-setup' },
          { text: 'Home Assistant Actions', link: '/getting-started/home-assistant-actions' },
          { text: 'Troubleshooting', link: '/getting-started/troubleshooting' },
        ],
      },
      {
        text: 'Supported Screens',
        items: [
          { text: '10.1-inch JC8012P4A1', link: '/screens/jc8012p4a1' },
          { text: '7-inch JC1060P470', link: '/screens/jc1060p470' },
          { text: '4.3-inch JC4880P443', link: '/screens/jc4880p443' },
          { text: '4-inch 4848S040', link: '/screens/4848s040' },
        ],
      },
      {
        text: 'Configuring',
        items: [
          { text: 'Setup', link: '/features/setup' },
          { text: 'Subpages', link: '/features/subpages' },
          { text: 'Appearance', link: '/features/appearance' },
          { text: 'Temperature', link: '/features/temperature' },
          { text: 'Idle', link: '/features/idle' },
          { text: 'Clock', link: '/features/clock' },
          { text: 'Screensaver', link: '/features/screensaver' },
          { text: 'Backlight', link: '/features/backlight' },
          { text: 'Screen Schedule', link: '/features/screen-schedule' },
          { text: 'Backup', link: '/features/backup' },
          { text: 'Firmware Updates', link: '/features/firmware-updates' },
        ],
      },
      {
        text: 'Card Types',
        items: [
          { text: 'Switch', link: '/card-types/switches' },
          { text: 'Action', link: '/card-types/actions' },
          { text: 'Trigger', link: '/card-types/buttons' },
          { text: 'Sensor', link: '/card-types/sensors' },
          { text: 'Slider', link: '/card-types/sliders' },
          { text: 'Cover', link: '/card-types/covers' },
          { text: 'Climate', link: '/card-types/climate' },
          { text: 'Garage Door', link: '/card-types/garage-doors' },
          { text: 'Date', link: '/card-types/calendar' },
          { text: 'World Clock', link: '/card-types/timezones' },
          { text: 'Weather', link: '/card-types/weather' },
          { text: 'Weather Forecast', link: '/card-types/weather-forecast' },
          { text: 'Subpage', link: '/features/subpages' },
        ],
      },
      {
        text: 'Reference',
        items: [
          { text: 'Icon Reference', link: '/reference/icons' },
          { text: 'FAQ', link: '/reference/faq' },
          { text: 'Roadmap', link: '/reference/roadmap' },
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
