<script setup lang="ts">
import { ref } from 'vue'
import { withBase } from 'vitepress'

const selectedFeature = ref(0)
const features = [
  {
    name: 'Take control', label: 'THE EVERYDAY, ONE TAP AWAY', title: 'Set the mood.\nStart the music.',
    description: 'Dim the lights, put on a favourite playlist, or get the room just right. Bring the controls you use most out of your phone and into your home.',
    image: 'jc1060p470-hero.jpg', alt: '7-inch EspControl panel with radar, temperature, music and room controls',
    links: [
      { text: 'Lights, scenes & switches', href: '/card-types/lights' },
      { text: 'Music & speaker groups', href: '/card-types/media' },
      { text: 'Climate & comfort', href: '/card-types/climate' },
    ],
  },
  {
    name: 'Stay in the know', label: 'A LITTLE MORE IN THE PICTURE', title: 'Your day.\nAt a glance.',
    description: 'A forecast before you head out. A camera snapshot by the front door. Live temperatures, energy readings and even animated weather radar, together on one screen.',
    image: 'jc8012p4a1-hero.jpg', alt: '10.1-inch panel displaying a radar image, daily weather forecast and temperature cards',
    links: [
      { text: 'Daily weather forecasts', href: '/card-types/weather' },
      { text: 'Cameras & animated GIFs', href: '/card-types/cameras' },
      { text: 'Live sensors & readings', href: '/card-types/sensors' },
    ],
  },
  {
    name: 'Make it yours', label: 'YOUR ROOMS. YOUR ROUTINES.', title: 'A place for\neverything.',
    description: 'Give each room its own page. Make an important card bigger. Pick colours that feel like you. Arrange it all in your browser with a drag, a resize and a click.',
    image: '4848s040-subpage.jpg', alt: 'Devices subpage on a 4-inch S3, with a Back button and differently sized cards',
    links: [
      { text: 'Build your layout', href: '/features/setup' },
      { text: 'Create room subpages', href: '/features/subpages' },
      { text: 'Colours, curves & themes', href: '/features/appearance' },
    ],
  },
]
const panels = [
  { name: '4-inch S3', model: '4848S040', detail: 'Small screen. Everyday essentials.', spec: '480 × 480 · 9 home slots', image: '4848s040-hero.jpg', height: 1167, href: '/screens/4848s040' },
  { name: '4-inch P4', model: 'ESP32-P4 86', detail: 'A sharper square of control.', spec: '720 × 720 · 9 home slots', image: 'p4-86-hero.jpg', height: 1316, href: '/screens/p4-86' },
  { name: '4.3-inch P4', model: 'JC4880P443', detail: 'Portrait or landscape. Your call.', spec: '480 × 800 · 6 home slots', image: 'jc4880p443-hero.jpg', height: 1228, href: '/screens/jc4880p443' },
  { name: '7-inch P4', model: 'JC1060P470', detail: 'More room for your favourite things.', spec: '1024 × 600 · 15 home slots', image: 'jc1060p470-hero.jpg', height: 1137, href: '/screens/jc1060p470' },
  { name: '10.1-inch P4', model: 'JC8012P4A1', detail: 'See the bigger picture.', spec: '1280 × 800 · 20 home slots', image: 'jc8012p4a1-hero.jpg', height: 1177, href: '/screens/jc8012p4a1' },
]
function onTabKeydown(event: KeyboardEvent) {
  let next = selectedFeature.value
  if (event.key === 'ArrowRight') next = (next + 1) % features.length
  else if (event.key === 'ArrowLeft') next = (next + features.length - 1) % features.length
  else if (event.key === 'Home') next = 0
  else if (event.key === 'End') next = features.length - 1
  else return
  event.preventDefault()
  selectedFeature.value = next
  const tabs = (event.currentTarget as HTMLElement).parentElement?.querySelectorAll<HTMLButtonElement>('[role="tab"]')
  tabs?.[next]?.focus()
}
</script>

