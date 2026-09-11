---
title: EspControl Backup and Restore
description:
  How to export and import your EspControl panel configuration as a backup file.
---

# Backup

You can save your entire panel configuration as a file and restore it later. You'll find these options in the **Settings** tab on the [Setup](/features/setup) page, under the **Backup** section.

- **Export** - saves your entire setup (cards, subpages, colours, brightness, screen schedule, display settings, Home Assistant artwork settings, and firmware update preferences) as a file you can keep as a backup.
- **Import** - loads a previously saved file to restore your setup. If you're loading a backup from a different-sized panel, the cards are rearranged to fit automatically.

Backup files are versioned so newer EspControl releases can keep importing older backups safely. Older version 1 backups still import, and new exports use version 2 while keeping the same readable layout fields for compatibility.

## Compatibility Notes

EspControl keeps old saved card strings readable during upgrades. That means cards created before newer card options or compact subpage storage were added should still load, display, and export correctly after an update.

New backup exports continue to use `version: 2` with `format: "espcontrol.backup"`. The readable JSON file also includes a `native_config` section. It records the panel profile, document version, and an encoded copy of the configuration so a firmware version with the native configuration service can restore it exactly. Older panels continue to import the same file through the compatible JSON fields.

When importing a backup from a different-sized panel, EspControl keeps the saved card order where it can and rearranges cards that no longer fit the target screen. Subpages are moved with their parent card when the parent card is kept.

## Panel Names in Backups

Custom-named panels include their name and device suffix in backup filenames,
for example `espcontrol-4-inch-kitchen-a1b2c3-2026-09-11.json`.

When importing a backup with panel naming metadata, **Also restore panel name**
is off by default. Leave it off to keep the destination's name and address.
Selecting it copies the name, generates an address using the destination panel's
own suffix, and restarts after the configuration restores successfully. The
source panel's hardware identity is never copied. Older backups keep the
current panel name.
