---
title: EspControl Backup and Restore
description:
  How to export and import your EspControl panel configuration as a backup file.
---

# Backup

You can save your entire panel configuration as a file and restore it later. You'll find these options in the **Settings** tab on the [Setup](/features/setup) page, under the **Backup** section.

- **Export** - saves your entire setup (cards, subpages, card background images, colours, brightness, screen schedule, display settings, Home Assistant artwork settings, and firmware update preferences) as a file you can keep as a backup.
- **Import** - loads a previously saved file to restore your setup. If you're loading a backup from a different-sized panel, the cards are rearranged to fit automatically. New images are staged until the panel confirms that the restored card settings were saved; an interrupted import is cleaned up when the panel restarts.

If an image cannot be read during export, you can retry, cancel, or explicitly export a configuration-only backup without images. Displays without image storage export their configuration normally.

Backup files are versioned so newer EspControl releases can keep importing older backups safely. Older version 1 backups still import, and new exports use version 2 while keeping the same readable layout fields for compatibility.

## Compatibility Notes

EspControl keeps old saved card strings readable during upgrades. That means cards created before newer card options or compact subpage storage were added should still load, display, and export correctly after an update.

New backup exports continue to use `version: 2` with `format: "espcontrol.backup"`. The readable JSON file also includes a `native_config` section. It records the panel profile, document version, and an encoded copy of the configuration so a firmware version with the native configuration service can restore it exactly. Older panels continue to import the same file through the compatible JSON fields.

When importing a backup from a different-sized panel, EspControl keeps the saved card order where it can and rearranges cards that no longer fit the target screen. Subpages are moved with their parent card when the parent card is kept.