<template>
  <main class="ec-home">
    <section class="ec-hero ec-container" aria-labelledby="espcontrol-no-code-home-assistant-touchscreen">
      <div class="ec-hero-copy">
        <p class="ec-eyebrow"><span class="ec-status-dot" aria-hidden="true"></span> MADE FOR HOME ASSISTANT</p>
        <h1 id="espcontrol-no-code-home-assistant-touchscreen">Your home.<br><span>At your fingertips.</span></h1>
        <p class="ec-intro">Give your smart home a screen of its own. Lights, music, climate and the little things you check every day. All a tap away.</p>
        <div class="ec-actions">
          <a class="ec-button ec-primary" :href="withBase('/screens/')">Find your screen <span aria-hidden="true">↗</span></a>
          <a class="ec-button ec-secondary" :href="withBase('/getting-started/install')">Install EspControl <span aria-hidden="true">→</span></a>
        </div>
        <p class="ec-hero-note">Already up and running? <a :href="withBase('/guides/')">Explore the docs <span aria-hidden="true">↗</span></a></p>
      </div>
      <figure class="ec-hero-visual">
        <img :src="withBase('/images/home_screen_hero.jpg')" alt="EspControl panels with colourful temperature, lighting, media and weather cards" width="1600" height="1288" fetchpriority="high">
        <figcaption><span class="ec-status-dot" aria-hidden="true"></span> A dedicated screen. A home that feels like yours.</figcaption>
        <span class="ec-photo-label" aria-hidden="true">Real panels. Real possibilities.</span>
      </figure>
    </section>

    <div class="ec-proof ec-container" aria-label="EspControl essentials">
      <p><span aria-hidden="true">↗</span> Install from your browser</p>
      <p><span aria-hidden="true">⌘</span> Configure without YAML</p>
      <p><span aria-hidden="true">◉</span> Runs on dedicated ESP32 panels</p>
    </div>

    <section id="what-you-can-build" class="ec-section ec-container" aria-labelledby="ec-features-heading">
      <div class="ec-section-heading">
        <div><p class="ec-eyebrow">SMALL SCREEN. SO MANY POSSIBILITIES.</p><h2 id="ec-features-heading">More than a row of buttons.</h2></div>
        <p>Bring your Home Assistant devices together in a way that makes sense for the people who live with them.</p>
      </div>
      <div class="ec-tabs" role="tablist" aria-label="Explore EspControl features">
        <button v-for="(feature, index) in features" :id="`ec-feature-tab-${index}`" :key="feature.name" type="button" role="tab" :aria-selected="selectedFeature === index" :aria-controls="`ec-feature-panel-${index}`" :tabindex="selectedFeature === index ? 0 : -1" @click="selectedFeature = index" @keydown="onTabKeydown">
          <span class="ec-tab-number" aria-hidden="true">0{{ index + 1 }}</span>{{ feature.name }}<span class="ec-tab-arrow" aria-hidden="true">↗</span>
        </button>
      </div>
      <div v-for="(feature, index) in features" v-show="selectedFeature === index" :id="`ec-feature-panel-${index}`" :key="feature.name" class="ec-feature-panel" role="tabpanel" :aria-labelledby="`ec-feature-tab-${index}`" tabindex="0">
        <div class="ec-feature-image"><img :src="withBase(`/images/${feature.image}`)" :alt="feature.alt" width="1600" :height="index === 0 ? 1137 : index === 1 ? 1177 : 1238" loading="lazy"></div>
        <div class="ec-feature-copy">
          <p class="ec-eyebrow">{{ feature.label }}</p>
          <h3>{{ feature.title }}</h3><p>{{ feature.description }}</p>
          <ul><li v-for="link in feature.links" :key="link.href"><a :href="withBase(link.href)">{{ link.text }} <span aria-hidden="true">↗</span></a></li></ul>
        </div>
      </div>
      <p id="card-types" class="ec-more">From door locks to lawn mowers, there’s more to explore. <a :href="withBase('/card-types/')">See all card types <span aria-hidden="true">→</span></a></p>
    </section>

    <section id="main-features" class="ec-section ec-life-section" aria-labelledby="ec-life-heading">
      <div class="ec-container">
        <p class="ec-eyebrow">THOUGHTFUL LITTLE EXTRAS</p><h2 id="ec-life-heading">Fits your home.<br>Follows your day.</h2>
        <div class="ec-life-grid">
          <a class="ec-life-card" :href="withBase('/features/screen-schedule')"><span class="ec-life-symbol ec-moon" aria-hidden="true">◐</span><h3>Knows when to wind down.</h3><p>Dim the screen, show a clock, or go dark overnight. Set a schedule that fits your routine.</p><span class="ec-text-link">Explore screen schedules <span aria-hidden="true">↗</span></span></a>
          <a class="ec-life-card" :href="withBase('/immich/')"><span class="ec-life-symbol ec-photo" aria-hidden="true">▧</span><h3>Your memories, on display.</h3><p>Turn quiet moments into a photo frame. Connect your Immich library through the companion integration.</p><span class="ec-text-link">Meet the photo screensaver <span aria-hidden="true">↗</span></span></a>
          <a class="ec-life-card" :href="withBase('/features/screensaver#pin-protection-after-wake')"><span class="ec-life-symbol ec-lock" aria-hidden="true">⌑</span><h3>A little peace of mind.</h3><p>Add a touchscreen PIN for shared spaces, with an optional shuffled keypad when you wake the panel.</p><span class="ec-text-link">Discover PIN protection <span aria-hidden="true">↗</span></span></a>
        </div>
      </div>
    </section>

    <section id="where-to-buy" class="ec-section ec-container" aria-labelledby="ec-panels-heading">
      <div class="ec-section-heading"><div><p class="ec-eyebrow">FIND YOUR FIT</p><h2 id="ec-panels-heading">A screen for every space.</h2></div><p>A compact bedside companion or a bigger view of your home. Pick the panel that feels right.</p></div>
      <div class="ec-panel-grid">
        <a v-for="panel in panels" :key="panel.model" :href="withBase(panel.href)" class="ec-device-card">
          <div class="ec-device-photo"><img :src="withBase(`/images/${panel.image}`)" :alt="`${panel.model} running EspControl`" width="1600" :height="panel.height" loading="lazy"></div>
          <div class="ec-device-copy"><p class="ec-device-model">{{ panel.model }} <span aria-hidden="true">↗</span></p><h3>{{ panel.name }}</h3><p>{{ panel.detail }}</p><small>{{ panel.spec }}</small></div>
        </a>
      </div>
      <p class="ec-more">Choose the exact model and hardware revision before installing. <a :href="withBase('/screens/')">Compare supported screens <span aria-hidden="true">→</span></a></p>
    </section>

    <section id="how-it-works" class="ec-section ec-container ec-start" aria-labelledby="ec-start-heading">
      <div><p class="ec-eyebrow">FROM BOX TO BEDSIDE</p><h2 id="ec-start-heading">Less setup.<br>More living.</h2><p>You bring the panel and Home Assistant.<br>We’ll help you with the rest.</p><a class="ec-text-link" :href="withBase('/getting-started/install')">Follow the installation guide <span aria-hidden="true">→</span></a></div>
      <ol class="ec-steps">
        <li><span aria-hidden="true">01</span><div><h3>Pick your panel.</h3><p>Choose a supported ESP32 touchscreen and a USB data cable.</p></div></li>
        <li><span aria-hidden="true">02</span><div><h3>Plug in. Install.</h3><p>Flash from Chrome or Edge on a computer, connect to WiFi, then pair with Home Assistant and enable actions.</p></div></li>
        <li><span aria-hidden="true">03</span><div><h3>Make yourself at home.</h3><p>Open the panel’s web page. Add your devices, arrange your cards and make it yours. No YAML required.</p></div></li>
      </ol>
    </section>

    <section id="next-steps" class="ec-finale ec-container" aria-labelledby="ec-finale-heading">
      <div class="ec-finale-photo"><img :src="withBase('/images/screens-hero.jpg')" alt="A collection of EspControl panels with custom control cards and radar images" width="1600" height="1131" loading="lazy"></div>
      <div class="ec-finale-copy"><p class="ec-eyebrow">YOUR NEXT HOME UPGRADE</p><h2 id="ec-finale-heading">Make room for<br>one more screen.</h2><p>A little less reaching for your phone.<br>A little more home at your fingertips.</p><a class="ec-button ec-primary" :href="withBase('/getting-started/install')">Get started with EspControl <span aria-hidden="true">↗</span></a></div>
    </section>
    <footer id="support-this-project" class="ec-footer ec-container"><div><strong>EspControl</strong><p>Made for the way you live with Home Assistant.</p></div><div class="ec-footer-links"><a :href="withBase('/guides/')">Documentation</a><a href="https://github.com/jtenniswood/espcontrol">GitHub <span aria-hidden="true">↗</span></a><a href="https://www.buymeacoffee.com/jtenniswood">Support the project <span aria-hidden="true">↗</span></a></div></footer>
  </main>
