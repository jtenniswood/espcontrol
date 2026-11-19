---
title: "Home Assistant Sensor Cards"
description:
  How to display live readings, durations, text, or icon states from Home Assistant on EspControl.
---

# Sensor

A Sensor card is read-only. It displays a Home Assistant `sensor`, `binary_sensor`, or `text_sensor`; it can also use a [Local Sensor](/card-types/local-sensors) from the panel itself.

![Sensor card showing 0 kph wind speed](/images/card-sensor.png)

## Set Up a Sensor Card

1. Select a card and change its type to **Sensor**.
2. Leave **Source** as **Home Assistant**, choose the display type, and enter the sensor entity.
3. Set a label, unit, and icon as needed.

| Type | What it shows |
|---|---|
| **Numeric** | A live number, optional unit, and label. **Large Sensor Numbers** is available on Large cards. |
| **Time** | A compact duration such as `36m` or `1h 30m`. Leave **Incoming Value Unit** on Auto unless the entity does not report a usable unit. |
| **Text** | The live state beside a chosen icon. Advanced settings can replace up to two raw states with friendlier text. |
| **Icon** | A normal and optional active icon for a status-style sensor. |

Choose **Lit When Active** when an active status should use the on colour. It is not available for Time cards. Numeric cards treat values above zero as active; Text and Icon cards follow recognised Home Assistant active states.

## Useful Details

- A Time card needs a value in days, hours, minutes, seconds, milliseconds, or microseconds. You can select the unit manually when Auto cannot identify it.
- Icon cards show a dimmed dash while waiting for a reading or when the entity is unknown or unavailable. Their chosen icons return when valid data arrives. Door/Window and Presence cards use the same treatment, so missing data never looks like a confirmed closed or clear state.
- Numeric and Time cards show a placeholder for unknown, unavailable, or invalid values.
- Changes made in Home Assistant update the panel automatically.
- To show a device-local sensor, change **Source** to **Local Sensor** and follow the [Local Sensor](/card-types/local-sensors) setup.

| Example entity | Suggested type |
|---|---|
| `sensor.living_room_temperature` | Numeric |
| `sensor.ups_battery_runtime` | Time |
| `binary_sensor.laundry_running` | Icon |
| `text_sensor.washing_machine_status` | Text |

## Power Dashboard

