<script setup lang="ts">
import { ref } from 'vue'
import { withBase } from 'vitepress'

const lightsOn = ref(true)
const playing = ref(false)
const blindsOpen = ref(true)
const temperature = ref(21)
const demoStatus = ref('Try the controls. This demo only changes this page.')
const paths = {
  bulb: 'M9 18h6m-5 3h4M8.1 13.6a6 6 0 1 1 7.8 0c-.9.8-1.4 1.5-1.4 2.4h-5c0-.9-.5-1.6-1.4-2.4Z',
  music: 'M9 18V5l11-2v13M9 8l11-2M9 18c0 1.7-1.6 3-3.5 3S2 19.7 2 18s1.6-3 3.5-3S9 16.3 9 18Zm11-2c0 1.7-1.6 3-3.5 3S13 17.7 13 16s1.6-3 3.5-3S20 14.3 20 16Z',
  blinds: 'M3 4h18v12H3zM3 8h18M3 12h18M8 16v5m8-5v5',
  arrow: 'M4 12h16M13 5l7 7-7 7',
  plus: 'M12 5v14M5 12h14',
  minus: 'M5 12h14',
}
function toggleLights() {
  lightsOn.value = !lightsOn.value
  demoStatus.value = `Living room lights ${lightsOn.value ? 'on' : 'off'}.`
}
function toggleMusic() {
  playing.value = !playing.value
  demoStatus.value = `Kitchen speaker ${playing.value ? 'playing' : 'paused'}.`
}
function toggleBlinds() {
  blindsOpen.value = !blindsOpen.value
  demoStatus.value = `Bedroom blinds ${blindsOpen.value ? 'open' : 'closed'}.`
}
function adjustTemperature(change: number) {
  temperature.value = Math.max(16, Math.min(28, temperature.value + change))
  demoStatus.value = `Demo temperature set to ${temperature.value} degrees Celsius.`
}
const panels = [
  { name: '4″ S3', model: '4848S040', spec: '480 × 480 · 9 home slots', image: '4848s040-hero.jpg', height: 1167, href: '/screens/4848s040' },
  { name: '4″ P4', model: 'ESP32-P4 86', spec: '720 × 720 · 9 home slots', image: 'p4-86-hero.jpg', height: 1316, href: '/screens/p4-86' },
  { name: '4.3″ P4', model: 'JC4880P443', spec: '480 × 800 · 6 home slots', image: 'jc4880p443-hero.jpg', height: 1228, href: '/screens/jc4880p443' },
  { name: '7″ P4', model: 'JC1060P470', spec: '1024 × 600 · 15 home slots', image: 'jc1060p470-hero.jpg', height: 1137, href: '/screens/jc1060p470' },
  { name: '10.1″ P4', model: 'JC8012P4A1', spec: '1280 × 800 · 20 home slots', image: 'jc8012p4a1-hero.jpg', height: 1177, href: '/screens/jc8012p4a1' },
]
</script>

