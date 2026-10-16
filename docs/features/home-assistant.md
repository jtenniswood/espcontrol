---
title: "Home Assistant Settings"
description: Check EspControl camera and artwork connections, or configure a manual Home Assistant image endpoint.
---

# Home Assistant Settings

Leave **System > Home Assistant Settings > Connection** on **Automatic** for normal local installations. The panel discovers a local Home Assistant image endpoint and shows its address, source, and health.

If controls work but images do not:

1. Check that the camera image or player artwork appears in Home Assistant.
2. Compare the panel's displayed endpoint with **Home Assistant > Settings > System > Network > Home Assistant URL > Local network**.
3. If needed, choose **Manual** and enter **Home Assistant Host**, **Home Assistant Protocol**, and **Home Assistant Port**. Leave Host blank to use the connected Home Assistant API address. Use Manual for a hostname-based proxy or a proxy on another host.
4. Retry the image. **Connection checked** means Home Assistant responded; **Images received** confirms a successful image download.

Automatic discovery accepts an advertised local URL only on the connected Home Assistant server's IP. If it is unreachable, the panel checks advertised/configured protocol and port fallbacks; it does not scan ports. The separate Internet URL is not used. Image access errors do not trigger a server switch.

For HTTPS, use a host matching its certificate. A self-signed certificate must be trusted separately. **Access denied** needs an authentication or IP-ban check in Home Assistant; switching protocols will not fix it. See [Camera troubleshooting](/card-types/cameras#troubleshooting) or [missing artwork](/settings/sleep#track-details-work-but-artwork-is-missing).
