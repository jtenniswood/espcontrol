---
title: Config API
description:
  Read and write an EspControl panel's configuration over HTTP, from Home Assistant automations, scripts, or the command line.
---

# Config API

The panel serves its whole configuration as JSON over HTTP, and accepts changes the same way. It is the same data you get from **Export** on the [Backup](/features/backup) page — the `espcontrol.backup` version 2 envelope — so anything you can set in the web interface you can also read and set from a script or a Home Assistant automation.

This exists because the web interface was the only way to change most settings. Card and subpage configuration is stored in internal entities that never reach Home Assistant, so automations could not touch them at all, and there was no headless path for scripts.

Everything below works with no Home Assistant connection. The API is registered while the panel boots, before it joins your network, so it is available as soon as the web server is up.

## Security

**This adds no authentication and no new exposure.** Read this before you use it.

Your panel's web server already lets anyone on your network read *and change* every configuration value without a password:

```bash
# This has always worked, on every version, with no credentials
curl -X POST 'http://panel.local/text/Button%201%20Config/set?value=~evil'
```

The Config API is that same capability in a more usable shape. It is not a new door into the panel.

If you turn on web server authentication, the Config API is covered by it — the endpoints are registered through the same mechanism as the built-in web interface, so a username and password protects them too. Authentication is off by default, and turning it on is a separate decision that affects the whole web interface.

The practical advice is the one that already applied to these panels: keep them on a trusted network, and do not forward their web port to the internet.

## Endpoints

| Method | Path | Purpose |
| --- | --- | --- |
| `GET` | `/api/config` | The full configuration envelope |
| `GET` | `/api/config?section=…` | One section only |
| `POST` | `/api/config/set` | Change one value, or a handful |
| `POST` | `/api/config/import` | Apply a whole envelope, or any part of one |
| `POST` | `/api/config/apply` | Commit to flash, optionally reboot |
| `POST` | `/api/config/probe` | Diagnostics — echoes body size and free memory |

Responses are always JSON, including errors.

## Reading the configuration

```bash
curl -s http://panel.local/api/config > my-panel.json
```

The result is a complete, importable backup envelope — identical in shape to a browser export, with two extra fields (`exported_at` and `etag`) and a `raw` string on each card.

`?section=` narrows the response. Valid sections are `meta`, `root`, `settings`, `screen`, `cards`, and `subpages`; `all` is the default.

```bash
curl -s 'http://panel.local/api/config?section=meta'
```

```json
{
  "version": 2,
  "format": "espcontrol.backup",
  "device": "guition-esp32-s3-4848s040",
  "exported_at": "2026-07-28T17:08:35Z",
  "etag": "26c20fbc",
  "source": { "device": "guition-esp32-s3-4848s040", "slots": 9 }
}
```

```bash
curl -s 'http://panel.local/api/config?section=screen'
```

```json
{
  "screen": {
    "automatic_brightness": true,
    "schedule_enabled": false,
    "schedule_trigger": "disabled",
    "schedule_mode": "screen_off",
    "schedule_on_hour": 6,
    "schedule_off_hour": 23,
    "brightness_day": 100,
    "brightness_night": 75,
    "brightness_dawn_time": "06:00",
    "brightness_dusk_time": "18:00"
  }
}
```

Add `&slot=N` to `section=cards` to read a single card. Cards are numbered from 1.

```bash
curl -s 'http://panel.local/api/config?section=cards&slot=1'
```

```json
{
  "buttons": [
    {
      "slot": 1,
      "raw": "media_player.spotify;Spotify;Auto;Auto;play_pause;;media",
      "entity": "media_player.spotify",
      "label": "Spotify",
      "icon": "Auto",
      "sensor": "play_pause",
      "type": "media"
    }
  ]
}
```

`raw` is the card exactly as the panel stores it. The parsed fields beside it are there so you can read a card without knowing the storage format — they are informational, and an import uses `raw`.

`exported_at` is empty until the panel's clock has been set.

