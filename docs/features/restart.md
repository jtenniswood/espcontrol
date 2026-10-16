---
title: "Restart Your Panel"
description: Restart an EspControl panel from Home Assistant without changing its saved settings.
---

# Restart Your Panel

In Home Assistant, open **Settings > Devices & services > ESPHome**, choose the panel, and press **Restart** under its configuration controls. Saved cards and settings are kept.

For an automation, select the panel's Restart entity with the `button.press` action. Replace this example ID with the actual entity:

```yaml
actions:
  - action: button.press
    target:
      entity_id: button.your_panel_restart
```

For automations triggered by Home Assistant startup, wait until that entity is available before pressing it. Use **Apply Configuration** in the web page after editing settings.
