import { defineConfig } from 'vitepress'

const hostname = 'https://jtenniswood.github.io/espcontrol/'
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
  'screens/p4-86.md': {
    name: 'ESP32-P4 86 Panel',
    brand: 'ESP32-P4',
    model: 'ESP32-P4-86-Panel-ETH-2RO',
    size: '4 inches',
    resolution: '720 x 720',
    processor: 'ESP32-P4',
  },
}

const faqItems = [
  {
    question: "How Do I Find My Device's IP Address?",
    answer:
      'Check the display when no cards are configured, your router connected devices list, or the ESPHome device page in Home Assistant.',
  },
  {
    question: 'The Web Page Looks Broken or Unstyled',
    answer:
      'Make sure the panel has internet access, clear your browser cache, and try Chrome or Edge.',
  },
  {
    question: "My Device Won't Connect to WiFi",
    answer:
      'Use a 2.4 GHz WiFi network, double-check the password, move closer to the router during setup, or reconnect through the espcontrol setup hotspot.',
  },
  {
    question: 'How Do I Reset the Device?',
    answer:
      'Re-flash the firmware from the install guide with a USB-C data cable. WiFi settings reset, while card configuration is stored separately.',
  },
  {
    question: 'Can I Use This Without Home Assistant?',
    answer:
      'No. EspControl is designed for Home Assistant and needs it for device control, clock sync, temperature data, and screensaver sensor data.',
  },
  {
    question: 'How Do I Update the Firmware?',
    answer:
      'Leave Auto Update enabled for automatic updates, or use Check for Update in the Firmware section of the setup page.',
  },
  {
    question: "What If the Icon I Need Isn't Listed?",
    answer:
      'Open a GitHub issue with the Material Design Icons name and what you would use it for.',
  },
  {
    question: 'How Many Cards Can I Have?',
    answer:
      'The home screen supports 20 cards on JC8012P4A1, 15 on JC1060P470, 6 on JC4880P443, and 9 on 4848S040 or the ESP32-P4 86 Panel, with more available through subpages.',
  },
  {
    question: 'What Is a Subpage?',
    answer:
      'A Subpage card works like a folder, opening another page of cards for grouping rooms or device types.',
  },
  {
    question: 'Can I Back Up My Setup?',
    answer:
      'Yes. Use Export and Import in the Backup section of the setup page to save and restore cards, subpages, colours, and display settings.',
  },
  {
    question: 'Which Panels Are Supported?',
    answer:
      'EspControl supports the Guition JC8012P4A1, JC1060P470, JC4880P443, 4848S040, and ESP32-P4 86 Panel touchscreens.',
  },
  {
    question: 'Does the Panel Work with Other Smart Home Platforms?',
    answer:
      'EspControl is built for Home Assistant. Other platforms only work indirectly if they are integrated into Home Assistant.',
  },
  {
    question: 'The Display Is Stuck on the Loading Screen',
    answer:
      'Wait up to 60 seconds, power-cycle the panel, and if the setup hotspot appears, go through WiFi setup again.',
  },
  {
    question: 'How Is My Data Handled?',
    answer:
      'Device control stays on your local network. Internet access is only used for firmware update checks and web page styling resources.',
  },
]

export default defineConfig({
  title: 'Espcontrol',
  description:
    'Touchscreen control panel for Home Assistant on supported ESP32 panels — card-based controls, web configuration, automatic updates.',
  base: '/espcontrol/',
  lang: 'en-US',
  cleanUrls: true,
  lastUpdated: true,

  sitemap: {
    hostname,
    transformItems: (items) => items.filter((item) => item.url !== '404' && item.url !== '/404'),
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
      JSON.stringify({
        '@context': 'https://schema.org',
        '@graph': [
          {
            '@type': 'WebSite',
            '@id': `${hostname}#website`,
            url: hostname,
            name: 'Espcontrol',
            description:
              'ESPHome firmware for supported ESP32 touchscreens: Home Assistant card controls, web UI, OTA updates.',
            inLanguage: 'en-US',
          },
          {
            '@type': 'SoftwareApplication',
            '@id': `${hostname}#software`,
            name: 'Espcontrol',
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
      if (pageData.relativePath === 'reference/faq.md') {
        articleSchema['@type'] = 'FAQPage'
        articleSchema.mainEntity = faqItems.map((item) => ({
          '@type': 'Question',
          name: item.question,
          acceptedAnswer: {
            '@type': 'Answer',
            text: item.answer,
          },
        }))
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
          { text: '4-inch ESP32-P4 86 Panel', link: '/screens/p4-86' },
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
          { text: 'Time Settings', link: '/features/clock' },
          { text: 'Screensaver', link: '/features/screensaver' },
          { text: 'Backlight', link: '/features/backlight' },
          { text: 'Screen Schedule', link: '/features/screen-schedule' },
          { text: 'Built-in Relays', link: '/features/relays' },
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
          { text: 'Garage Door', link: '/card-types/garage-doors' },
          { text: 'Lock', link: '/card-types/locks' },
          { text: 'Date', link: '/card-types/calendar' },
          { text: 'World Clock', link: '/card-types/timezones' },
          { text: 'Weather', link: '/card-types/weather' },
          { text: 'Internal', link: '/card-types/internal-relays' },
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
      {
        text: 'Development',
        items: [
          { text: 'Project Organization', link: '/development/project-organization' },
          { text: 'Display Sizing Notes', link: '/development/display-sizing' },
          { text: 'Climate Control Spec', link: '/development/climate-control' },
          { text: 'Light Modal UX Plan', link: '/development/light-modal-ux-plan' },
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
