# ha_calendar Widget Design

**Date:** 2026-06-02
**Status:** Approved

## Overview

A new `ha_calendar` card type for espcontrol that shows upcoming or active Google Calendar events from one or more Home Assistant `calendar.*` entities. Tapping the card opens a modal with today's remaining events.

---

## 1. Card Identity & Configuration

**Card type:** `ha_calendar`
**JS file:** `src/webserver/types/ha-calendar.js`
**HA domains:** `calendar`

### Settings Panel

| Field | Control | Notes |
|---|---|---|
| **Entities** | Text input | Comma-delimited list of `calendar.*` entity IDs |
| **Display mode** | Segment control | `Current` / `Next` |
| **Modal layout** | Segment control | `Time Column` / `Compact` |
| **Label** | Text input | Optional override; defaults to friendly name (single entity) or `"Calendar"` (multiple) |

**Firmware field mapping:**
- `p.entity` — comma-delimited entity list (parsed inside `create_ha_calendar_card_context()`)
- `p.options` — display mode via `cfg_option_value(p.options, "display_mode")`: `"current"` or `"next"` (empty = next)
- `p.options` — modal layout via `cfg_option_value(p.options, "modal_layout")`: `"column"` or `"compact"`
- `p.label` — optional label override

---

## 2. Card Face Rendering

### Layout (B3)

```
┌─────────────────┐
│ In              │  ← 8px muted label
│ 34          min │  ← big number (42px), unit beside at footer height
│                 │
│ Meet for lunch  │  ← 10px event title, truncates with ellipsis
└─────────────────┘
```

### Display Modes

**Next mode** (`p.precision != "current"`): Shows countdown to the next event across all configured entities.

**Current mode** (`p.precision == "current"`): Shows time remaining in the active event. Falls back to "Free" when no event is active.

### Color States (Next mode)

| Condition | Appearance |
|---|---|
| > 5 min until start | White number, muted "In" label and unit, white title |
| ≤ 5 min until start | Whole card in accent color (number, unit, "In", title, border) |
| 0–5 min into event | `"Now"` in accent color, warm background tint, event title in accent |
| > 5 min into event | `"Now"` greyed out, event title dimmed |
| No events today | Calendar icon · `"—"` · label at bottom |

### Unit Thresholds

| Time to event | Unit shown |
|---|---|
| < 60 minutes | `min` |
| < 48 hours | `hr` |
| ≥ 48 hours | `days` |

### Back-to-Back Layout

Activated when the current event's `end_time` matches the next event's `start_time` (next event data comes from the cached `calendar.get_events` response).

```
┌─────────────────────────┐
│ ▌ Now · ends 1:00 pm    │  ← grey left bar, 8px muted time, grey title (2-line wrap)
│ ▌ Q4 Budget Planning    │
├─────────────────────────┤
│ ▌ In 3 min · 1:00 pm    │  ← accent left bar, 8px accent time, accent title (2-line wrap)
│ ▌ Team Standup          │
└─────────────────────────┘
```

Event names wrap to a maximum of 2 lines; longer names are clipped with `…`.

---

## 3. Modal (Tap to Open)

Calls `calendar.get_events` for each configured entity on tap, merges results sorted by `start_time`, filters to today's remaining events only.

**Header:** `"{Friendly name} · Today"` (single entity) or `"Today"` (multiple entities)

### Layout A — Time Column

```
← Back          Erik · Today
────────────────────────────
12:00 │ Q4 Budget Planning
1:00  │ Now
──────┤─────────────────────
1:00  │ Team Standup
1:30  │ In 3 min
──────┤─────────────────────
3:00  │ Engineering Review
4:00  │ In 2 hr
            No more events today
```

Start + end time stacked in a left column, accent vertical bar, title + countdown on the right.

### Layout B — Compact

```
← Back          Erik · Today
────────────────────────────
Q4 Budget Planning    Now
12:00 – 1:00 pm
────────────────────────────
Team Standup          In 3 min
1:00 – 1:30 pm
────────────────────────────
Engineering Review    In 2 hr
3:00 – 4:00 pm
            No more events today
```

Title + countdown on one line, time range below in smaller text.

### Row Coloring

| Condition | Style |
|---|---|
| Active event (state = on) | Amber text + warm background row |
| ≤ 5 min to start | Accent color text |
| Everything else | White title, muted time / countdown |

### Loading / Error States

Follows the todo modal pattern:
- `"Loading"` while fetching
- `"Waiting for Home Assistant"` when disconnected, with auto-retry on reconnect
- `"Could not load"` on fetch failure

---

## 4. Firmware Architecture

### New File

`components/espcontrol/button_grid_ha_calendar.h`

Included in `button_grid.h` before `button_grid_grid.h`, following the same include chain as `button_grid_todo.h`.

### Key Structs

