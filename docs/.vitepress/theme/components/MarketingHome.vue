<script setup lang="ts">
import { ref } from 'vue'
import { withBase } from 'vitepress'

const activeScene = ref(0)
const activeFeature = ref(0)
const scenes = [
  {
    name: 'Control', summary: 'Lights, music & climate', image: 'home_screen_hero.jpg', height: 1288,
    alt: 'EspControl panels showing lighting, music and temperature controls',
    features: [
      { label: 'Climate', x: 40, y: 54, mx: 30, my: 47, title: 'Get the room just right.', detail: 'Adjust heating, cooling and target temperatures.', href: '/card-types/climate', link: 'Climate controls' },
      { label: 'Lights', x: 52, y: 22, mx: 52, my: 39, title: 'Set the light. Set the mood.', detail: 'Dimming, colour temperature and colour presets.', href: '/card-types/lights', link: 'Lighting controls' },
      { label: 'Music', x: 86, y: 14, mx: 83, my: 33, title: 'A little louder?', detail: 'Playback, volume and Home Assistant speaker groups.', href: '/card-types/media', link: 'Media controls' },
    ],
  },
  {
    name: 'See', summary: 'Weather, cameras & sensors', image: 'jc8012p4a1-hero.jpg', height: 1177,
    alt: 'A 10.1-inch panel showing rain radar, daily forecasts and temperature readings',
    features: [
      { label: 'Radar & images', x: 44, y: 46, mx: 38, my: 49, title: 'Watch the weather roll in.', detail: 'Camera snapshots, images and animated GIFs.', href: '/card-types/cameras', link: 'Cameras & images' },
      { label: 'Forecasts', x: 83, y: 76, mx: 80, my: 78, title: 'Know what’s coming.', detail: 'Current conditions and daily weather forecasts.', href: '/card-types/weather', link: 'Weather cards' },
      { label: 'Sensors', x: 66, y: 30, mx: 72, my: 42, title: 'Keep an eye on things.', detail: 'Live temperatures, energy readings and device states.', href: '/card-types/sensors', link: 'Sensor cards' },
    ],
  },
  {
    name: 'Make it yours', summary: 'Layouts, rooms & themes', image: '4848s040-subpage.jpg', height: 1238,
    alt: 'A room subpage on a 4-inch panel, with a large Aircon card and smaller controls',
    features: [
      { label: 'Resize cards', x: 65, y: 48, mx: 64, my: 48, title: 'Make space for what matters.', detail: 'Drag and resize cards in your browser. No YAML.', href: '/features/setup', link: 'Build your layout' },
      { label: 'Room pages', x: 45, y: 25, mx: 32, my: 37, title: 'A page for every room.', detail: 'Group controls into subpages, with a tap to go back.', href: '/features/subpages', link: 'Create subpages' },
      { label: 'Colours & icons', x: 37, y: 67, mx: 43, my: 76, title: 'A screen that feels like yours.', detail: 'Choose card colours, icons, curves and themes.', href: '/features/appearance', link: 'Personalise your screen' },
    ],
  },
]
const panels = [
  { name: '4″ S3', model: '4848S040', href: '/screens/4848s040' },
  { name: '4″ P4', model: 'ESP32-P4 86', href: '/screens/p4-86' },
  { name: '4.3″ P4', model: 'JC4880P443', href: '/screens/jc4880p443' },
  { name: '7″ P4', model: 'JC1060P470', href: '/screens/jc1060p470' },
  { name: '10.1″ P4', model: 'JC8012P4A1', href: '/screens/jc8012p4a1' },
]
function selectScene(index: number) {
  activeScene.value = index
  activeFeature.value = 0
}
function onSceneKeydown(event: KeyboardEvent) {
  let next = activeScene.value
  if (event.key === 'ArrowRight') next = (next + 1) % scenes.length
  else if (event.key === 'ArrowLeft') next = (next + scenes.length - 1) % scenes.length
  else if (event.key === 'Home') next = 0
  else if (event.key === 'End') next = scenes.length - 1
  else return
  event.preventDefault()
  selectScene(next)
  const tabs = (event.currentTarget as HTMLElement).parentElement?.querySelectorAll<HTMLButtonElement>('[role="tab"]')
  tabs?.[next]?.focus()
}
</script>

