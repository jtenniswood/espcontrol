<script setup lang="ts">
import { withBase } from 'vitepress'

const arrow = 'M5 12h14m-6-6 6 6-6 6'
const features = [
  {
    title: 'Lights & scenes', detail: 'Brightness, colour and your favourite scenes.',
    href: '/card-types/lights', colour: 'lavender',
    icon: 'M9 18h6m-5 3h4M8.1 13.6a6 6 0 1 1 7.8 0c-.9.8-1.4 1.5-1.4 2.4h-5c0-.9-.5-1.6-1.4-2.4Z',
  },
  {
    title: 'Music', detail: 'Playback, volume and speaker groups.',
    href: '/card-types/media', colour: 'blue',
    icon: 'M9 18V5l11-2v13M9 8l11-2M9 18c0 1.7-1.6 3-3.5 3S2 19.7 2 18s1.6-3 3.5-3S9 16.3 9 18Zm11-2c0 1.7-1.6 3-3.5 3S13 17.7 13 16s1.6-3 3.5-3S20 14.3 20 16Z',
  },
  {
    title: 'Climate', detail: 'Heating, cooling and target temperatures.',
    href: '/card-types/climate', colour: 'coral',
    icon: 'M9 14.5V5a3 3 0 0 1 6 0v9.5a5 5 0 1 1-6 0ZM12 8v10m0 0a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3Z',
  },
  {
    title: 'Weather', detail: 'Current conditions and daily forecasts.',
    href: '/card-types/weather', colour: 'sky',
    icon: 'M8 14a4 4 0 1 1 4-5M7 4V2M3 6 1.5 4.5M13 6l1.5-1.5M2 11H0M9 20a4 4 0 1 1 1-7.9 5 5 0 0 1 9.5 1.9 3 3 0 1 1 .5 6Z',
  },
  {
    title: 'Cameras & images', detail: 'Camera snapshots and animated GIFs.',
    href: '/card-types/cameras', colour: 'navy',
    icon: 'M3 6h4l2-3h6l2 3h4v14H3ZM16 12a4 4 0 1 1-8 0 4 4 0 0 1 8 0Z',
  },
  {
    title: 'Your layout', detail: 'Resize cards. Add pages for each room.',
    href: '/features/setup', colour: 'mint',
    icon: 'M3 3h10v10H3ZM17 3h4v10h-4ZM3 17h10v4H3ZM17 17h4v4h-4Z',
  },
]
const extras = [
  { title: 'Blinds & covers', href: '/card-types/covers' },
  { title: 'Locks', href: '/card-types/locks' },
  { title: 'Alarms', href: '/card-types/alarms' },
  { title: 'Live sensors', href: '/card-types/sensors' },
  { title: 'Photos & screensavers', href: '/features/screensaver' },
]
const panels = [
  { name: '4″ S3', model: '4848S040', image: '4848s040-hero.jpg', height: 1167, href: '/screens/4848s040' },
  { name: '4″ P4', model: 'ESP32-P4 86', image: 'p4-86-hero.jpg', height: 1316, href: '/screens/p4-86' },
  { name: '4.3″ P4', model: 'JC4880P443', image: 'jc4880p443-hero.jpg', height: 1228, href: '/screens/jc4880p443' },
  { name: '7″ P4', model: 'JC1060P470', image: 'jc1060p470-hero.jpg', height: 1137, href: '/screens/jc1060p470' },
  { name: '10.1″ P4', model: 'JC8012P4A1', image: 'jc8012p4a1-hero.jpg', height: 1177, href: '/screens/jc8012p4a1' },
]
</script>