Choose **Power** to show instantaneous Home consumption in **watts (W)**. Tap the card
on the panel to open **Power Flow**. The label defaults to **Power Dashboard**;
the secondary line summarizes Battery and Grid activity. Power is a navigation
card: tapping opens the view without toggling an entity or calling a service.
Use the standard [Custom Colours controls](/features/appearance#active-colour) to
change the Power card; Power Flow keeps the native theme's information-card surface.

Power needs **at least two grid columns** on the main grid and subpages. The
editor only expands into free space; move neighboring cards yourself if needed.
Previously saved narrow cards retain label-only navigation until resized, without
moving or deleting any card. Unsupported narrow saves/restores are rejected
before writes. All Power values use whole watts, rounded to nearest with exact
halves away from zero (`1234.5 → 1235 W`, `-328.5 → -329 W`, `-0.3 → 0 W`).
Calculations retain full input precision. Older saved decimal-precision fields
remain compatible but no longer affect displayed values or Flow layout; input sensors may still
report W or kW. Grid/Battery values in Power Flow retain their signs: negative means exporting
or charging. The muted status caption also names the direction. Battery SOC
and activity share one row beneath its signed watt value.

On supported landscape 7-inch panels, the diagram occupies the right three
columns and three rows; 10-inch panels use the right three columns and four rows.
A subpage-style Back card and four native Sensor-style readings fill the left
two columns. Each reading shows watts above its label, with Grid/Battery activity
below. Smaller or portrait layouts retain the compact view and Back arrow. Both
areas share the same readings. The landscape page uses six separate native cards,
including the diagram, aligned with Home's grid against the normal page background.
On every display, the diagram uses the native non-interactive information-card
surface and follows theme changes while open. Node positions and arrow attachment
points stay fixed as readings, activity and SOC availability change.

Configure any installed components with Home Assistant `sensor.*` entities reporting W or kW:

| Reading | Default sign convention |
|---|---|
| Home power | Positive consumption |
| Solar power | Positive production |
| Grid power | Positive importing; negative exporting |
| Battery power | Positive discharging; negative charging |

**Battery SOC** is optional and must report a percentage. Its absence never
prevents Battery power flows. Enable Grid/Battery sign inversion only for sensors
using the opposite convention.

Home is always included and displayed; its power sensor is optional. Solar,
Grid and Battery each have an **included in system** switch, enabled by default
for existing configurations. Uncheck it only when the component is physically absent
from the configured electrical system: that contribution is then zero for balance
checking. Its node/landscape reading card is hidden without moving other nodes or
cards. An included component with an empty sensor field remains installed but
**unmeasured**, never zero. In the Flow diagram, missing or unavailable power
readings show `—`; the central status chip explains the calculation state. Valid
activity and optional Battery SOC remain visible. The landscape Sensor cards keep
their detailed **No measurement** / **Unavailable** captions. Configured sensors
reporting unknown, unavailable, invalid or explicitly stale data also retain their
node with no current numeric value. Valid other readings and Battery/Grid activity
remain visible. When neither Battery nor Grid is included, the activity subtitle
is omitted rather than claiming Idle. Older saves that excluded Home are accepted:
Home becomes included, its sensor remains unchanged, and other settings are preserved.
An empty or unavailable Home sensor never becomes `0 W`; only a valid zero reading does.

Arrows require a valid measurement for **every included component**. Otherwise
Power shows readings/status only: it does not solve for missing measurements or
assume that an unmeasured installed component is idle. For example:

| Included components | Required measurements for arrows | Interpretation |
|---|---|---|
| Grid + Home | Both | Grid supplies Home; persistent export conflicts with this boundary |
| Solar + Grid + Home | All three | Solar-first Home/export allocation; no Battery contribution |
| Grid + Battery + Home | All three | Import/discharge/charge/export according to measured signs |
| All four | All four | Full-system Solar-first allocation |
| Solar + Home, Grid installed but unmeasured | Grid also required | Readings only; no unsupported attribution |
| Solar + Grid, Home installed but unmeasured | Home also required | Readings only |
| Home with any combination of Solar, Grid and Battery (eight topologies) | Every included component, including Home | Same balance/capacity rules; isolated nonzero sources/sinks are inconsistent above tolerance |

The signed balance is **Solar + Grid + Battery − Home**. Allowed mismatch is
**max(10 W, 2% of the larger source/sink total)**; the idle threshold remains
**1 W**, applied to full-precision readings. A valid zero/near-zero system is
separate from missing data. Inconsistent readings hide all arrows.

Paths are **inferred visual allocations, not measured physical routes**. Available
capacity is consumed in order: Solar→Home, Solar→Battery, Solar→Grid,
Battery→Home, Battery→Grid, Grid→Home, Grid→Battery. No path is negative or exceeds
its source/sink capacity. With one active source or one active sink the routes are
determined by the observations (within balance tolerance); multiple sources and
sinks use the documented Solar-first convention, not a claim about actual wiring.
Even apparently plausible partial routes are suppressed if an installed boundary
contribution is unknown.

Valid updates arriving within a fixed **200 ms** window are published together to
reduce mixed snapshots; the window is not restarted indefinitely. Unavailable
inputs clear arrows immediately; persistent imbalance hides them after the
window. Disconnect clears retained readings, and reconnect requires fresh state
and units. Callback arrival ages are not measurement timestamps: numeric HA values
that silently become stale cannot be detected without trustworthy sensor metadata;
no arbitrary age limit is applied to steady sensors that do not send updates.

The web card preview uses placeholders, not live measurements. It follows the
selected Dark/Light theme, or Active Theme in Auto mode.

Current firmware supports **4 KiB per card or subpage**. Save and restore use
this panel's advertised capacity and reject oversized settings without truncation.
Older firmware may retain a 255-byte card limit or shared chunked subpage storage;
shorter entity IDs/labels or omitting optional SOC can help on those installations.
