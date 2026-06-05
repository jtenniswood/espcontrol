# Solar Energy Card

The Solar card shows a live or daily summary of your solar energy system on a single grid tile. Tap to open a full breakdown.

## What it does

- **Tile face:** shows a single hero value — the net energy flow — in green (surplus/exporting) or red (importing from grid). A battery percentage corner appears in the top-right when a battery field is configured.
- **Tap → modal:** a breakdown list with one row per configured field, showing each value and its unit verbatim from Home Assistant.

## Mode: Live vs Today

Set once when adding the card. **Live** expects instantaneous power entities (W or kW); **Today** expects cumulative energy-today entities (kWh). You need separate cards if you want both views.

## Six optional fields

All fields are optional — configure only the ones you have. The card adapts to whatever subset is wired up.

| Field | Typical entity | Notes |
|---|---|---|
| Production | `sensor.solar_power` | Power from panels |
| Consumption | `sensor.house_power` | Total home usage |
| Net Production | `sensor.net_power` | If your integration provides it directly |
| Battery | `sensor.battery_soc` | Shows as corner % on tile face |
| From Grid | `sensor.grid_import_power` | `↓` arrow in modal |
| To Grid | `sensor.grid_export_power` | `↑` arrow in modal |

## How the hero value is determined

The tile picks the best "Net" value it can from what's configured:

1. **Net Production entity** (if configured and available)
2. **Production − Consumption** (computed, only when both are available and share the same unit)
3. **Production** alone
4. **First available field** (fallback when only one field is wired)

Positive values are green (surplus), negative values are red (importing).

## Values and units

Values and units are shown **verbatim from Home Assistant** — the card does not convert between W and kW. If your integration reports in W, it shows in W. Configure your HA entities to report in the unit you prefer.

## Tested on

Guition ESP32-S3-4848S040 (4-inch, 480×480) at normal, wide, tall, and large tile sizes.