<template>
  <main class="ec-home">
    <section class="ec-hero" aria-labelledby="espcontrol-no-code-home-assistant-touchscreen">
      <div class="ec-hero-top ec-wrap"><p class="ec-label">ESPCONTROL / HOME ASSISTANT, HANDS ON</p><a :href="withBase('/guides/')">Straight to the docs <span aria-hidden="true">↗</span></a></div>
      <div class="ec-hero-stage ec-wrap">
        <div class="ec-hero-copy">
          <h1 id="espcontrol-no-code-home-assistant-touchscreen">Home,<br><span>on tap.</span></h1>
          <p class="ec-hero-description">A proper control panel for Home Assistant.<br>For the lights you left on. The music you want louder. The heating that needs a nudge.</p>
          <a class="ec-button" :href="withBase('/getting-started/install')">Build your panel <svg viewBox="0 0 24 24" aria-hidden="true"><path :d="paths.arrow" /></svg></a>
          <p class="ec-hero-fact">ESP32 touchscreens. Browser setup. No YAML.</p>
        </div>
        <figure class="ec-hero-photo">
          <img :src="withBase('/images/home_screen_hero.jpg')" alt="EspControl touchscreen panels on a desk, showing temperature, lighting, music and weather controls" width="1600" height="1288" fetchpriority="high">
          <figcaption><span>REAL HARDWARE. YOUR HOME ASSISTANT.</span><span aria-hidden="true">↙</span></figcaption>
        </figure>
        <a class="ec-size-note" :href="withBase('/screens/')"><span>4″—10.1″</span>Find your size <span aria-hidden="true">↗</span></a>
      </div>
      <div class="ec-feature-ribbon" aria-label="Control lights, music, climate, blinds, and more"><span>Lights</span><i aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M12 2v20M3.3 7l17.4 10M3.3 17 20.7 7" /></svg></i><span>Music</span><i aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M12 2v20M3.3 7l17.4 10M3.3 17 20.7 7" /></svg></i><span>Climate</span><i aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M12 2v20M3.3 7l17.4 10M3.3 17 20.7 7" /></svg></i><span>Blinds</span><i aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M12 2v20M3.3 7l17.4 10M3.3 17 20.7 7" /></svg></i><a :href="withBase('/card-types/')">And then some <span aria-hidden="true">↗</span></a></div>
    </section>

    <section id="what-you-can-build" class="ec-touch ec-wrap" aria-labelledby="ec-touch-heading">
      <div class="ec-touch-copy"><p class="ec-label">01 / THE EVERYDAY CONTROLS</p><h2 id="ec-touch-heading">Lights down.<br>Music up.<br><em>Phone away.</em></h2><p>Put the things you use every day where everyone can reach them. Big buttons. Live values. A screen that’s ready when you are.</p><a id="card-types" class="ec-link" :href="withBase('/card-types/')">Explore the card types <span aria-hidden="true">↗</span></a></div>
      <div class="ec-demo">
        <div class="ec-demo-heading"><span>Go on. Give it a tap.</span><span class="ec-label">INTERACTIVE DEMO</span></div>
        <div class="ec-demo-grid">
          <button class="ec-tile ec-light-tile" :class="{ 'ec-light-off': !lightsOn }" type="button" :aria-pressed="lightsOn" aria-label="Living room lights" @click="toggleLights"><svg viewBox="0 0 24 24" aria-hidden="true"><path :d="paths.bulb" /></svg><span class="ec-tile-value">{{ lightsOn ? 'On' : 'Off' }}</span><span class="ec-tile-caption">Living room</span></button>
          <button class="ec-tile ec-music-tile" type="button" :aria-pressed="playing" aria-label="Kitchen speaker playback" @click="toggleMusic"><svg viewBox="0 0 24 24" aria-hidden="true"><path :d="paths.music" /></svg><span class="ec-equalizer" :class="{ 'is-playing': playing }" aria-hidden="true"><i></i><i></i><i></i><i></i><i></i></span><span class="ec-tile-caption">{{ playing ? 'Playing' : 'Kitchen speaker' }} <span class="ec-play-icon" aria-hidden="true">{{ playing ? 'Ⅱ' : '▶' }}</span></span></button>
          <div class="ec-tile ec-temperature-tile"><div class="ec-temperature-value">{{ temperature }}<span>°C</span></div><span class="ec-tile-caption">A little warmer?</span><div class="ec-temperature-actions"><button type="button" aria-label="Lower demo temperature" :disabled="temperature === 16" @click="adjustTemperature(-1)"><svg viewBox="0 0 24 24" aria-hidden="true"><path :d="paths.minus" /></svg></button><button type="button" aria-label="Raise demo temperature" :disabled="temperature === 28" @click="adjustTemperature(1)"><svg viewBox="0 0 24 24" aria-hidden="true"><path :d="paths.plus" /></svg></button></div></div>
          <button class="ec-tile ec-blind-tile" type="button" :aria-pressed="!blindsOpen" aria-label="Close bedroom blinds" @click="toggleBlinds"><svg class="ec-blind-icon" :class="{ 'is-closed': !blindsOpen }" viewBox="0 0 24 24" aria-hidden="true"><path :d="paths.blinds" /></svg><span class="ec-tile-value">{{ blindsOpen ? 'Hello, sun.' : 'Goodnight.' }}</span><span class="ec-tile-caption">Bedroom blinds</span></button>
        </div>
        <p class="ec-demo-status" role="status">{{ demoStatus }}</p>
      </div>
    </section>

    <section class="ec-weather" aria-labelledby="ec-weather-heading">
      <div class="ec-wrap ec-weather-inner"><div class="ec-weather-copy"><p class="ec-label">02 / A WINDOW ON YOUR HOME</p><h2 id="ec-weather-heading">Rain on<br>the way?</h2><p class="ec-weather-answer">See it coming.</p><p>Weather forecasts, camera snapshots, animated radar and live sensors. Check what’s happening before you head out.</p><a class="ec-link" :href="withBase('/card-types/weather')">Weather on your screen <span aria-hidden="true">↗</span></a><a class="ec-weather-secondary" :href="withBase('/card-types/cameras')">Cameras & animated images <span aria-hidden="true">↗</span></a></div><figure class="ec-weather-photo"><img :src="withBase('/images/jc8012p4a1-hero.jpg')" alt="10.1-inch EspControl panel showing a large rain radar map beside weather forecasts and room controls" width="1600" height="1177" loading="lazy"><figcaption><span class="ec-label">THE 10.1″ P4</span><span>A bigger view of what’s going on.</span></figcaption></figure></div>
    </section>

    <section class="ec-layout ec-wrap" aria-labelledby="ec-layout-heading">
      <figure class="ec-layout-photo"><img :src="withBase('/images/4848s040-subpage.jpg')" alt="A Devices subpage on a 4-inch panel, with an enlarged Aircon card and controls for blinds and music" width="1600" height="1238" loading="lazy"><figcaption>One small screen. A whole room of controls.</figcaption><span class="ec-photo-corner" aria-hidden="true">↗</span></figure>
      <div class="ec-layout-copy"><p class="ec-label">03 / ARRANGE IT YOUR WAY</p><h2 id="ec-layout-heading">Make the big button the one you <em>actually use.</em></h2><p>Open the panel’s web page and make yourself at home. The layout is yours to move around.</p><div class="ec-layout-links"><a :href="withBase('/features/setup')"><span>01</span><div><strong>Drag it. Resize it.</strong><p>Give the important things more space.</p></div><span aria-hidden="true">↗</span></a><a :href="withBase('/features/subpages')"><span>02</span><div><strong>Give each room a page.</strong><p>Keep the kitchen out of the bedroom.</p></div><span aria-hidden="true">↗</span></a><a :href="withBase('/features/appearance')"><span>03</span><div><strong>Pick your colours.</strong><p>Bright and bold, or a little more understated.</p></div><span aria-hidden="true">↗</span></a></div></div>
    </section>

    <section id="main-features" class="ec-afterhours" aria-labelledby="ec-afterhours-heading"><div class="ec-wrap ec-afterhours-inner"><div><p class="ec-label">THERE’S A QUIETER SIDE, TOO.</p><h2 id="ec-afterhours-heading">And when<br>the day’s done.</h2></div><div class="ec-afterhours-links"><a :href="withBase('/features/screen-schedule')"><span>01</span><div><h3>Let it call it a night.</h3><p>Schedule a dimmer screen, a clock or lights-out.</p></div><span aria-hidden="true">↗</span></a><a :href="withBase('/immich/')"><span>02</span><div><h3>Put your photos up.</h3><p>Show your Immich library through the companion integration.</p></div><span aria-hidden="true">↗</span></a><a :href="withBase('/features/screensaver#pin-protection-after-wake')"><span>03</span><div><h3>Keep little fingers out.</h3><p>Protect touchscreen controls with a wake-up PIN.</p></div><span aria-hidden="true">↗</span></a></div></div></section>

    <section id="where-to-buy" class="ec-panels ec-wrap" aria-labelledby="ec-panels-heading"><div class="ec-panels-heading"><div><p class="ec-label">FIVE PANELS. SAME ESPCONTROL.</p><h2 id="ec-panels-heading">Find your size.</h2></div><a class="ec-link" :href="withBase('/screens/')">Compare the hardware <span aria-hidden="true">↗</span></a></div><div class="ec-panel-grid"><a v-for="(panel, index) in panels" :key="panel.model" :href="withBase(panel.href)" class="ec-device-link"><div class="ec-device-photo"><img :src="withBase(`/images/${panel.image}`)" :alt="`${panel.model} running EspControl`" width="1600" :height="panel.height" loading="lazy"><span class="ec-device-number" aria-hidden="true">0{{ index + 1 }}</span></div><div class="ec-device-title"><h3>{{ panel.name }}</h3><span aria-hidden="true">↗</span></div><p class="ec-device-model">{{ panel.model }}</p><p class="ec-device-spec">{{ panel.spec }}</p></a></div><p class="ec-panel-note">Pick the exact model and hardware revision before installing. Each guide has the details.</p></section>

    <section id="how-it-works" class="ec-start" aria-labelledby="ec-start-heading"><div class="ec-wrap ec-start-inner"><div><p class="ec-label">YOUR NEXT WEEKEND PROJECT</p><h2 id="ec-start-heading">Plug in.<br>Set up.<br><em>Tap away.</em></h2><a id="next-steps" class="ec-button" :href="withBase('/getting-started/install')">Let’s build yours <svg viewBox="0 0 24 24" aria-hidden="true"><path :d="paths.arrow" /></svg></a></div><div class="ec-start-right"><img :src="withBase('/images/screens-hero.jpg')" alt="Square EspControl panels showing temperature, music and weather radar layouts" width="1600" height="1131" loading="lazy"><ol><li><strong>Choose a supported panel.</strong><span>Bring a USB data cable and Home Assistant.</span></li><li><strong>Install from your browser.</strong><span>Use Chrome or Edge on a computer. Join WiFi, pair with Home Assistant and enable actions.</span></li><li><strong>Arrange your controls.</strong><span>Open the panel’s web page. No YAML to write.</span></li></ol></div></div></section>
    <footer id="support-this-project" class="ec-footer ec-wrap"><a class="ec-wordmark" :href="withBase('/')">EspControl<span aria-hidden="true">↗</span></a><p>A touchscreen project for your Home Assistant home.</p><nav aria-label="Homepage footer"><a :href="withBase('/guides/')">Documentation</a><a href="https://github.com/jtenniswood/espcontrol">GitHub ↗</a><a href="https://www.buymeacoffee.com/jtenniswood">Buy James a coffee ↗</a></nav></footer>
  </main>
