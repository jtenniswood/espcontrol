<template>
  <div class="icon-gallery">
    <input
      v-model="search"
      type="text"
      class="icon-search"
      placeholder="Search icons…"
    />
    <p class="icon-count">{{ totalFiltered }} icon{{ totalFiltered === 1 ? '' : 's' }}</p>
    <div v-for="group in filteredGroups" :key="group.name" class="icon-group">
      <h3 class="icon-group-title">{{ group.name }}</h3>
      <div class="icon-grid">
        <button
          v-for="icon in group.icons"
          :key="icon.name"
          class="icon-card"
          :class="{ copied: copiedName === icon.name }"
          @click="copy(icon.name)"
        >
          <span class="icon-preview mdi" :class="'mdi-' + icon.mdi" />
          <span class="icon-name">{{ copiedName === icon.name ? 'Copied!' : icon.name }}</span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import iconsData from '../../../../common/assets/icons.json'

const GROUP_ORDER = [
  'Lighting',
  'Climate & Air',
  'Covers & Shading',
  'Security & Access',
  'Power & Energy',
  'Weather',
  'Appliances',
  'Media & Entertainment',
  'Water & Outdoors',
  'Network & Tech',
  'Rooms & Furniture',
  'Vehicles',
  'Health',
  'General',
]

