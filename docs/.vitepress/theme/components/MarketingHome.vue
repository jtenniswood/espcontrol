<script setup lang="ts">
import { ref } from 'vue'
import { withBase } from 'vitepress'

// Frame the original photographs with CSS; keep every on-screen card unchanged.
function product(name: string, image: string, imageHeight: number, bounds: number[], outline: number[][], description: string, guide: string, alt: string) {
  const [left, top, width, height] = bounds
  return {
    name, image, imageHeight, description, guide, alt,
    style: {
      '--ratio': width / height,
      '--image-width': `${1600 / width * 100}%`,
      '--image-left': `${-left / width * 100}%`,
      '--image-top': `${-top / height * 100}%`,
      '--outline': `polygon(${outline.map(([x, y]) => `${x / 1600 * 100}% ${y / imageHeight * 100}%`).join(', ')})`,
    },
  }
}
const products = [
  product('Lights & music', 'home_screen_hero.jpg', 1288, [155, 189, 953, 995], [
    [272, 210], [277, 202], [287, 195], [301, 193], [1072, 251], [1086, 256], [1098, 268], [1104, 284], [1106, 307], [938, 1146], [932, 1166], [922, 1177], [907, 1183], [887, 1180], [177, 977], [166, 972], [158, 963], [155, 949], [156, 934], [267, 228],
  ], 'Lights, scenes, music and climate. One tap away.', '/card-types/', '4-inch EspControl panel with colourful climate, lighting, weather and media cards'),
  product('Weather & cameras', 'jc8012p4a1-hero.jpg', 1177, [64, 90, 1492, 1027], [
    [72, 210], [88, 197], [110, 191], [1278, 92], [1298, 95], [1316, 103], [1334, 123], [1552, 823], [1556, 849], [1553, 865], [1545, 878], [1528, 888], [216, 1116], [196, 1109], [184, 1096], [177, 1080], [65, 252], [65, 228],
  ], 'Daily forecasts, camera snapshots and animated radar.', '/card-types/cameras', '10.1-inch EspControl panel showing rain radar, daily forecasts and room controls'),
  product('Your layout', '4848s040-subpage.jpg', 1238, [313, 137, 965, 971], [
    [347, 161], [353, 149], [365, 140], [382, 139], [1245, 221], [1263, 231], [1274, 245], [1277, 263], [1116, 1082], [1109, 1096], [1095, 1106], [1077, 1107], [335, 955], [323, 949], [315, 938], [313, 922], [346, 180],
  ], 'Resize cards. Create room pages. Pick your colours.', '/features/setup', 'EspControl room page with an enlarged climate card and smaller blinds, temperature and music cards'),
]
const selected = ref(0)
function onTabKeydown(event: KeyboardEvent) {
  let next = selected.value
  if (event.key === 'ArrowRight') next = (next + 1) % products.length
  else if (event.key === 'ArrowLeft') next = (next + products.length - 1) % products.length
  else if (event.key === 'Home') next = 0
  else if (event.key === 'End') next = products.length - 1
  else return
  event.preventDefault()
  selected.value = next
  const tabs = (event.currentTarget as HTMLElement).parentElement?.querySelectorAll<HTMLButtonElement>('[role="tab"]')
  tabs?.[next]?.focus()
}
</script>

<template>
  <main class="ec-home">
    <section id="what-you-can-build" class="ec-product-hero" aria-labelledby="espcontrol-no-code-home-assistant-touchscreen">
      <h1 id="espcontrol-no-code-home-assistant-touchscreen">Home Assistant. <em>Hands on.</em></h1>
      <div class="ec-product-stage">
        <div v-for="(panel, index) in products" v-show="selected === index" :id="`ec-product-${index}`" :key="panel.name" class="ec-product-view" role="tabpanel" :aria-labelledby="`ec-product-tab-${index}`">
          <div class="ec-product-frame" :style="panel.style"><img :src="withBase(`/images/${panel.image}`)" :alt="panel.alt" width="1600" :height="panel.imageHeight" :fetchpriority="index === 0 ? 'high' : undefined" :loading="index === 0 ? 'eager' : 'lazy'"></div>
        </div>
      </div>
      <div id="main-features" class="ec-product-tabs" role="tablist" aria-label="Explore the screen features">
        <button v-for="(panel, index) in products" :id="`ec-product-tab-${index}`" :key="panel.name" type="button" role="tab" :aria-selected="selected === index" :aria-controls="`ec-product-${index}`" :tabindex="selected === index ? 0 : -1" @click="selected = index" @keydown="onTabKeydown">{{ panel.name }}</button>
      </div>
      <p class="ec-feature-summary" aria-live="polite"><a id="card-types" :href="withBase(products[selected].guide)">{{ products[selected].description }} <span aria-hidden="true">↗</span></a></p>
      <div id="next-steps" class="ec-actions"><a id="how-it-works" class="ec-primary" :href="withBase('/getting-started/install')">Install EspControl <span aria-hidden="true">↗</span></a><a id="where-to-buy" class="ec-secondary" :href="withBase('/screens/')">Choose a screen <span aria-hidden="true">→</span></a></div>
    </section>
    <footer id="support-this-project" class="ec-footer"><a :href="withBase('/guides/')">Documentation</a><a href="https://github.com/jtenniswood/espcontrol">GitHub ↗</a><a href="https://www.buymeacoffee.com/jtenniswood">Support the project ↗</a></footer>
  </main>