</template>

<style scoped>
.ec-home { --ec-paper: #f0eee6; --ec-ink: #20221e; --ec-lime: #deff7a; --ec-blue: #174dff; --ec-muted: #616359; background: var(--ec-paper); color: var(--ec-ink); overflow: clip; font-family: var(--vp-font-family-base); }
:global(body:has(.marketing-home) .sp-support-btn) { display: none; }
.ec-home *, .ec-home *::before, .ec-home *::after { box-sizing: border-box; }
.ec-home h1, .ec-home h2, .ec-home h3, .ec-home p, .ec-home figure { margin: 0; }
.ec-home a { text-decoration: none; }
.ec-home a:focus-visible, .ec-home button:focus-visible { outline: 3px solid #ff694f; outline-offset: 5px; }
.ec-home svg { fill: none; stroke: currentColor; stroke-width: 1.6; stroke-linecap: round; stroke-linejoin: round; width: 24px; height: 24px; }
.ec-wrap { width: min(1440px, calc(100% - 112px)); margin-inline: auto; }
.ec-label { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 10px; line-height: 1.6; letter-spacing: .09em; }
.ec-home h2 { font-size: clamp(44px, 5.7vw, 82px); line-height: 1; letter-spacing: -.065em; font-weight: 650; }
.ec-home em { font-family: Georgia, 'Times New Roman', serif; font-weight: 400; letter-spacing: -.06em; }
.ec-button { display: inline-flex; align-items: center; justify-content: space-between; gap: 42px; min-height: 58px; padding: 18px 24px; color: #20221e; background: var(--ec-lime); font-size: 15px; font-weight: 650; transition: background-color .2s, transform .2s; }
.ec-button:hover { background: #c8f455; transform: translateY(-3px); }
.ec-link { display: inline-flex; align-items: center; gap: 30px; padding-block: 10px; border-bottom: 1px solid currentColor; font-size: 14px; font-weight: 550; }
.ec-link span { transition: transform .2s; }
.ec-link:hover span { transform: translate(3px, -3px); }
.ec-hero { background: #1c211c; color: var(--ec-paper); }
.ec-hero-top { display: flex; justify-content: space-between; align-items: center; padding-block: 27px; border-bottom: 1px solid #f0eee633; }
.ec-hero-top .ec-label { color: #d1d6c8; }
.ec-hero-top a { font-size: 11px; }
.ec-hero-top a span { margin-left: 17px; color: var(--ec-lime); }
.ec-hero-stage { position: relative; display: grid; grid-template-columns: .92fr 1.08fr; align-items: center; min-height: 660px; padding-block: 48px 80px; }
.ec-hero-copy { position: relative; z-index: 2; }
.ec-hero h1 { font-size: clamp(94px, 10.8vw, 162px); font-weight: 750; line-height: .88; letter-spacing: -.08em; }
.ec-hero h1 span { color: var(--ec-lime); }
.ec-hero-description { max-width: 345px; margin-top: 32px !important; font-size: 15px; line-height: 1.7; color: #d0d3c9; }
.ec-hero .ec-button { margin-top: 28px; }
.ec-hero-fact { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; margin-top: 18px !important; color: #a4ad9e; font-size: 10px; }
.ec-hero-photo { position: relative; margin-right: -56px !important; margin-left: -10px !important; transform: rotate(-3deg); }
.ec-hero-photo img { display: block; width: 100%; height: auto; }
.ec-hero-photo figcaption { display: flex; align-items: center; justify-content: space-between; padding: 13px 5px; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 9px; letter-spacing: .06em; color: #d1d6c8; }
.ec-hero-photo figcaption span:last-child { font-size: 32px; line-height: 1; color: var(--ec-lime); }
.ec-size-note { position: absolute; right: 16px; bottom: 29px; display: flex; gap: 12px; align-items: center; font-size: 11px; }
.ec-size-note > span:first-child { font-size: 24px; font-weight: 550; letter-spacing: -.05em; margin-right: 10px; }
.ec-feature-ribbon { display: flex; justify-content: center; align-items: center; gap: 35px; padding: 20px 30px; background: var(--ec-lime); color: var(--ec-ink); }
.ec-feature-ribbon > span { font-size: clamp(18px, 2.2vw, 30px); font-weight: 550; letter-spacing: -.04em; }
.ec-feature-ribbon i { font-size: 25px; font-style: normal; }
.ec-feature-ribbon a { font-size: 13px; white-space: nowrap; }
.ec-feature-ribbon a span { margin-left: 10px; }
.ec-touch { display: grid; grid-template-columns: 1fr 1.05fr; gap: 90px; align-items: center; padding-block: 130px; scroll-margin-top: 80px; }
.ec-touch-copy h2 { margin-top: 27px; }
.ec-touch-copy > p:not(.ec-label) { max-width: 340px; font-size: 15px; line-height: 1.8; margin-top: 30px; color: var(--ec-muted); }
.ec-touch-copy .ec-link { margin-top: 25px; }
.ec-demo { transform: rotate(2deg); }
.ec-demo-heading { display: flex; justify-content: space-between; align-items: center; gap: 15px; padding-bottom: 18px; }
.ec-demo-heading > span:first-child { font-family: Georgia, 'Times New Roman', serif; font-style: italic; font-size: 23px; }
.ec-demo-heading .ec-label { font-size: 8px; color: var(--ec-muted); }
.ec-demo-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; padding: 14px; background: #242824; border-radius: 22px; box-shadow: 12px 15px 0 #d9d8ce; }
.ec-tile { min-width: 0; min-height: 190px; display: flex; flex-direction: column; align-items: flex-start; position: relative; border-radius: 35px; padding: 23px; text-align: left; border: 0; transition: background-color .2s, transform .2s; }
button.ec-tile { cursor: pointer; }
button.ec-tile:active { transform: scale(.97); }
.ec-tile > svg { width: 34px; height: 34px; }
.ec-light-tile { background: #c9eb9e; color: #243a1d; }
.ec-light-off { background: #4d574c; color: #e0e3d9; }
.ec-tile-value { font-size: 32px; line-height: 1.1; letter-spacing: -.05em; margin-top: 24px; }
.ec-tile-caption { display: flex; align-items: center; justify-content: space-between; width: 100%; font-size: 13px; line-height: 1.3; margin-top: auto; padding-top: 12px; }
.ec-music-tile { background: #174dff; color: #fff; }
.ec-play-icon { font-size: 15px; }
.ec-equalizer { display: flex; align-items: center; gap: 5px; height: 50px; margin-top: 17px; }
.ec-equalizer i { display: block; width: 7px; height: 7px; background: currentColor; transition: height .3s; }
.ec-equalizer.is-playing i:nth-child(1) { height: 20px; }
.ec-equalizer.is-playing i:nth-child(2) { height: 37px; }
.ec-equalizer.is-playing i:nth-child(3) { height: 27px; }
.ec-equalizer.is-playing i:nth-child(4) { height: 46px; }
.ec-equalizer.is-playing i:nth-child(5) { height: 17px; }
.ec-temperature-tile { background: #fa816f; color: #3c2320; }
.ec-temperature-value { font-size: 68px; letter-spacing: -.08em; line-height: 1; }
.ec-temperature-value > span { font-size: 20px; letter-spacing: -.04em; margin-left: 5px; }
.ec-temperature-tile .ec-tile-caption { padding-top: 8px; }
.ec-temperature-actions { display: flex; justify-content: space-between; width: 100%; margin-top: 14px; border-top: 1px solid #3c232050; padding-top: 10px; }
.ec-temperature-actions button { width: 44px; min-height: 44px; display: grid; place-items: center; border: 1px solid #3c232050; border-radius: 50%; cursor: pointer; }
.ec-temperature-actions button:hover { background: #ffffff40; }
.ec-temperature-actions button:disabled { opacity: .35; cursor: default; }
.ec-blind-tile { background: #cfb8ef; color: #3a2457; }
.ec-blind-tile .ec-tile-value { font-size: 26px; }
.ec-blind-icon { transform: scaleY(.75); transform-origin: center top; transition: transform .3s; }
.ec-blind-icon.is-closed { transform: scaleY(1.15); }
.ec-demo-status { padding: 25px 5px 0; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 9px; line-height: 1.6; color: var(--ec-muted); }
.ec-weather { background: #123d59; color: #e2f2f3; }
.ec-weather-inner { display: grid; grid-template-columns: .8fr 1.2fr; gap: 38px; align-items: center; padding-block: 100px; }
.ec-weather-copy { position: relative; z-index: 1; }
.ec-weather .ec-label { color: #a8d6df; }
.ec-weather h2 { font-size: clamp(66px, 7.9vw, 114px); letter-spacing: -.07em; margin-top: 30px; }
.ec-weather-answer { font-family: Georgia, 'Times New Roman', serif; font-style: italic; font-size: clamp(30px, 3.3vw, 47px); line-height: 1.2; margin-top: 14px !important; color: #a8d6df; }
.ec-weather-copy > p:not(.ec-label):not(.ec-weather-answer) { font-size: 14px; line-height: 1.8; max-width: 290px; margin-top: 30px; color: #c3dbe1; }
.ec-weather .ec-link { margin-top: 24px; }
.ec-weather-secondary { display: block; width: fit-content; font-size: 11px; color: #c3dbe1; margin-top: 17px; }
.ec-weather-secondary span { margin-left: 12px; }
.ec-weather-photo { margin-right: -56px !important; transform: rotate(3deg); }
.ec-weather-photo img { display: block; width: 100%; height: auto; }
.ec-weather-photo figcaption { display: flex; justify-content: space-between; align-items: center; gap: 15px; font-size: 10px; line-height: 1.6; padding-top: 15px; }
.ec-layout { display: grid; grid-template-columns: 1fr 1fr; gap: 85px; align-items: center; padding-block: 140px; }
.ec-layout-photo { position: relative; transform: rotate(-3deg); }
.ec-layout-photo img { display: block; width: 100%; height: auto; }
.ec-layout-photo figcaption { font-family: Georgia, 'Times New Roman', serif; font-style: italic; font-size: 18px; padding-top: 18px; }
.ec-photo-corner { position: absolute; top: -30px; right: -28px; display: grid; place-items: center; width: 80px; height: 80px; font-size: 46px; color: var(--ec-paper); background: var(--ec-blue); }
.ec-layout h2 { font-size: clamp(40px, 4.1vw, 59px); margin-top: 22px; }
.ec-layout-copy > p:not(.ec-label) { max-width: 350px; color: var(--ec-muted); font-size: 14px; line-height: 1.8; margin-top: 25px; }
.ec-layout-links { margin-top: 25px; }
.ec-layout-links a { display: flex; gap: 15px; border-top: 1px solid #20221e40; padding-block: 17px; }
.ec-layout-links a > span { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 10px; padding-top: 3px; }
.ec-layout-links a > span:last-child { font-size: 18px; margin-left: auto; }
.ec-layout-links strong { font-size: 17px; font-weight: 550; letter-spacing: -.02em; }
.ec-layout-links p { font-size: 11px; line-height: 1.6; color: var(--ec-muted); margin-top: 5px; }
.ec-layout-links a:hover strong { color: var(--ec-blue); }
.ec-afterhours { background: #302c39; color: #eee8f4; }
.ec-afterhours-inner { display: grid; grid-template-columns: 1fr 1fr; gap: 85px; padding-block: 85px; }
.ec-afterhours h2 { font-size: clamp(44px, 4.5vw, 65px); margin-top: 25px; }
.ec-afterhours .ec-label { color: #c5b6d5; }
.ec-afterhours-links a { display: flex; align-items: flex-start; gap: 20px; border-top: 1px solid #eee8f440; padding-block: 23px; }
.ec-afterhours-links a > span:first-child { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 10px; padding-top: 3px; color: #c5b6d5; }
.ec-afterhours-links a > span:last-child { margin-left: auto; font-size: 22px; color: #cfb8ef; }
.ec-afterhours h3 { font-size: 23px; font-weight: 500; letter-spacing: -.03em; line-height: 1.2; }
.ec-afterhours p:not(.ec-label) { font-size: 12px; color: #c5b6d5; line-height: 1.7; margin-top: 8px; }
.ec-afterhours a:hover h3 { color: #deff7a; }
.ec-panels { padding-block: 110px; scroll-margin-top: 80px; }
.ec-panels-heading { display: flex; align-items: flex-end; justify-content: space-between; gap: 30px; }
.ec-panels h2 { margin-top: 20px; }
.ec-panel-grid { display: grid; grid-template-columns: repeat(5, minmax(0, 1fr)); gap: 18px; margin-top: 50px; }
.ec-device-link { min-width: 0; }
.ec-device-photo { position: relative; background: #20221e; aspect-ratio: 1.13; }
.ec-device-photo img { display: block; width: 100%; height: 100%; object-fit: contain; transition: opacity .2s; }
.ec-device-number { position: absolute; bottom: -18px; left: 4px; color: var(--ec-paper); font-size: 65px; font-weight: 650; letter-spacing: -.08em; line-height: 1; -webkit-text-stroke: 1px var(--ec-ink); }
.ec-device-title { display: flex; align-items: center; justify-content: space-between; gap: 10px; margin-top: 38px; }
.ec-device-title h3 { font-size: clamp(20px, 2vw, 29px); line-height: 1.1; font-weight: 600; letter-spacing: -.04em; }
.ec-device-title > span { font-size: 21px; }
.ec-device-model { margin-top: 10px !important; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 9px; }
.ec-device-spec { margin-top: 12px !important; font-size: 10px; color: var(--ec-muted); line-height: 1.6; }
.ec-device-link:hover .ec-device-photo img { opacity: .8; }
.ec-device-link:hover .ec-device-title { color: var(--ec-blue); }
.ec-panel-note { margin-top: 35px !important; font-size: 11px; color: var(--ec-muted); line-height: 1.7; }
.ec-start { background: var(--ec-blue); color: #f0eee6; }
.ec-start-inner { display: grid; grid-template-columns: 1fr 1.1fr; gap: 80px; padding-block: 90px; }
.ec-start h2 { font-size: clamp(66px, 7.3vw, 105px); margin-top: 30px; }
.ec-start h2 em { color: var(--ec-lime); }
.ec-start .ec-button { margin-top: 36px; }
.ec-start-right img { width: 100%; height: auto; display: block; }
.ec-start ol { list-style: none; counter-reset: setup; margin: 25px 0 0; padding: 0; }
.ec-start li { counter-increment: setup; display: grid; grid-template-columns: 24px 1fr; column-gap: 14px; padding-block: 15px; border-top: 1px solid #ffffff50; }
.ec-start li::before { content: '0' counter(setup); grid-row: span 2; padding-top: 3px; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 10px; }
.ec-start li strong { font-size: 14px; font-weight: 550; }
.ec-start li span { font-size: 11px; line-height: 1.7; margin-top: 4px; color: #d7e0ff; }
.ec-footer { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; padding-block: 50px; }
.ec-wordmark { display: flex; align-items: center; gap: 18px; width: fit-content; font-size: 38px; font-weight: 700; letter-spacing: -.07em; line-height: 1; }
.ec-wordmark > span { color: var(--ec-blue); }
.ec-footer > p { align-self: center; font-size: 12px; color: var(--ec-muted); }
.ec-footer nav { grid-column: 2; display: flex; flex-wrap: wrap; gap: 25px; font-size: 11px; }
.ec-footer nav a:hover { text-decoration: underline; }
@media (min-width: 1700px) { .ec-hero-photo, .ec-weather-photo { margin-right: 0 !important; } }
@media (max-width: 1100px) {
  .ec-wrap { width: calc(100% - 72px); }
  .ec-hero-stage { min-height: 600px; }
  .ec-hero h1 { font-size: 116px; }
  .ec-hero-photo { margin-right: -36px !important; }
  .ec-hero-description { max-width: 285px; font-size: 14px; }
  .ec-hero-fact { max-width: 260px; line-height: 1.8; }
  .ec-feature-ribbon { gap: 24px; }
  .ec-touch { gap: 50px; padding-block: 100px; }
  .ec-tile { min-height: 175px; padding: 19px; border-radius: 28px; }
  .ec-tile-value { font-size: 28px; }
  .ec-blind-tile .ec-tile-value { font-size: 23px; }
  .ec-tile-caption { font-size: 11px; }
  .ec-weather-inner { gap: 30px; }
  .ec-weather-photo { margin-right: -36px !important; }
  .ec-layout { gap: 55px; }
  .ec-afterhours-inner, .ec-start-inner { gap: 50px; }
  .ec-panel-grid { gap: 12px; }
  .ec-device-spec { font-size: 9px; }
}
@media (max-width: 767px) {
  .ec-wrap { width: calc(100% - 40px); }
  .ec-label { font-size: 8px; }
  .ec-hero-top { padding-block: 21px; }
  .ec-hero-top a { font-size: 9px; }
  .ec-hero-top a span { margin-left: 5px; }
  .ec-hero-stage { grid-template-columns: 1fr; min-height: 0; padding-block: 38px 65px; }
  .ec-hero h1 { font-size: clamp(100px, 24vw, 150px); }
  .ec-hero-description { max-width: 350px; margin-top: 23px !important; font-size: 14px; }
  .ec-hero .ec-button { margin-top: 24px; }
  .ec-hero-fact { max-width: none; margin-top: 14px !important; font-size: 9px; }
  .ec-hero-photo { margin: 38px -20px 0 0 !important; transform: rotate(-3deg); }
  .ec-hero-photo figcaption { font-size: 8px; padding-right: 14px; }
  .ec-size-note { right: 5px; bottom: 20px; font-size: 10px; }
  .ec-size-note > span:first-child { font-size: 21px; }
  .ec-feature-ribbon { gap: 14px; padding: 17px 20px; flex-wrap: wrap; justify-content: flex-start; }
  .ec-feature-ribbon > span { font-size: 20px; }
  .ec-feature-ribbon i { font-size: 17px; }
  .ec-feature-ribbon a { width: 100%; font-size: 11px; margin-top: 4px; }
  .ec-touch { grid-template-columns: 1fr; gap: 46px; padding-block: 70px; }
  .ec-home h2 { font-size: clamp(46px, 12.5vw, 70px); }
  .ec-touch-copy h2 { margin-top: 20px; }
  .ec-touch-copy > p:not(.ec-label) { margin-top: 23px; font-size: 14px; }
  .ec-touch-copy .ec-link { margin-top: 19px; }
  .ec-demo { transform: rotate(1deg); }
  .ec-demo-heading > span:first-child { font-size: 21px; }
  .ec-demo-heading .ec-label { font-size: 7px; }
  .ec-demo-grid { padding: 10px; gap: 8px; border-radius: 18px; box-shadow: 7px 9px 0 #d9d8ce; }
  .ec-tile { min-height: 176px; padding: 18px; border-radius: 28px; }
  .ec-tile-value { font-size: 30px; }
  .ec-temperature-value { font-size: 59px; }
  .ec-blind-tile .ec-tile-value { font-size: 22px; }
  .ec-demo-status { min-height: 50px; padding-top: 20px; font-size: 8px; }
  .ec-weather-inner { grid-template-columns: 1fr; gap: 40px; padding-block: 65px; }
  .ec-weather h2 { font-size: clamp(76px, 20vw, 105px); margin-top: 22px; }
  .ec-weather-answer { font-size: 36px; }
  .ec-weather-copy > p:not(.ec-label):not(.ec-weather-answer) { max-width: 350px; margin-top: 23px; }
  .ec-weather .ec-link { margin-top: 19px; }
  .ec-weather-photo { margin: 0 -8px !important; transform: rotate(2deg); }
  .ec-weather-photo figcaption { font-size: 8px; }
  .ec-layout { grid-template-columns: 1fr; gap: 50px; padding-block: 80px; }
  .ec-layout-photo { margin-inline: 10px !important; }
  .ec-photo-corner { width: 60px; height: 60px; top: -20px; right: -16px; font-size: 36px; }
  .ec-layout-photo figcaption { font-size: 16px; }
  .ec-layout h2 { font-size: 43px; }
  .ec-layout-links strong { font-size: 17px; }
  .ec-afterhours-inner { grid-template-columns: 1fr; gap: 35px; padding-block: 65px; }
  .ec-afterhours h2 { font-size: 53px; }
  .ec-afterhours h3 { font-size: 22px; }
  .ec-panels { padding-block: 70px; }
  .ec-panels-heading { align-items: flex-start; flex-direction: column; gap: 18px; }
  .ec-panels h2 { margin-top: 18px; }
  .ec-panel-grid { grid-template-columns: 1fr 1fr; column-gap: 15px; row-gap: 32px; margin-top: 30px; }
  .ec-device-title { margin-top: 30px; }
  .ec-device-title h3 { font-size: 24px; }
  .ec-device-number { font-size: 58px; }
  .ec-device-model { font-size: 8px; }
  .ec-device-spec { font-size: 9px; }
  .ec-panel-note { font-size: 10px; }
  .ec-start-inner { grid-template-columns: 1fr; gap: 45px; padding-block: 65px; }
  .ec-start h2 { font-size: 82px; margin-top: 22px; }
  .ec-start .ec-button { margin-top: 28px; }
  .ec-start-right { min-width: 0; }
  .ec-start li { padding-block: 18px; }
  .ec-footer { grid-template-columns: 1fr; gap: 22px; padding-block: 35px; }
  .ec-footer nav { grid-column: 1; gap: 20px; }
}
@media (max-width: 360px) {
  .ec-hero-top { gap: 14px; align-items: flex-start; }
  .ec-hero-top .ec-label { max-width: 160px; }
  .ec-demo-heading { align-items: flex-start; }
  .ec-demo-heading > span:first-child { font-size: 19px; }
  .ec-tile { padding: 14px; }
  .ec-blind-tile .ec-tile-value { font-size: 20px; }
}
@media (prefers-reduced-motion: reduce) { .ec-home *, .ec-home *::before, .ec-home *::after { transition: none !important; } .ec-home a:hover { transform: none; } }
</style>