const ICON_GROUPS = {
  'Ceiling Light': 'Lighting',
  'Ceiling Lights Multiple': 'Lighting',
  'Chandelier': 'Lighting',
  'Desk Lamp': 'Lighting',
  'Downlight': 'Lighting',
  'Floor Lamp': 'Lighting',
  'Lamp': 'Lighting',
  'Lamp Outline': 'Lighting',
  'LED Strip': 'Lighting',
  'LED Strip Variant': 'Lighting',
  'LED Strip Variant Off': 'Lighting',
  'Light Switch': 'Lighting',
  'Lightbulb': 'Lighting',
  'Lightbulb Group': 'Lighting',
  'Lightbulb Group Outline': 'Lighting',
  'Lightbulb Night': 'Lighting',
  'Lightbulb Night Outline': 'Lighting',
  'Lightbulb Off': 'Lighting',
  'Lightbulb On Outline': 'Lighting',
  'Lightbulb Outline': 'Lighting',
  'Lightbulb Spot': 'Lighting',
  'Lightbulb Spot Off': 'Lighting',
  'Lightbulb Variant': 'Lighting',
  'Lightbulb Variant Outline': 'Lighting',
  'Outdoor Lamp': 'Lighting',
  'Spotlight': 'Lighting',
  'String Lights': 'Lighting',
  'String Lights Off': 'Lighting',
  'Wall Sconce': 'Lighting',

  'Air Conditioner': 'Climate & Air',
  'Air Filter': 'Climate & Air',
  'Air Purifier': 'Climate & Air',
  'Ceiling Fan': 'Climate & Air',
  'Fan': 'Climate & Air',
  'Fire': 'Climate & Air',
  'Fireplace': 'Climate & Air',
  'Heat Pump': 'Climate & Air',
  'HVAC': 'Climate & Air',
  'HVAC Off': 'Climate & Air',
  'Humidifier': 'Climate & Air',
  'Radiator': 'Climate & Air',
  'Radiator Off': 'Climate & Air',
  'Snowflake': 'Climate & Air',
  'Snowflake Alert': 'Climate & Air',
  'Snowflake Thermometer': 'Climate & Air',
  'Thermometer': 'Climate & Air',
  'Thermometer Alert': 'Climate & Air',
  'Thermometer High': 'Climate & Air',
  'Thermometer Low': 'Climate & Air',
  'Thermometer Water': 'Climate & Air',
  'Thermostat': 'Climate & Air',
  'Thermostat Auto': 'Climate & Air',
  'Home-Thermostat': 'Climate & Air',

  'Blinds': 'Covers & Shading',
  'Blinds Horizontal': 'Covers & Shading',
  'Blinds Horizontal Closed': 'Covers & Shading',
  'Blinds Open': 'Covers & Shading',
  'Arrow Down': 'Covers & Shading',
  'Arrow Up': 'Covers & Shading',
  'Chevron Down': 'Covers & Shading',
  'Chevron Up': 'Covers & Shading',
  'Curtains': 'Covers & Shading',
  'Curtains Closed': 'Covers & Shading',
  'Roller Shade': 'Covers & Shading',
  'Roller Shade Closed': 'Covers & Shading',
  'Stop': 'Covers & Shading',
  'Window Closed': 'Covers & Shading',
  'Window Open': 'Covers & Shading',
  'Window Shutter': 'Covers & Shading',
  'Window Shutter Open': 'Covers & Shading',

  'Alarm': 'Security & Access',
  'Alarm Light': 'Security & Access',
  'Bell': 'Security & Access',
  'Camera': 'Security & Access',
  'CCTV': 'Security & Access',
  'Door': 'Security & Access',
  'Door Open': 'Security & Access',
  'Doorbell': 'Security & Access',
  'Garage': 'Security & Access',
  'Garage Open': 'Security & Access',
  'Garage Open Variant': 'Security & Access',
  'Garage Variant': 'Security & Access',
  'Gate': 'Security & Access',
  'Gate Open': 'Security & Access',
  'Key': 'Security & Access',
  'Lock': 'Security & Access',
  'Lock Open': 'Security & Access',
  'Lock Open Outline': 'Security & Access',
  'Lock Outline': 'Security & Access',
  'Motion Sensor': 'Security & Access',
  'Security': 'Security & Access',
  'Smoke Detector': 'Security & Access',

  'Battery': 'Power & Energy',
  'Battery 10%': 'Power & Energy',
  'Battery 20%': 'Power & Energy',
  'Battery 30%': 'Power & Energy',
  'Battery 40%': 'Power & Energy',
  'Battery 50%': 'Power & Energy',
  'Battery 60%': 'Power & Energy',
  'Battery 70%': 'Power & Energy',
  'Battery 80%': 'Power & Energy',
  'Battery 90%': 'Power & Energy',
  'Battery Alert': 'Power & Energy',
  'Battery Charging': 'Power & Energy',
  'Battery Charging 100': 'Power & Energy',
  'Battery Charging 70': 'Power & Energy',
  'Battery High': 'Power & Energy',
  'Battery Low': 'Power & Energy',
  'Battery Medium': 'Power & Energy',
  'Battery Off': 'Power & Energy',
  'Battery Outline': 'Power & Energy',
  'Battery Unknown': 'Power & Energy',
  'Current AC': 'Power & Energy',
  'Current DC': 'Power & Energy',
  'Flash': 'Power & Energy',
  'Gauge': 'Power & Energy',
  'Gauge Empty': 'Power & Energy',
  'Gauge Full': 'Power & Energy',
  'Gauge Low': 'Power & Energy',
  'Grid Export': 'Power & Energy',
  'Grid Import': 'Power & Energy',
  'Grid Off': 'Power & Energy',
  'Lightning Bolt': 'Power & Energy',
  'Meter Electric': 'Power & Energy',
  'Meter Gas': 'Power & Energy',
  'Power': 'Power & Energy',
  'Power Plug': 'Power & Energy',
  'Power Socket EU': 'Power & Energy',
  'Power Socket UK': 'Power & Energy',
  'Power Socket US': 'Power & Energy',
  'Solar Panel': 'Power & Energy',
  'Solar Panel Large': 'Power & Energy',
  'Solar Power': 'Power & Energy',
  'Solar Power Variant': 'Power & Energy',
  'Transmission Tower': 'Power & Energy',
  'Wall Outlet': 'Power & Energy',
  'Wind Power': 'Power & Energy',
  'Wind Turbine': 'Power & Energy',
  'Wind Turbine Alert': 'Power & Energy',
  'Wind Turbine Check': 'Power & Energy',

  'Sun': 'Weather',
  'Weather Cloudy': 'Weather',
  'Weather Cloudy Alert': 'Weather',
  'Weather Dust': 'Weather',
  'Weather Fog': 'Weather',
  'Weather Hail': 'Weather',
  'Weather Hazy': 'Weather',
  'Weather Hurricane': 'Weather',
  'Weather Lightning': 'Weather',
  'Weather Lightning Rainy': 'Weather',
  'Weather Night': 'Weather',
  'Weather Night Cloudy': 'Weather',
  'Weather Partly Cloudy': 'Weather',
  'Weather Partly Lightning': 'Weather',
  'Weather Partly Rainy': 'Weather',
  'Weather Partly Snowy': 'Weather',
  'Weather Partly Snowy Rainy': 'Weather',
  'Weather Pouring': 'Weather',
  'Weather Rainy': 'Weather',
  'Weather Snowy': 'Weather',
  'Weather Snowy Heavy': 'Weather',
  'Weather Snowy Rainy': 'Weather',
  'Weather Sunny': 'Weather',
  'Weather Sunny Alert': 'Weather',
  'Weather Sunny Off': 'Weather',
  'Weather Sunset': 'Weather',
  'Weather Sunset Down': 'Weather',
  'Weather Sunset Up': 'Weather',
  'Weather Tornado': 'Weather',
  'Weather Windy': 'Weather',
  'Weather Windy Variant': 'Weather',

  'Coffee Maker': 'Appliances',
  'Dishwasher': 'Appliances',
  'Dryer': 'Appliances',
  'Fridge': 'Appliances',
  'Iron': 'Appliances',
  'Kettle': 'Appliances',
  'Microwave': 'Appliances',
  'Oven': 'Appliances',
  'Washer': 'Appliances',

  'Cast': 'Media & Entertainment',
  'Gamepad': 'Media & Entertainment',
  'Headphones': 'Media & Entertainment',
  'Message Video': 'Media & Entertainment',
  'Microsoft Xbox': 'Media & Entertainment',
  'Monitor': 'Media & Entertainment',
  'Movie Roll': 'Media & Entertainment',
  'Music': 'Media & Entertainment',
  'Play Pause': 'Media & Entertainment',
  'Projector': 'Media & Entertainment',
  'Projector Off': 'Media & Entertainment',
  'Speaker': 'Media & Entertainment',
  'Skip Next': 'Media & Entertainment',
  'Skip Previous': 'Media & Entertainment',
  'Television': 'Media & Entertainment',
  'Television Off': 'Media & Entertainment',
  'Volume High': 'Media & Entertainment',

  'Fountain': 'Water & Outdoors',
  'Hot Tub': 'Water & Outdoors',
  'Humidity Alert': 'Water & Outdoors',
  'Lawnmower': 'Water & Outdoors',
  'Leaf': 'Water & Outdoors',
  'Pool': 'Water & Outdoors',
  'Shower': 'Water & Outdoors',
  'Sprinkler': 'Water & Outdoors',
  'Water': 'Water & Outdoors',
  'Water Alert': 'Water & Outdoors',
  'Water Heater': 'Water & Outdoors',
  'Water Percent': 'Water & Outdoors',

  'Application': 'Network & Tech',
  'Bluetooth': 'Network & Tech',
  'LAN': 'Network & Tech',
  'Printer': 'Network & Tech',
  'Printer 3D': 'Network & Tech',
  'Robot Vacuum': 'Network & Tech',
  'Router': 'Network & Tech',
  'Router Network': 'Network & Tech',

  'Bed': 'Rooms & Furniture',
  'Sofa': 'Rooms & Furniture',
  'Table': 'Rooms & Furniture',
  'Toilet': 'Rooms & Furniture',

  'Car Electric': 'Vehicles',
  'EV Charger': 'Vehicles',

  'Medication': 'Health',
  'Medication Outline': 'Health',
  'Pill': 'Health',
  'Pill Multiple': 'Health',

  'Account': 'General',
  'Broom': 'General',
  'Check': 'General',
  'Clock': 'General',
  'Dog': 'General',
  'Dots Horizontal': 'General',
  'Gesture Tap': 'General',
  'Home': 'General',
  'Mailbox': 'General',
  'Minus': 'General',
  'Package': 'General',
  'Package Closed': 'General',
  'Plus': 'General',
  'Progress Clock': 'General',
  'Timer': 'General',
  'Trash Can': 'General',
}