</template>

<style scoped>
.ec-home { --product-height: clamp(360px, calc(100svh - 310px), 600px); color: #f6f5ef; background: #111719; font-family: var(--vp-font-family-base); overflow: clip; }
:global(body:has(.marketing-home) .sp-support-btn) { display: none; }
.ec-home *, .ec-home *::before, .ec-home *::after { box-sizing: border-box; }
.ec-home h1, .ec-home p { margin: 0; }
.ec-home a { text-decoration: none; }
.ec-home a:focus-visible, .ec-home button:focus-visible { outline: 3px solid #b8f1dc; outline-offset: 5px; }
.ec-product-hero { text-align: center; padding: 32px 24px 36px; }
.ec-home h1 { font-size: clamp(34px, 3.7vw, 54px); font-weight: 500; line-height: 1.15; letter-spacing: -.05em; }
.ec-home h1 em { font-family: Georgia, 'Times New Roman', serif; font-weight: 400; color: #c6ece0; }
.ec-product-stage { position: relative; height: var(--product-height); margin-top: 25px; }
.ec-product-view { position: absolute; inset: 0; display: grid; place-items: center; }
.ec-product-frame { position: relative; width: min(80vw, calc(var(--product-height) * var(--ratio))); aspect-ratio: var(--ratio); filter: drop-shadow(0 25px 30px #00000065); animation: ec-product-enter .35s ease-out; }
.ec-product-frame img { position: absolute; width: var(--image-width); max-width: none; height: auto; left: var(--image-left); top: var(--image-top); clip-path: var(--outline); }
.ec-product-tabs { display: flex; justify-content: center; gap: 32px; margin-top: 32px; }
.ec-product-tabs button { padding: 10px 0; border-bottom: 2px solid transparent; color: #99a9ac; font-size: 19px; line-height: 1.4; font-weight: 500; cursor: pointer; transition: color .2s, border-color .2s; }
.ec-product-tabs button[aria-selected="true"] { color: #f6f5ef; border-bottom-color: #c6ece0; }
.ec-product-tabs button:hover { color: #c6ece0; }
.ec-feature-summary { min-height: 30px; margin-top: 18px !important; color: #b8c7c9; font-size: 17px; line-height: 1.6; }
.ec-feature-summary a span { margin-left: 9px; color: #c6ece0; }
.ec-feature-summary a:hover { color: #f6f5ef; }
.ec-actions { display: flex; justify-content: center; align-items: center; gap: 28px; margin-top: 26px; }
.ec-primary { display: inline-flex; align-items: center; gap: 30px; min-height: 48px; padding: 13px 20px; background: #c6ece0; color: #152a2b; font-size: 15px; font-weight: 550; }
.ec-primary:hover { background: #e2f7ee; }
.ec-secondary { font-size: 15px; }
.ec-secondary span { margin-left: 12px; }
.ec-secondary:hover { color: #c6ece0; }
.ec-footer { display: flex; justify-content: center; flex-wrap: wrap; gap: 30px; padding: 25px 24px 28px; border-top: 1px solid #ffffff15; color: #99a9ac; font-size: 14px; line-height: 1.7; }
.ec-footer a:hover { color: #f6f5ef; }
@keyframes ec-product-enter { from { opacity: .45; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }
@media (max-width: 767px) {
  .ec-home { --product-height: 350px; }
  .ec-product-hero { padding: 28px 18px 30px; }
  .ec-home h1 { font-size: clamp(32px, 8.5vw, 45px); }
  .ec-home h1 em { display: block; margin-top: 3px; }
  .ec-product-stage { margin-top: 22px; }
  .ec-product-frame { width: min(84vw, calc(var(--product-height) * var(--ratio))); }
  .ec-product-tabs { gap: 20px; margin-top: 20px; }
  .ec-product-tabs button { font-size: 15px; padding-block: 11px; }
  .ec-feature-summary { font-size: 16px; min-height: 52px; max-width: 340px; margin: 17px auto 0 !important; }
  .ec-actions { gap: 22px; margin-top: 22px; }
  .ec-primary { gap: 17px; padding-inline: 15px; font-size: 13px; }
  .ec-secondary { font-size: 13px; }
  .ec-secondary span { margin-left: 7px; }
  .ec-footer { gap: 18px; padding-inline: 18px; font-size: 13px; }
}
@media (max-width: 360px) {
  .ec-home { --product-height: 305px; }
  .ec-product-tabs { gap: 14px; }
  .ec-product-tabs button { font-size: 13px; }
  .ec-actions { gap: 18px; }
  .ec-primary { font-size: 12px; gap: 13px; }
  .ec-secondary { font-size: 12px; }
}
@media (prefers-reduced-motion: reduce) { .ec-product-frame { animation: none; } .ec-home button { transition: none; } }
</style>