<template>
  <main class="ec-home">
    <header class="ec-intro ec-wrap">
      <div>
        <p class="ec-eyebrow"><span aria-hidden="true"></span> ESPCONTROL FOR HOME ASSISTANT</p>
        <h1 id="espcontrol-no-code-home-assistant-touchscreen">Lights, music, climate.<br><span>All on one screen.</span></h1>
        <p class="ec-description">Custom touchscreen controls. Set up in your browser, no YAML.</p>
      </div>
      <div id="how-it-works" class="ec-intro-actions">
        <a id="next-steps" class="ec-button" :href="withBase('/getting-started/install')">Install EspControl <svg viewBox="0 0 24 24" aria-hidden="true"><path :d="arrow" /></svg></a>
        <a class="ec-secondary" href="#where-to-buy">Choose a screen <span aria-hidden="true">↓</span></a>
      </div>
    </header>

    <section id="what-you-can-build" class="ec-features ec-wrap" aria-label="What EspControl screens support">
      <div id="main-features" class="ec-feature-grid">
        <figure class="ec-showcase-photo">
          <div class="ec-photo-top"><span>EspControl</span><span>On real hardware <i aria-hidden="true"></i></span></div>
          <img :src="withBase('/images/home_screen_hero.jpg')" alt="EspControl touchscreen panels showing temperature, lighting, music and weather cards" width="1600" height="1288" fetchpriority="high">
          <figcaption>Built around your Home Assistant devices.</figcaption>
        </figure>
        <a v-for="feature in features" :key="feature.title" class="ec-feature-tile" :class="`ec-${feature.colour}`" :href="withBase(feature.href)">
          <div class="ec-tile-top"><svg viewBox="0 0 24 24" aria-hidden="true"><path :d="feature.icon" /></svg><span class="ec-tile-arrow" aria-hidden="true">↗</span></div>
          <h2>{{ feature.title }}</h2>
          <p>{{ feature.detail }}</p>
        </a>
      </div>
      <nav id="card-types" class="ec-extra-features" aria-label="More supported features"><span>And more</span><a v-for="extra in extras" :key="extra.href" :href="withBase(extra.href)">{{ extra.title }} <span aria-hidden="true">↗</span></a><a class="ec-all-cards" :href="withBase('/card-types/')">All card types <span aria-hidden="true">→</span></a></nav>
      <div class="ec-setup-strip"><p><strong>Make it your screen.</strong> Drag cards, choose colours, set sleep schedules.</p><a :href="withBase('/features/setup')">See how setup works <span aria-hidden="true">→</span></a></div>
    </section>

    <section id="where-to-buy" class="ec-hardware ec-wrap" aria-labelledby="ec-hardware-heading">
      <div class="ec-section-heading"><div><p class="ec-eyebrow">FIVE PANELS. ONE FAMILIAR INTERFACE.</p><h2 id="ec-hardware-heading">Pick your screen.</h2></div><a class="ec-compare" :href="withBase('/screens/')">Compare sizes & capabilities <span aria-hidden="true">↗</span></a></div>
      <div class="ec-panel-grid"><a v-for="panel in panels" :key="panel.model" class="ec-panel" :href="withBase(panel.href)"><div class="ec-panel-image"><img :src="withBase(`/images/${panel.image}`)" :alt="`${panel.model} running EspControl`" width="1600" :height="panel.height" loading="lazy"></div><div class="ec-panel-name"><h3>{{ panel.name }}</h3><span aria-hidden="true">↗</span></div><p>{{ panel.model }}</p></a></div>
      <p class="ec-hardware-note">Image capacity and hardware vary by model. Check the device guide before installing.</p>
    </section>
    <footer id="support-this-project" class="ec-footer ec-wrap"><strong>EspControl</strong><nav aria-label="Homepage footer"><a :href="withBase('/guides/')">Read the docs</a><a href="https://github.com/jtenniswood/espcontrol">GitHub ↗</a><a href="https://www.buymeacoffee.com/jtenniswood">Support the project ↗</a></nav></footer>
  </main>
</template>

