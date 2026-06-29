# How to use

Add to your ESPHome device config:
```yaml
external_components:
  - source:
      type: local
      path: ../../components
    components: [lvgl_screenshot]

lvgl_screenshot:
  id: my_screenshot
  port: <PORT>
```

## Interact
Screenshot: `http://<IP>:<PORT>/screenshot`
Touch: `curl -X POST http://<IP>:<PORT>/touch -H "Content-Type: application/json" -d '{"x":360, "y": 360, "pressed": true}'
