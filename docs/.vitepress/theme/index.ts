import { h } from 'vue'
import DefaultTheme from 'vitepress/theme'
import './styles.css'
import EspInstallButton from './components/EspInstallButton.vue'
import EspInstallSelector from './components/EspInstallSelector.vue'
import C6RecoveryCallout from './components/C6RecoveryCallout.vue'
import C6RecoverySelector from './components/C6RecoverySelector.vue'
import GitHubStars from './components/GitHubStars.vue'
import IconGallery from './components/IconGallery.vue'

export default {
  extends: DefaultTheme,
  Layout() {
    return h(DefaultTheme.Layout, null, {
      'nav-bar-content-after': () => h(GitHubStars),
    })
  },
  enhanceApp({ app }) {
    app.component('EspInstallButton', EspInstallButton)
    app.component('EspInstallSelector', EspInstallSelector)
    app.component('C6RecoveryCallout', C6RecoveryCallout)
    app.component('C6RecoverySelector', C6RecoverySelector)
    app.component('GitHubStars', GitHubStars)
    app.component('IconGallery', IconGallery)
  },
}