const icons = iconsData.icons
const search = ref('')
const copiedName = ref(null)
let copyTimer = null

function groupOf(icon) {
  return ICON_GROUPS[icon.name] || 'General'
}

const filteredGroups = computed(() => {
  const q = search.value.toLowerCase()
  const matched = q
    ? icons.filter(i => i.name.toLowerCase().includes(q))
    : icons

  const buckets = {}
  for (const icon of matched) {
    const g = groupOf(icon)
    if (!buckets[g]) buckets[g] = []
    buckets[g].push(icon)
  }

  return GROUP_ORDER
    .filter(g => buckets[g])
    .map(g => ({ name: g, icons: buckets[g] }))
})

const totalFiltered = computed(() =>
  filteredGroups.value.reduce((sum, g) => sum + g.icons.length, 0)
)

function copy(name) {
  navigator.clipboard.writeText(name)
  copiedName.value = name
  clearTimeout(copyTimer)
  copyTimer = setTimeout(() => { copiedName.value = null }, 1200)
}

onMounted(() => {
  if (document.querySelector('link[href*="materialdesignicons"]')) return
  const link = document.createElement('link')
  link.rel = 'stylesheet'
  link.href = 'https://cdn.jsdelivr.net/npm/@mdi/font@7.4.47/css/materialdesignicons.min.css'
  document.head.appendChild(link)
})
</script>

<style scoped>
.icon-gallery {
  margin-top: 1rem;
}

.icon-search {
  width: 100%;
  padding: 10px 14px;
  font-size: 15px;
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  outline: none;
  transition: border-color 0.2s;
  font-family: inherit;
}

.icon-search:focus {
  border-color: var(--vp-c-brand-1);
}

.icon-count {
  font-size: 13px;
  color: var(--vp-c-text-3);
  margin: 8px 0 4px;
}

.icon-group {
  margin-top: 1.25rem;
}

.icon-group-title {
  font-size: 15px;
  font-weight: 600;
  color: var(--vp-c-text-1);
  margin: 0 0 10px;
  padding-bottom: 6px;
  border-bottom: 1px solid var(--vp-c-border);
}

.icon-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
  gap: 10px;
}

.icon-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 6px;
  padding: 14px 8px 10px;
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  background: var(--vp-c-bg-soft);
  cursor: pointer;
  transition: border-color 0.2s, transform 0.15s, background-color 0.2s;
  font-family: inherit;
  color: var(--vp-c-text-1);
}

.icon-card:hover {
  border-color: var(--vp-c-brand-1);
  transform: translateY(-2px);
}

.icon-card.copied {
  border-color: var(--vp-c-green-2);
  background: var(--vp-c-green-soft);
}

.icon-preview {
  font-size: 32px;
  line-height: 1;
  color: var(--vp-c-text-1);
}

.icon-name {
  font-size: 11px;
  text-align: center;
  line-height: 1.3;
  color: var(--vp-c-text-2);
  word-break: break-word;
}

.icon-card.copied .icon-name {
  color: var(--vp-c-green-2);
  font-weight: 600;
}
</style>