<style scoped>
.ec-home { --ec-bg: #f7f9fd; --ec-ink: #172c46; --ec-muted: #53657b; --ec-line: #dce4ee; --ec-blue: #0755df; --ec-panel: #fff; color: var(--ec-ink); background: var(--ec-bg); font-family: var(--vp-font-family-base); }
:global(.dark .marketing-home .ec-home) { --ec-bg: #101c2c; --ec-ink: #e7f0fa; --ec-muted: #a5b7cd; --ec-line: #2c3f56; --ec-blue: #91bfff; --ec-panel: #182a40; }
:global(body:has(.marketing-home) .sp-support-btn) { display: none; }
.ec-home *, .ec-home *::before, .ec-home *::after { box-sizing: border-box; }
.ec-home h1, .ec-home h2, .ec-home h3, .ec-home p, .ec-home figure { margin: 0; }
.ec-home a { text-decoration: none; }
.ec-home a:focus-visible { outline: 3px solid #f17454; outline-offset: 5px; }
.ec-home svg { width: 24px; height: 24px; fill: none; stroke: currentColor; stroke-width: 1.6; stroke-linecap: round; stroke-linejoin: round; }
.ec-wrap { width: min(1320px, calc(100% - 96px)); margin-inline: auto; }
.ec-intro { display: flex; justify-content: space-between; align-items: flex-end; gap: 40px; padding-block: 38px 30px; }
.ec-eyebrow { display: flex; align-items: center; gap: 8px; color: var(--ec-muted); font-size: 10px; line-height: 1.7; font-weight: 600; letter-spacing: .1em; }
.ec-intro .ec-eyebrow > span { width: 7px; height: 7px; border-radius: 50%; background: #15a6c0; }
.ec-home h1 { font-size: clamp(38px, 4.3vw, 58px); font-weight: 600; letter-spacing: -.055em; line-height: 1.06; margin-top: 18px; }
.ec-home h1 > span { color: var(--ec-blue); }
.ec-description { font-size: 14px; line-height: 1.7; color: var(--ec-muted); margin-top: 18px !important; }
.ec-intro-actions { display: flex; flex-direction: column; align-items: center; gap: 15px; flex-shrink: 0; padding-bottom: 3px; }
.ec-button { display: flex; align-items: center; justify-content: center; gap: 25px; min-height: 50px; padding: 13px 20px; background: #0755df; border-radius: 12px; color: #fff; font-size: 13px; font-weight: 600; }
.ec-button:hover { background: #0647bc; }
.ec-button svg { width: 20px; height: 20px; }
.ec-secondary { font-size: 11px; color: var(--ec-muted); }
.ec-secondary > span { margin-left: 10px; }
.ec-feature-grid { display: grid; grid-template-columns: .85fr .85fr 1.15fr 1.15fr; gap: 12px; }
.ec-showcase-photo { grid-column: 1 / 3; grid-row: 1 / 4; display: flex; flex-direction: column; background: #162b42; color: #e7f0fa; border-radius: 24px; overflow: hidden; }
.ec-photo-top { display: flex; align-items: center; justify-content: space-between; padding: 19px 22px; gap: 15px; }
.ec-photo-top > span:first-child { font-size: 16px; font-weight: 600; letter-spacing: -.03em; }
.ec-photo-top > span:last-child { display: flex; align-items: center; gap: 8px; font-size: 10px; color: #b3c4d7; }
.ec-photo-top i { width: 6px; height: 6px; border-radius: 50%; background: #71dfd2; }
.ec-showcase-photo img { width: 100%; height: 0; min-height: 0; flex: 1; display: block; object-fit: contain; }
.ec-showcase-photo figcaption { display: flex; flex-wrap: wrap; justify-content: space-between; gap: 14px; align-items: center; padding: 20px 22px; font-size: 10px; line-height: 1.6; color: #b3c4d7; }
.ec-feature-tile { display: flex; flex-direction: column; min-width: 0; min-height: 145px; border-radius: 24px; padding: 19px 21px; transition: transform .2s, box-shadow .2s; }
.ec-feature-tile:hover { transform: translateY(-3px); box-shadow: 0 8px 20px #132d4c20; }
.ec-tile-top { display: flex; align-items: center; justify-content: space-between; gap: 15px; margin-bottom: 17px; }
.ec-tile-top > svg { width: 28px; height: 28px; }
.ec-tile-arrow { font-size: 17px; opacity: .65; }
.ec-feature-tile h2 { font-size: 19px; font-weight: 600; line-height: 1.2; letter-spacing: -.03em; }
.ec-feature-tile p { margin-top: 7px; font-size: 12px; line-height: 1.5; opacity: .9; }
.ec-lavender { background: #ddd4fa; color: #42316c; }
.ec-blue { background: #1458ed; color: #fff; }
.ec-coral { background: #f7b8a7; color: #6a3026; }
.ec-sky { background: #c6eafb; color: #1c4c6d; }
.ec-navy { background: #244565; color: #e9f4ff; }
.ec-mint { background: #bfe8db; color: #235b4b; }
.ec-extra-features { display: flex; flex-wrap: wrap; align-items: center; justify-content: space-between; gap: 16px; padding-block: 24px; color: var(--ec-muted); font-size: 11px; line-height: 1.6; }
.ec-extra-features > span { font-size: 10px; font-weight: 600; }
.ec-extra-features a > span { margin-left: 6px; }
.ec-extra-features a:hover, .ec-secondary:hover { color: var(--ec-blue); }
.ec-extra-features .ec-all-cards { color: var(--ec-blue); font-weight: 600; }
.ec-setup-strip { display: flex; justify-content: space-between; align-items: center; gap: 25px; padding: 22px 25px; background: var(--ec-panel); border: 1px solid var(--ec-line); border-radius: 16px; font-size: 12px; line-height: 1.6; }
.ec-setup-strip strong { font-weight: 600; margin-right: 8px; }
.ec-setup-strip p { color: var(--ec-muted); }
.ec-setup-strip p strong { color: var(--ec-ink); }
.ec-setup-strip a { color: var(--ec-blue); white-space: nowrap; font-weight: 550; }
.ec-setup-strip a > span { margin-left: 12px; }
.ec-hardware { padding-block: 55px 40px; scroll-margin-top: 85px; }
.ec-section-heading { display: flex; justify-content: space-between; align-items: flex-end; gap: 25px; }
.ec-section-heading .ec-eyebrow { font-size: 9px; }
.ec-section-heading h2 { font-size: 32px; line-height: 1.2; font-weight: 600; letter-spacing: -.04em; margin-top: 10px; }
.ec-compare { color: var(--ec-blue); font-size: 11px; line-height: 1.7; }
.ec-compare > span { margin-left: 10px; }
.ec-panel-grid { display: grid; grid-template-columns: repeat(5, minmax(0, 1fr)); gap: 17px; margin-top: 25px; }
.ec-panel { min-width: 0; }
.ec-panel-image { aspect-ratio: 1.5; background: #162b42; border-radius: 14px; overflow: hidden; }
.ec-panel-image img { display: block; width: 100%; height: 100%; object-fit: contain; transition: opacity .2s; }
.ec-panel-name { display: flex; align-items: center; justify-content: space-between; gap: 10px; margin-top: 13px; }
.ec-panel-name h3 { font-size: 18px; font-weight: 600; letter-spacing: -.025em; line-height: 1.2; }
.ec-panel-name > span { color: var(--ec-muted); font-size: 16px; }
.ec-panel > p { color: var(--ec-muted); font-size: 9px; margin-top: 5px; }
.ec-panel:hover .ec-panel-image img { opacity: .8; }
.ec-panel:hover .ec-panel-name { color: var(--ec-blue); }
.ec-hardware-note { margin-top: 23px !important; font-size: 10px; line-height: 1.7; color: var(--ec-muted); }
.ec-footer { display: flex; align-items: center; justify-content: space-between; gap: 25px; border-top: 1px solid var(--ec-line); padding-block: 27px; }
.ec-footer > strong { font-size: 18px; font-weight: 600; letter-spacing: -.03em; }
.ec-footer nav { display: flex; flex-wrap: wrap; gap: 22px; font-size: 11px; color: var(--ec-muted); }
.ec-footer a:hover { color: var(--ec-blue); }
@media (max-width: 1100px) {
  .ec-wrap { width: calc(100% - 64px); }
  .ec-intro { gap: 25px; padding-top: 38px; }
  .ec-feature-grid { grid-template-columns: 1fr 1fr 1fr 1fr; gap: 10px; }
  .ec-feature-tile { padding: 17px; }
  .ec-feature-tile h2 { font-size: 17px; }
  .ec-feature-tile p { font-size: 10px; }
  .ec-photo-top { padding: 17px; }
  .ec-showcase-photo figcaption { padding: 17px; }
  .ec-panel-grid { gap: 12px; }
}
@media (max-width: 767px) {
  .ec-wrap { width: calc(100% - 36px); }
  .ec-intro { display: block; padding-block: 34px 26px; }
  .ec-eyebrow { font-size: 8px; }
  .ec-home h1 { font-size: clamp(32px, 8.5vw, 47px); margin-top: 15px; line-height: 1.08; }
  .ec-description { font-size: 12px; max-width: 350px; margin-top: 15px !important; }
  .ec-intro-actions { flex-direction: row; justify-content: flex-start; gap: 22px; margin-top: 22px; padding: 0; }
  .ec-button { font-size: 11px; gap: 15px; padding-inline: 15px; min-height: 46px; }
  .ec-secondary { font-size: 10px; }
  .ec-feature-grid { grid-template-columns: 1fr 1fr; gap: 10px; }
  .ec-showcase-photo { grid-column: 1 / -1; grid-row: auto; order: 1; margin-top: 4px !important; }
  .ec-showcase-photo img { height: auto; flex: none; }
  .ec-feature-tile { border-radius: 20px; min-height: 148px; padding: 17px; }
  .ec-tile-top { margin-bottom: 17px; }
  .ec-tile-top > svg { width: 27px; height: 27px; }
  .ec-feature-tile h2 { font-size: 17px; }
  .ec-feature-tile p { font-size: 10px; line-height: 1.5; }
  .ec-photo-top { padding: 18px; }
  .ec-photo-top > span:first-child { font-size: 15px; }
  .ec-photo-top > span:last-child { font-size: 9px; }
  .ec-showcase-photo figcaption { padding: 18px; font-size: 10px; }
  .ec-extra-features { justify-content: flex-start; column-gap: 20px; row-gap: 13px; padding-block: 22px; font-size: 10px; }
  .ec-extra-features > span { width: 100%; }
  .ec-setup-strip { display: block; padding: 20px; font-size: 12px; }
  .ec-setup-strip strong { display: block; margin: 0 0 4px; }
  .ec-setup-strip a { display: inline-block; margin-top: 13px; font-size: 11px; }
  .ec-hardware { padding-block: 40px 30px; }
  .ec-section-heading { display: block; }
  .ec-section-heading h2 { font-size: 30px; }
  .ec-section-heading .ec-eyebrow { font-size: 8px; }
  .ec-compare { display: inline-block; margin-top: 14px; font-size: 11px; }
  .ec-panel-grid { grid-template-columns: 1fr 1fr; gap: 23px 14px; margin-top: 23px; }
  .ec-panel-name h3 { font-size: 20px; }
  .ec-panel > p { font-size: 9px; }
  .ec-hardware-note { font-size: 10px; }
  .ec-footer { align-items: flex-start; flex-direction: column; gap: 20px; }
  .ec-footer nav { gap: 20px; font-size: 10px; }
}
@media (max-width: 360px) {
  .ec-intro-actions { gap: 17px; }
  .ec-feature-tile { padding: 15px; }
  .ec-feature-tile h2 { font-size: 16px; }
  .ec-photo-top > span:last-child { font-size: 8px; }
}
@media (prefers-reduced-motion: reduce) { .ec-home *, .ec-home *::before, .ec-home *::after { transition: none !important; } .ec-feature-tile:hover { transform: none; } }
</style>