</template>

<style scoped>
.ec-home { --ec-bg: #f9fafc; --ec-surface: #eef0f5; --ec-ink: #17202b; --ec-muted: #566170; --ec-line: #d9dfe8; --ec-accent: #5b46b8; --ec-button: #6853ce; color: var(--ec-ink); background: var(--ec-bg); overflow: clip; font-family: var(--vp-font-family-base); }
:global(.dark .marketing-home .ec-home) { --ec-bg: #101216; --ec-surface: #191c23; --ec-ink: #f5f6fa; --ec-muted: #a8afbf; --ec-line: #2d323c; --ec-accent: #b8abff; --ec-button: #b8abff; }
:global(body:has(.marketing-home) .sp-support-btn) { display: none; }
.ec-container { width: min(1240px, calc(100% - 96px)); margin-inline: auto; }
.ec-home h1, .ec-home h2, .ec-home h3, .ec-home p, .ec-home figure { margin: 0; }
.ec-home a { text-decoration: none; }
.ec-home a:focus-visible, .ec-home button:focus-visible, .ec-feature-panel:focus-visible { outline: 3px solid var(--ec-accent); outline-offset: 5px; }
.ec-hero { display: grid; grid-template-columns: 1fr 1.08fr; gap: 44px; align-items: center; padding-block: 88px 64px; }
.ec-eyebrow { font-size: 11px; font-weight: 700; letter-spacing: .13em; line-height: 1.8; color: var(--ec-muted); }
.ec-hero .ec-eyebrow { display: flex; align-items: center; gap: 9px; }
.ec-status-dot { display: inline-block; flex-shrink: 0; width: 7px; height: 7px; border-radius: 50%; background: #48bf9a; }
.ec-hero h1 { font-size: clamp(48px, 5.1vw, 72px); font-weight: 650; line-height: 1.05; letter-spacing: -.065em; margin-block: 23px; }
.ec-hero h1 span { color: var(--ec-accent); }
.ec-intro { max-width: 440px; color: var(--ec-muted); font-size: 18px; line-height: 1.7; }
.ec-actions { display: flex; flex-wrap: wrap; gap: 12px; margin-top: 30px; }
.ec-button { min-height: 48px; display: inline-flex; align-items: center; justify-content: center; gap: 20px; padding: 14px 21px; border-radius: 9px; font-size: 14px; font-weight: 650; line-height: 1.4; transition: transform .2s, background-color .2s; }
.ec-button:hover { transform: translateY(-3px); }
.ec-primary { background: var(--ec-button); color: #fff; }
:global(.dark .marketing-home .ec-primary) { color: #19152d; }
.ec-secondary { color: var(--ec-ink); border: 1px solid var(--ec-line); }
.ec-secondary:hover { background: var(--ec-surface); }
.ec-hero-note { margin-top: 20px !important; color: var(--ec-muted); font-size: 12px; line-height: 1.8; }
.ec-hero-note a { color: var(--ec-ink); border-bottom: 1px solid var(--ec-line); }
.ec-hero-visual { position: relative; border: 1px solid var(--ec-line); border-radius: 20px; background: var(--ec-surface); box-shadow: 0 22px 60px #00000015; }
.ec-hero-visual img { display: block; width: 100%; height: auto; border-radius: 19px 19px 0 0; }
.ec-hero-visual figcaption { display: flex; align-items: center; gap: 9px; padding: 16px 20px; font-size: 11px; line-height: 1.6; color: var(--ec-muted); }
.ec-photo-label { position: absolute; top: 18px; left: 18px; padding: 7px 12px; color: #fff; background: #101216b8; border: 1px solid #ffffff30; border-radius: 30px; backdrop-filter: blur(12px); font-size: 10px; letter-spacing: .03em; }
.ec-proof { display: flex; justify-content: space-between; gap: 24px; border-top: 1px solid var(--ec-line); border-bottom: 1px solid var(--ec-line); padding-block: 25px; }
.ec-proof p { display: flex; align-items: center; gap: 12px; font-size: 13px; color: var(--ec-muted); }
.ec-proof p span { color: var(--ec-accent); font-size: 21px; }
.ec-section { padding-block: 100px; scroll-margin-top: 90px; }
.ec-home h2 { font-size: clamp(30px, 3.3vw, 44px); font-weight: 600; line-height: 1.15; letter-spacing: -.045em; }
.ec-section-heading { display: flex; align-items: flex-end; justify-content: space-between; gap: 40px; margin-bottom: 36px; }
.ec-section-heading h2, .ec-life-section h2, .ec-start h2 { margin-top: 12px; }
.ec-section-heading > p { max-width: 340px; color: var(--ec-muted); font-size: 14px; line-height: 1.8; }
.ec-tabs { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-bottom: 12px; }
.ec-tabs button { display: flex; align-items: center; gap: 13px; text-align: left; padding: 18px 20px; border-radius: 10px; border: 1px solid var(--ec-line); color: var(--ec-muted); background: transparent; font-size: 14px; font-weight: 600; cursor: pointer; transition: background-color .2s, color .2s; }
.ec-tabs button[aria-selected="true"] { background: var(--ec-accent); border-color: transparent; color: var(--ec-bg); }
.ec-tabs button:hover:not([aria-selected="true"]) { background: var(--ec-surface); color: var(--ec-ink); }
.ec-tab-number { font-size: 10px; opacity: .75; }
.ec-tab-arrow { margin-left: auto; }
.ec-feature-panel { display: grid; grid-template-columns: 1.15fr 1fr; background: var(--ec-surface); border: 1px solid var(--ec-line); border-radius: 16px; overflow: hidden; }
.ec-feature-image { display: flex; align-items: center; background: #121820; }
.ec-feature-image img { display: block; width: 100%; height: auto; }
.ec-feature-copy { display: flex; flex-direction: column; justify-content: center; padding: 40px; }
.ec-feature-copy h3 { white-space: pre-line; font-size: clamp(30px, 3vw, 42px); font-weight: 600; line-height: 1.12; letter-spacing: -.04em; margin-top: 16px; }
.ec-feature-copy > p:not(.ec-eyebrow) { color: var(--ec-muted); font-size: 14px; line-height: 1.8; margin-top: 20px; }
.ec-feature-copy ul { list-style: none; padding: 0; margin: 26px 0 0; }
.ec-feature-copy li + li { border-top: 1px solid var(--ec-line); }
.ec-feature-copy li a { display: flex; align-items: center; justify-content: space-between; padding: 12px 0; font-size: 13px; font-weight: 550; }
.ec-feature-copy li a:hover, .ec-text-link:hover, .ec-more a:hover, .ec-footer a:hover { color: var(--ec-accent); }
.ec-more { margin-top: 24px !important; color: var(--ec-muted); font-size: 12px; line-height: 1.8; }
.ec-more a { color: var(--ec-ink); font-weight: 550; margin-left: 6px; }
.ec-life-section { background: var(--ec-surface); }
.ec-life-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 48px; margin-top: 40px; }
.ec-life-symbol { display: inline-flex; align-items: center; justify-content: center; width: 48px; height: 48px; border-radius: 12px; font-size: 30px; margin-bottom: 24px; }
.ec-moon { color: #8d79de; background: #9e8ae720; }
.ec-photo { color: #288d73; background: #48bf9a20; }
.ec-lock { color: #c67b45; background: #e69b6220; }
.ec-life-card { display: flex; flex-direction: column; align-items: flex-start; }
.ec-life-card h3 { font-size: 21px; letter-spacing: -.03em; font-weight: 600; line-height: 1.3; }
.ec-life-card p { color: var(--ec-muted); font-size: 14px; line-height: 1.8; margin-top: 13px; margin-bottom: 24px; }
.ec-text-link { font-size: 12px; font-weight: 600; line-height: 1.7; }
.ec-life-card .ec-text-link { margin-top: auto; }
.ec-panel-grid { display: grid; grid-template-columns: repeat(5, minmax(0, 1fr)); gap: 15px; }
.ec-device-card { border: 1px solid var(--ec-line); border-radius: 12px; overflow: hidden; background: var(--ec-surface); transition: transform .2s, border-color .2s; }
.ec-device-card:hover { transform: translateY(-5px); border-color: var(--ec-accent); }
.ec-device-photo { aspect-ratio: 5 / 4; display: flex; align-items: center; background: #121820; }
.ec-device-photo img { width: 100%; height: 100%; object-fit: contain; }
.ec-device-copy { padding: 18px 15px; }
.ec-device-model { display: flex; justify-content: space-between; gap: 5px; font-size: 9px; line-height: 1.5; color: var(--ec-muted); letter-spacing: .03em; }
.ec-device-copy h3 { font-size: 22px; font-weight: 600; letter-spacing: -.03em; margin-top: 12px; }
.ec-device-copy > p:not(.ec-device-model) { color: var(--ec-muted); font-size: 12px; line-height: 1.6; min-height: 39px; margin-top: 8px; }
.ec-device-copy small { display: block; padding-top: 16px; margin-top: 18px; border-top: 1px solid var(--ec-line); color: var(--ec-muted); font-size: 10px; line-height: 1.5; }
.ec-start { display: grid; grid-template-columns: 1fr 1fr; gap: 70px; border-top: 1px solid var(--ec-line); }
.ec-start > div > p:not(.ec-eyebrow) { margin-block: 22px; font-size: 16px; color: var(--ec-muted); line-height: 1.8; }
.ec-steps { list-style: none; padding: 0; margin: 0; }
.ec-steps li { display: flex; gap: 26px; padding-block: 24px; border-bottom: 1px solid var(--ec-line); }
.ec-steps li:first-child { padding-top: 0; }
.ec-steps li > span { color: var(--ec-accent); font-size: 13px; font-variant-numeric: tabular-nums; }
.ec-steps h3 { font-size: 20px; font-weight: 600; letter-spacing: -.025em; }
.ec-steps p { font-size: 14px; line-height: 1.8; color: var(--ec-muted); margin-top: 8px; }
.ec-finale { display: grid; grid-template-columns: 1fr 1fr; border-radius: 18px; overflow: hidden; background: var(--ec-surface); border: 1px solid var(--ec-line); scroll-margin-top: 90px; }
.ec-finale-photo { display: flex; align-items: center; background: #121820; }
.ec-finale-photo img { width: 100%; height: auto; }
.ec-finale-copy { padding: 48px; align-self: center; }
.ec-finale h2 { margin-top: 14px; }
.ec-finale p:not(.ec-eyebrow) { margin-block: 22px; color: var(--ec-muted); font-size: 14px; line-height: 1.8; }
.ec-footer { display: flex; align-items: center; justify-content: space-between; gap: 30px; padding-block: 48px; }
.ec-footer strong { font-size: 19px; font-weight: 650; }
.ec-footer p { color: var(--ec-muted); font-size: 11px; line-height: 1.8; margin-top: 5px; }
.ec-footer-links { display: flex; gap: 25px; font-size: 12px; color: var(--ec-muted); }
@media (min-width: 1440px) { .ec-hero { padding-block: 110px 80px; } }
@media (max-width: 1100px) {
  .ec-container { width: calc(100% - 64px); }
  .ec-hero { gap: 28px; }
  .ec-hero h1 { font-size: 52px; }
  .ec-button { font-size: 12px; padding-inline: 16px; gap: 10px; }
  .ec-feature-copy { padding: 30px; }
  .ec-panel-grid { gap: 10px; }
  .ec-device-copy { padding: 16px 11px; }
  .ec-device-copy h3 { font-size: 19px; }
  .ec-device-copy small { font-size: 9px; }
  .ec-finale-copy { padding: 32px; }
}
@media (max-width: 767px) {
  .ec-container { width: calc(100% - 40px); }
  .ec-hero { grid-template-columns: 1fr; padding-block: 50px 32px; gap: 36px; }
  .ec-hero h1 { font-size: clamp(48px, 9vw, 64px); }
  .ec-intro { max-width: 520px; font-size: 16px; }
  .ec-hero-visual { max-width: 540px; width: 100%; justify-self: center; }
  .ec-proof { gap: 14px; flex-wrap: wrap; justify-content: flex-start; padding-block: 20px; }
  .ec-proof p { font-size: 11px; gap: 7px; }
  .ec-proof p span { font-size: 17px; }
  .ec-section { padding-block: 60px; }
  .ec-section-heading { display: block; margin-bottom: 26px; }
  .ec-section-heading > p { margin-top: 18px; max-width: 500px; }
  .ec-tabs { gap: 6px; }
  .ec-tabs button { padding: 13px 9px; font-size: 11px; justify-content: center; gap: 5px; }
  .ec-tab-number, .ec-tab-arrow { display: none; }
  .ec-feature-panel { grid-template-columns: 1fr; }
  .ec-feature-copy { padding: 28px; }
  .ec-feature-copy h3 { font-size: 36px; }
  .ec-more a { display: inline-block; margin-left: 0; }
  .ec-life-grid { grid-template-columns: 1fr; gap: 30px; margin-top: 30px; }
  .ec-life-card { padding-top: 26px; border-top: 1px solid var(--ec-line); }
  .ec-life-symbol { margin-bottom: 18px; }
  .ec-life-card p { max-width: 520px; margin-bottom: 15px; }
  .ec-panel-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px; }
  .ec-device-copy { padding: 18px 14px; }
  .ec-device-copy small { font-size: 10px; }
  .ec-start { grid-template-columns: 1fr; gap: 36px; }
  .ec-finale { grid-template-columns: 1fr; }
  .ec-finale-copy { padding: 32px 26px; }
  .ec-footer { align-items: flex-start; flex-direction: column; gap: 20px; padding-block: 32px; }
  .ec-footer-links { flex-wrap: wrap; gap: 20px; }
}
@media (prefers-reduced-motion: reduce) { .ec-home a, .ec-home button { transition: none; } .ec-home a:hover { transform: none; } }
</style>
