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

## Reset the display

Open **Settings → System → Reset** to choose:

- **Reset customization** removes cards, layouts, subpages, actions and device preferences, while keeping the saved Wi-Fi connection and Home Assistant encryption key. The display restarts into card setup.
- **Factory reset** also removes saved Wi-Fi credentials and the Home Assistant encryption key. Type `RESET` to confirm, then follow the display's first-time setup instructions. Ethernet displays remain accessible through their wired network. Home Assistant may need to be configured again; its integration records and automations are not deleted.

Export a backup first if you want to restore your customization later. Backups do not restore the display's Wi-Fi login or Home Assistant encryption key. They can still contain passwords saved in Wi-Fi Sharing cards. Both options keep the installed firmware and its built-in defaults. Names, credentials and other settings compiled into a custom ESPHome YAML remain; install stock firmware to restore stock defaults.

The Reset section appears only on firmware that supports it. After reset, reload any other open setup pages before editing. A reset interrupted by power loss resumes at startup. If storage cleanup fails, startup remains paused and the serial log reports `Reset recovery`; cleanup retries every ten seconds. Do not restore a backup until reset has completed.

Reset clears operational configuration; it is not a forensic secure-erasure feature.