<template>
  <main class="ec-home">
    <section id="what-you-can-build" class="ec-explorer" aria-labelledby="espcontrol-no-code-home-assistant-touchscreen">
      <div class="ec-stage">
        <div v-for="(scene, index) in scenes" v-show="activeScene === index" :id="`ec-scene-${index}`" :key="scene.name" class="ec-scene" :style="{ '--image-ratio': 1600 / scene.height }" role="tabpanel" :aria-labelledby="`ec-scene-tab-${index}`">
          <img class="ec-scene-image" :src="withBase(`/images/${scene.image}`)" :alt="scene.alt" width="1600" :height="scene.height" :fetchpriority="index === 0 ? 'high' : undefined" :loading="index === 0 ? 'eager' : 'lazy'">
          <div class="ec-image-shade" aria-hidden="true"></div>
          <div class="ec-callouts" aria-label="Explore features on this screen">
            <button v-for="(feature, featureIndex) in scene.features" :key="feature.label" class="ec-callout" :class="{ 'is-active': activeFeature === featureIndex }" type="button" :style="{ '--x': `${feature.x}%`, '--y': `${feature.y}%`, '--mx': `${feature.mx}%`, '--my': `${feature.my}%` }" :aria-label="`Explore ${feature.label}`" :aria-pressed="activeFeature === featureIndex" :aria-controls="`ec-feature-detail-${index}`" @click="activeFeature = featureIndex"><span class="ec-marker" aria-hidden="true"><span></span></span><span class="ec-callout-label">{{ feature.label }}</span></button>
          </div>
          <div :id="`ec-feature-detail-${index}`" class="ec-feature-detail" aria-live="polite" aria-atomic="true">
            <p class="ec-detail-label">{{ scene.features[activeFeature].label }}</p>
            <h2>{{ scene.features[activeFeature].title }}</h2>
            <p>{{ scene.features[activeFeature].detail }}</p>
            <a :href="withBase(scene.features[activeFeature].href)">{{ scene.features[activeFeature].link }} <span aria-hidden="true">↗</span></a>
          </div>
        </div>
        <header class="ec-intro">
          <p class="ec-eyebrow">ESPCONTROL <span aria-hidden="true">/</span> MADE FOR HOME ASSISTANT</p>
          <h1 id="espcontrol-no-code-home-assistant-touchscreen">Home Assistant.<br><em>Off your phone.</em></h1>
          <p>Real controls. A dedicated touchscreen.<br>Set up in your browser. No YAML.</p>
          <a id="next-steps" class="ec-install" :href="withBase('/getting-started/install')">Install EspControl <span aria-hidden="true">↗</span></a>
        </header>
        <div class="ec-explore-hint" aria-hidden="true"><span></span> TAP A + TO EXPLORE</div>
        <p class="ec-photo-credit">REAL PANELS. RUNNING ESPCONTROL.</p>
      </div>
      <nav id="main-features" class="ec-scene-tabs" role="tablist" aria-label="Explore what the screens support">
        <button v-for="(scene, index) in scenes" :id="`ec-scene-tab-${index}`" :key="scene.name" type="button" role="tab" :aria-selected="activeScene === index" :aria-controls="`ec-scene-${index}`" :tabindex="activeScene === index ? 0 : -1" @click="selectScene(index)" @keydown="onSceneKeydown"><span class="ec-scene-number" aria-hidden="true">0{{ index + 1 }}</span><span><strong>{{ scene.name }}</strong><small>{{ scene.summary }}</small></span><span class="ec-scene-arrow" aria-hidden="true">↗</span></button>
      </nav>
    </section>

    <nav id="card-types" class="ec-more" aria-label="More screen features"><span>THERE’S MORE ON SCREEN</span><a :href="withBase('/card-types/covers')">Blinds</a><a :href="withBase('/card-types/locks')">Locks</a><a :href="withBase('/card-types/alarms')">Alarms</a><a :href="withBase('/features/screensaver')">Photos & screensavers</a><a :href="withBase('/features/screen-schedule')">Sleep schedules</a><a :href="withBase('/card-types/')">All card types <span aria-hidden="true">↗</span></a></nav>

    <section id="where-to-buy" class="ec-hardware" aria-labelledby="ec-hardware-heading"><div class="ec-hardware-title"><p class="ec-eyebrow">PICK YOUR HARDWARE</p><h2 id="ec-hardware-heading">Small screen.<br>Big screen. <em>Your call.</em></h2><a :href="withBase('/screens/')">Compare the panels <span aria-hidden="true">↗</span></a></div><div class="ec-panel-list"><a v-for="panel in panels" :key="panel.model" :href="withBase(panel.href)"><strong>{{ panel.name }}</strong><span>{{ panel.model }}</span><span aria-hidden="true">↗</span></a><p>Image capacity and hardware vary by model.<br>Check the device guide before installing.</p></div></section>
    <footer id="support-this-project" class="ec-footer"><strong>EspControl.</strong><nav aria-label="Homepage footer"><a :href="withBase('/guides/')">Documentation</a><a href="https://github.com/jtenniswood/espcontrol">GitHub ↗</a><a href="https://www.buymeacoffee.com/jtenniswood">Support the project ↗</a></nav><a id="how-it-works" :href="withBase('/getting-started/install')">Ready? Install EspControl <span aria-hidden="true">↗</span></a></footer>
  </main>