### Detecting changes with the ETag

Every read returns an `ETag` header and the same value as `etag` in the `meta` section. It covers the whole configuration, not just the part you asked for, so `?section=meta` is a cheap way to poll for "has anything changed".

Pass it back on an import to make a read-modify-write safe against someone editing the panel in a browser at the same time — see [Import](#import-a-whole-envelope).

## Changing one value

```bash
curl -s -X POST -H 'Content-Length: 0' \
  'http://panel.local/api/config/set?key=settings.screensaver_timeout&value=1800'
```

```json
{
  "ok": true,
  "applied": 1, "unchanged": 0, "skipped": 0, "failed": 0,
  "results": [{ "key": "settings.screensaver_timeout", "result": "ok" }]
}
```

Keys are the envelope path. `settings.<field>`, `screen.<field>`, `root.<field>`, `buttons[N]`, and `subpages[N]` all work, and a bare field name is looked up in every section:

```bash
# All four of these mean the same thing
?key=settings.screensaver_timeout
?key=screensaver_timeout
?key=buttons[3]
?key=buttons.3
```

You can also send JSON, for one key or several:

```bash
curl -s -X POST -H 'Content-Type: application/json' \
  --data-binary '{"key": "settings.timezone", "value": "Europe/London"}' \
  http://panel.local/api/config/set

curl -s -X POST -H 'Content-Type: application/json' \
  --data-binary '{"keys": {"screen.brightness_day": 90, "screen.brightness_night": 40}}' \
  http://panel.local/api/config/set
```

`set` is **not** atomic. Keys are applied in the order given, and a failure part way through leaves the earlier writes in place. Use `import` when you need all-or-nothing. The response always lists every key you sent, whatever the status code:

```json
{
  "ok": false,
  "applied": 0, "unchanged": 0, "skipped": 0, "failed": 1,
  "results": [
    { "key": "screen.brightness_day", "result": "out_of_range",
      "detail": "outside range 10..100" }
  ]
}
```

### Result values

| `result` | Meaning |
| --- | --- |
| `ok` | Written |
| `unchanged` | Already held that value, so nothing was written |
| `not_present` | This panel does not have that setting (for example screen rotation) — not an error |
| `unknown_field` | No such field in the envelope. A mistake in your request |
| `bad_value` | Not a valid value for this setting |
| `out_of_range` | A number outside the setting's limits |
| `too_long` | Longer than the panel can store |
| `wrong_domain` | The field exists but is not the kind of value you sent |

## Import a whole envelope

`import` takes a full envelope, or any subset of one — you can send only `{"settings": {...}}` if that is all you want to change.

```bash
curl -s -X POST -H 'Content-Type: application/json' \
  --data-binary @my-panel.json \
  http://panel.local/api/config/import
```

**Import is atomic by default.** Every value is validated first, and if anything fails, nothing is written at all — you get a `400` with the full list of problems and `"applied_any": false`. This is deliberate: a half-applied configuration is worse than a rejected one.

| Query flag | Effect |
| --- | --- |
| `?dry_run=1` | Validate and report, write nothing |
| `?atomic=0` | Best effort — apply what validates, report the rest |
| `?if_match=<etag>` | Refuse with `412` unless the configuration still has this ETag |

```bash
# Check a backup would apply cleanly, without touching the panel
curl -s -X POST -H 'Content-Type: application/json' \
  --data-binary @my-panel.json \
  'http://panel.local/api/config/import?dry_run=1'

# Only apply if nobody has changed anything since we read it
curl -s -X POST -H 'Content-Type: application/json' \
  --data-binary @my-panel.json \
  'http://panel.local/api/config/import?if_match=26c20fbc'
```

An import commits to flash itself, so a restore survives a power cut without a separate `apply`.

The response is the same report as `set`, plus `etag`, `dry_run`, `atomic`, `applied_any`, and a `warning` when something was skipped rather than rejected — a card beyond the panel's slot count, for example.

**Check `applied_any`, not `applied`.** On a dry run or a rejected atomic import, `applied` counts the values that *would* have been written — nothing was. `applied_any: false` is the field that tells you the panel is unchanged.

Very large reports drop their per-key list rather than grow without bound. If that happens you get `results_truncated: true`; the counts remain exact.

### What import will refuse

Envelopes are validated the same way the web interface validates them, with the same messages:

- **Version 1 backups.** Restore these from the web interface instead. Fitting an old layout onto the current grid is the browser's job, and duplicating that logic in the firmware would be a good way to get it subtly wrong.
- **Newer versions**, from a future EspControl release.
- **Anything that is not an `espcontrol.backup` envelope.**

### Cards and subpages

Cards import from their `raw` string. A panel export always has one, so panel-to-panel copies and restores are exact. A **browser** export has no `raw`, and those cards are skipped with a warning — import a browser export from the web interface.

Subpages must be the pre-serialised string form. The `subpage_objects` field is ignored.

## Commit and reboot

Values set through `set` are held in memory and written to flash when the panel gets around to it, the same as changes made in the web interface. `apply` forces the commit:

```bash
curl -s -X POST -H 'Content-Length: 0' http://panel.local/api/config/apply
```

```json
{ "ok": true, "synced": true, "reboot": false }
```

Add `?reboot=1` to restart afterwards. The response is sent first, so you can tell a reboot from a failure.

Only a few settings need a reboot to take effect — the same ones that ask you to use **Apply Configuration** in the web interface.

## Home Assistant

```yaml
rest_command:
  espcontrol_set:
    url: "http://panel.local/api/config/set"
    method: POST
    content_type: "application/json"
    payload: '{"key": "{{ key }}", "value": {{ value | tojson }}}'

  espcontrol_import:
    url: "http://panel.local/api/config/import"
    method: POST
    content_type: "application/json"
    payload: "{{ envelope | tojson }}"
```

```yaml
automation:
  - alias: Dim the panel when the film starts
    triggers:
      - trigger: state
        entity_id: media_player.projector
        to: "playing"
    actions:
      - action: rest_command.espcontrol_set
        data:
          key: screen.brightness_day
          value: 20
        response_variable: result
      - if: "{{ result.status != 200 }}"
        then:
          - action: notify.persistent_notification
            data:
              message: "Panel brightness failed: {{ result.content.results }}"
```

`rest_command` **logs** a non-2xx response rather than failing the automation, so check `result.status` yourself if it matters.

To read values back into Home Assistant, use a `rest` sensor with `?section=` — the full envelope is far too large for a sensor state:

```yaml
rest:
  - resource: "http://panel.local/api/config?section=settings"
    scan_interval: 300
    sensor:
      - name: "Panel screensaver timeout"
        value_template: "{{ value_json.settings.screensaver_timeout }}"
        json_attributes_path: "$.settings"
        json_attributes: [timezone, clock_bar, screensaver_mode]
```

## Things that will trip you up

**Every POST needs a `Content-Length` header.** The panel's web server rejects a POST without one with `411 Length Required`, before it looks at anything else.

- Sending a body: use `--data-binary @file`. Never `@-` — reading from a pipe makes curl send a chunked request with no `Content-Length`.
- Sending no body (`set` and `apply` with query parameters): add `-H 'Content-Length: 0'` explicitly. `curl -X POST` does not send one on its own.
- Home Assistant's `rest_command` sets it for you.

**Send `Content-Type: application/json` with a JSON body.** Without it the web server hands your body to its form parser and the endpoint sees an empty request.

**Query flags need a value.** `?dry_run=1` works; a bare `?dry_run` is invisible to the panel and reads as off.

**A large read can interrupt an open web interface.** The panel keeps only a few connections, and reading the full configuration while the setup page is open can close the browser's live-update stream. The page recovers on reload. Prefer `?section=` when a panel is in front of someone.

**Card slots are numbered from 1**, matching the envelope and the web interface.