```cpp
struct HaCalendarEvent {
  std::string start_time;   // ISO-ish: "2026-06-02 14:00:00"
  std::string end_time;
  std::string title;
};

struct HaCalendarCardCtx {
  uint32_t magic = HA_CALENDAR_CTX_MAGIC;
  std::vector<std::string> entity_ids;  // parsed from comma-delimited p.entity
  std::string configured_label;
  std::string friendly_name;            // from first entity's friendly_name attribute
  std::string display_mode;             // "current" or "next" (from cfg_option_value)
  std::string modal_layout;             // "column" or "compact" (from cfg_option_value)
  HaCalendarEvent next_event;           // cached from last get_events response
  bool back_to_back = false;
  HaCalendarEvent back_to_back_next;    // second event in a back-to-back pair
  // LVGL widget pointers and fonts — same fields as TodoCardCtx:
  // btn, icon_lbl, value_lbl, unit_lbl, label_lbl, value_font, label_font,
  // list_font, icon_font, width_compensation_percent, available
};
```

### Key Functions

| Function | Purpose |
|---|---|
| `setup_ha_calendar_card()` | Initial visual setup (hides icon, shows sensor container) |
| `create_ha_calendar_card_context()` | Allocates ctx, parses entity list, sets fonts |
| `subscribe_ha_calendar_state()` | `ha_subscribe_state()` per entity for live countdown |
| `subscribe_ha_calendar_attributes()` | `ha_subscribe_attribute()` for `start_time`, `end_time`, `message` |
| `ha_calendar_apply_card_face()` | Recomputes and sets LVGL labels from cached state |
| `ha_calendar_open_modal()` | Opens modal shell, fires `ha_calendar_request_events()` |
| `ha_calendar_request_events()` | Calls `calendar.get_events` per entity, merges via response template |
| `ha_calendar_parse_events()` | Parses pipe-delimited payload into `HaCalendarEvent` vector |
| `ha_calendar_render_modal()` | Renders event rows in the chosen layout |

### Data Flow

```
HA state subscription (per entity)
  → state ("on"/"off"), message, start_time, end_time attributes
  → ha_calendar_apply_card_face() on each update
  → countdown computed from start_time vs device clock

On tap:
  → ha_calendar_open_modal()
  → ha_calendar_request_events() fires calendar.get_events per entity
  → response template returns: "start|end|title\n" lines, today only
  → merge + sort by start_time across all entities
  → cache next event into ctx->next_event (for back-to-back detection)
  → ha_calendar_render_modal() builds LVGL list
```

### Response Template Shape

```jinja
{% set events = namespace(out='') %}
{% for e in response.get('calendar.example', {}).get('events', []) %}
  {# filter to today, format: start|end|title #}
{% endfor %}
{{ events.out }}
```

One `calendar.get_events` call per entity; results merged in C++ before rendering.

### Wiring in `button_grid_grid.h`

Two insertion points (main grid + subpage), identical to todo:

```cpp
if (p.type == "ha_calendar") {
  setup_ha_calendar_card(s, p, palette.off_val);
  return;
}
```

```cpp
if (p.type == "ha_calendar") {
  if (!p.entity.empty()) {
    HaCalendarCardCtx *ctx = create_ha_calendar_card_context(
      s, p, accent_color, off_color,
      display_sensor_font(display),
      lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
      display_icon_font(display),
      display_main_width_percent(display));
    subscribe_ha_calendar_state(ctx);
    subscribe_ha_calendar_attributes(ctx);
    lv_obj_add_event_cb(s.btn, [](lv_event_t *e) {
      HaCalendarCardCtx *ctx = (HaCalendarCardCtx *)lv_event_get_user_data(e);
      if (ha_calendar_ctx_valid(ctx)) ha_calendar_open_modal(ctx);
    }, LV_EVENT_CLICKED, ctx);
  }
  continue;
}
```

---

## 5. Card Contract

New entry in `common/config/card_contract.json`:

```json
"ha_calendar": {
  "label": "Calendar",
  "allowInSubpage": true,
  "domains": ["calendar"],
  "options": [
    {
      "name": "display_mode",
      "label": "Display Mode",
      "kind": "choice",
      "values": ["", "current"],
      "defaultValue": ""
    },
    {
      "name": "modal_layout",
      "label": "Modal Layout",
      "kind": "choice",
      "values": ["", "column"],
      "defaultValue": ""
    }
  ],
  "default": {
    "entity": "",
    "label": "",
    "icon": "Auto",
    "icon_on": "Auto",
    "sensor": "",
    "unit": "",
    "type": "ha_calendar",
    "precision": "",
    "options": ""
  }
}
```

---

## 6. JS Settings Panel (`src/webserver/types/ha-calendar.js`)

```
registerButtonType("ha_calendar", {
  label: "Calendar",
  hideLabel: true,
  cardMetadata: HA_CALENDAR_CARD_METADATA,
  renderSettings: function(panel, b, slot, helpers) {
    // 1. Entities text field (comma-delimited)
    // 2. Display mode segment: Next / Current
    // 3. Modal layout segment: Compact / Time Column
    // 4. Label text field (optional)
  },
  renderPreview: function(b, helpers) {
    // B3 layout mockup: "In" / 34 / "min" / event title
  }
})
```

---

## 7. Out of Scope

- Location display in modal rows (can be added later as a 3rd line)
- Periodic background refresh of the event cache (modal-on-demand only)
- Creating or editing events from the panel
- Multi-day or all-day event special handling beyond showing the title