</template>

<style scoped>
.ec-home { background: #0c1011; color: #f6f5ef; font-family: var(--vp-font-family-base); }
:global(body:has(.marketing-home) .sp-support-btn) { display: none; }
.ec-home *, .ec-home *::before, .ec-home *::after { box-sizing: border-box; }
.ec-home h1, .ec-home h2, .ec-home p { margin: 0; }
.ec-home a { text-decoration: none; }
.ec-home a:focus-visible, .ec-home button:focus-visible { outline: 3px solid #b8f1dc; outline-offset: 5px; }
.ec-home em { font-family: Georgia, 'Times New Roman', serif; font-weight: 400; }
.ec-stage { position: relative; height: clamp(560px, calc(100svh - 160px), 820px); isolation: isolate; overflow: hidden; }
.ec-scene { position: absolute; inset: 0; }
.ec-scene-image { width: 100%; height: 100%; object-fit: cover; object-position: 50% 55%; display: block; animation: ec-image-enter .6s ease-out; }
.ec-image-shade { position: absolute; inset: 0; background: linear-gradient(90deg, #080e13e8 0%, #080e137d 27%, #080e1300 55%), linear-gradient(0deg, #080e13cc 0%, #080e1300 35%, #080e1330 100%); }
.ec-intro { position: absolute; z-index: 2; top: 36px; left: 4.5%; pointer-events: none; }
.ec-eyebrow { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 9px; line-height: 1.8; letter-spacing: .12em; }
.ec-intro .ec-eyebrow { color: #d5e6e4; }
.ec-intro .ec-eyebrow > span { margin-inline: 11px; color: #94c4c5; }
.ec-intro h1 { font-size: clamp(49px, 5vw, 72px); line-height: 1.05; letter-spacing: -.06em; font-weight: 550; margin-top: 18px; text-shadow: 0 2px 20px #00000020; }
.ec-intro h1 em { color: #c6ece0; }
.ec-intro > p:not(.ec-eyebrow) { margin-top: 16px; font-size: 13px; line-height: 1.8; color: #d4dedf; }
.ec-install { pointer-events: auto; display: inline-flex; align-items: center; gap: 45px; padding: 15px 20px; background: #f6f5ef; color: #162429; font-size: 12px; font-weight: 600; margin-top: 20px; }
.ec-install:hover { background: #c6ece0; }
.ec-explore-hint { position: absolute; z-index: 2; right: 4.5%; top: 35px; display: flex; align-items: center; gap: 10px; color: #f6f5ef; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 9px; letter-spacing: .1em; text-shadow: 0 1px 5px #000; }
.ec-explore-hint > span { width: 5px; height: 5px; border-radius: 50%; background: #c6ece0; box-shadow: 0 0 0 5px #c6ece025; }
.ec-photo-credit { position: absolute; bottom: 24px; right: 4.5%; z-index: 2; color: #d2dcdc; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 8px; letter-spacing: .1em; text-shadow: 0 1px 5px #000; }
.ec-callout { position: absolute; top: var(--y); left: var(--x); z-index: 3; display: flex; align-items: center; gap: 8px; min-height: 44px; padding: 0 12px 0 0; cursor: pointer; color: #fff; text-shadow: 0 1px 5px #000; transform: translate(-22px, -22px); }
.ec-marker { display: grid; place-items: center; width: 42px; height: 42px; border-radius: 50%; border: 1px solid #ffffffaa; background: #0b161e85; backdrop-filter: blur(8px); transition: color .2s, background-color .2s, transform .2s; }
.ec-marker > span { position: relative; width: 12px; height: 12px; }
.ec-marker > span::before, .ec-marker > span::after { content: ''; position: absolute; width: 12px; height: 1px; top: 5px; left: 0; background: currentColor; }
.ec-marker > span::after { transform: rotate(90deg); }
.ec-callout-label { background: #0b161ebf; padding: 5px 8px; font-size: 11px; line-height: 1.4; backdrop-filter: blur(8px); }
.ec-callout.is-active .ec-marker { background: #c6ece0; color: #162429; border-color: #c6ece0; text-shadow: none; box-shadow: 0 0 0 7px #c6ece02b; }
.ec-callout:hover .ec-marker { transform: scale(1.08); background: #c6ece0; color: #162429; }
.ec-feature-detail { position: absolute; z-index: 2; left: 4.5%; bottom: 32px; max-width: 365px; padding-left: 20px; border-left: 2px solid #c6ece0; }
.ec-detail-label { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; text-transform: uppercase; letter-spacing: .1em; color: #c6ece0; font-size: 9px; line-height: 1.8; }
.ec-feature-detail h2 { margin-top: 9px; font-size: 26px; line-height: 1.2; letter-spacing: -.035em; font-weight: 550; }
.ec-feature-detail > p:not(.ec-detail-label) { margin-top: 10px; font-size: 12px; line-height: 1.7; color: #d4dedf; }
.ec-feature-detail a { display: inline-flex; align-items: center; gap: 30px; margin-top: 19px; font-size: 11px; font-weight: 550; padding-bottom: 5px; border-bottom: 1px solid #ffffff50; }
.ec-feature-detail a:hover { color: #c6ece0; }
.ec-scene-tabs { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); padding-inline: 4.5%; border-bottom: 1px solid #ffffff25; background: #0c1011; }
.ec-scene-tabs button { display: flex; align-items: center; gap: 20px; min-width: 0; min-height: 96px; text-align: left; border-top: 2px solid transparent; padding: 22px 24px 22px 0; cursor: pointer; color: #8d9b9c; transition: color .2s, border-color .2s; }
.ec-scene-tabs button[aria-selected="true"] { color: #f6f5ef; border-top-color: #c6ece0; }
.ec-scene-tabs button:hover { color: #c6ece0; }
.ec-scene-number { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 10px; color: #83999a; align-self: flex-start; padding-top: 5px; }
.ec-scene-tabs strong { display: block; font-size: 24px; letter-spacing: -.035em; line-height: 1.2; font-weight: 500; }
.ec-scene-tabs small { display: block; font-size: 10px; line-height: 1.7; margin-top: 6px; }
.ec-scene-arrow { margin-left: auto; font-size: 23px; }
.ec-more { display: flex; align-items: center; flex-wrap: wrap; justify-content: space-between; gap: 20px; padding: 28px 4.5%; font-size: 11px; color: #c1cbca; border-bottom: 1px solid #ffffff25; }
.ec-more > span { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 8px; letter-spacing: .08em; color: #84999a; }
.ec-more > a:last-child { color: #c6ece0; }
.ec-more a:hover { color: #fff; }
.ec-hardware { display: grid; grid-template-columns: 1fr 1fr; gap: 100px; padding: 65px 4.5%; background: #e8eeea; color: #17282a; scroll-margin-top: 80px; }
.ec-hardware .ec-eyebrow { color: #5a7575; }
.ec-hardware h2 { font-size: clamp(34px, 4vw, 54px); line-height: 1.1; letter-spacing: -.05em; font-weight: 550; margin-top: 18px; }
.ec-hardware h2 em { white-space: nowrap; }
.ec-hardware-title > a { display: inline-block; font-size: 11px; border-bottom: 1px solid #17282a60; padding-bottom: 5px; margin-top: 25px; }
.ec-hardware-title > a span { margin-left: 25px; }
.ec-panel-list a { display: flex; align-items: center; gap: 20px; padding-block: 15px; border-bottom: 1px solid #17282a35; }
.ec-panel-list a:first-child { padding-top: 0; }
.ec-panel-list strong { min-width: 90px; font-size: 22px; font-weight: 500; letter-spacing: -.03em; }
.ec-panel-list a > span:not(:last-child) { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 10px; color: #5a7575; }
.ec-panel-list a > span:last-child { margin-left: auto; font-size: 19px; }
.ec-panel-list a:hover { color: #00645c; }
.ec-panel-list > p { font-size: 10px; line-height: 1.7; color: #5a7575; margin-top: 18px; }
.ec-footer { display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap; gap: 25px; padding: 30px 4.5%; }
.ec-footer > strong { font-size: 26px; font-weight: 550; letter-spacing: -.055em; }
.ec-footer nav { display: flex; flex-wrap: wrap; gap: 24px; font-size: 10px; color: #a8baba; }
.ec-footer > a { font-size: 11px; color: #c6ece0; }
.ec-footer > a span { margin-left: 18px; }
.ec-footer a:hover { color: #fff; }
@keyframes ec-image-enter { from { opacity: .5; transform: scale(1.025); } to { opacity: 1; transform: scale(1); } }
@media (max-width: 1100px) {
  .ec-stage { height: 620px; }
  .ec-intro h1 { font-size: 60px; }
  .ec-intro > p:not(.ec-eyebrow) { font-size: 12px; }
  .ec-scene-tabs button { gap: 12px; }
  .ec-scene-tabs strong { font-size: 22px; }
  .ec-hardware { gap: 60px; }
}
@media (max-width: 767px) {
  .ec-stage { height: auto; min-height: 0; overflow: visible; background: #0c1011; padding-top: 280px; }
  .ec-intro { top: 27px; left: 20px; right: 20px; }
  .ec-intro .ec-eyebrow { font-size: 7px; }
  .ec-intro .ec-eyebrow > span { margin-inline: 6px; }
  .ec-intro h1 { font-size: clamp(40px, 10.5vw, 60px); margin-top: 16px; }
  .ec-intro > p:not(.ec-eyebrow) { font-size: 11px; line-height: 1.6; margin-top: 15px; }
  .ec-install { position: static; padding: 12px 15px; gap: 18px; font-size: 10px; min-height: 44px; margin-top: 18px; }
  .ec-scene { position: relative; min-height: 0; }
  .ec-scene-image { width: 100%; height: auto; }
  .ec-image-shade { height: auto; aspect-ratio: var(--image-ratio); background: linear-gradient(0deg, #0c1011 0%, #0c101100 23%, #0c101100 85%, #0c101155 100%); }
  .ec-callouts { position: absolute; top: 0; left: 0; right: 0; height: auto; aspect-ratio: var(--image-ratio); }
  .ec-callout { top: var(--my); left: var(--mx); flex-direction: column; align-items: center; gap: 6px; padding: 0; transform: translate(-22px, -22px); }
  .ec-marker { width: 36px; height: 36px; }
  .ec-callout-label { font-size: 9px; line-height: 1.2; padding: 5px 7px; white-space: nowrap; }
  .ec-feature-detail { position: relative; left: auto; bottom: auto; margin: 0 20px; padding: 0 0 0 16px; max-width: none; min-height: 150px; }
  .ec-feature-detail h2 { font-size: 25px; }
  .ec-feature-detail > p:not(.ec-detail-label) { font-size: 11px; max-width: 330px; }
  .ec-feature-detail a { font-size: 10px; margin-top: 15px; }
  .ec-explore-hint { top: 290px; right: 20px; font-size: 7px; }
  .ec-photo-credit { display: none; }
  .ec-scene-tabs { padding-inline: 20px; gap: 15px; margin-top: 25px; }
  .ec-scene-tabs button { padding: 16px 0 20px; min-height: 100px; display: block; }
  .ec-scene-number, .ec-scene-arrow { display: none; }
  .ec-scene-tabs strong { font-size: 18px; }
  .ec-scene-tabs small { font-size: 8px; line-height: 1.6; margin-top: 8px; }
  .ec-more { justify-content: flex-start; gap: 18px 24px; padding: 25px 20px; font-size: 10px; }
  .ec-more > span { width: 100%; font-size: 7px; }
  .ec-hardware { grid-template-columns: 1fr; gap: 35px; padding: 40px 20px; }
  .ec-hardware h2 { font-size: 43px; }
  .ec-hardware .ec-eyebrow { font-size: 8px; }
  .ec-panel-list a { gap: 15px; }
  .ec-panel-list strong { min-width: 88px; }
  .ec-panel-list a > span:not(:last-child) { font-size: 9px; }
  .ec-footer { padding: 28px 20px; gap: 23px; }
  .ec-footer nav { width: 100%; gap: 20px; }
}
@media (max-width: 360px) {
  .ec-install { position: static; margin-top: 16px; }
  .ec-stage { padding-top: 280px; }
  .ec-explore-hint { top: 290px; }
  .ec-callout-label { font-size: 8px; }
  .ec-scene-tabs strong { font-size: 16px; }
}
@media (prefers-reduced-motion: reduce) { .ec-home *, .ec-home *::before, .ec-home *::after { animation: none !important; transition: none !important; } }
</style>
