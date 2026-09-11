# Task Router

Use this page only to choose a workflow. The linked playbook owns the exact
files, generators, checks, and stop conditions.

## Card Type or Card Behavior

Use [Add or Change a Card Type](playbooks/add-card-type.md) for card metadata,
settings, previews, saved options, firmware rendering, Home Assistant actions,
subscriptions, or subpage behavior.

## Saved Settings or Backups

Use [Change Saved Config](playbooks/change-saved-config.md) for compact card
strings, backup/import/export shape, aliases, normalization, or migrations.

## Supported Hardware or Device Profiles

Use [Add or Change a Supported Device](playbooks/add-supported-device.md) for a
new display, orientation, grid, device profile, package, build target, or
release-facing device name.

## Fonts or Icons

Use [Change Fonts or Icons](playbooks/change-fonts-or-icons.md) for icon names,
glyph sets, per-device font definitions, or firmware font roles.

## Web Configurator

Use [Change the Web Configurator](playbooks/change-web-configurator.md) for
shared setup-page behavior, card editors, previews, browser state, or generated
web bundles.

## Firmware UI

Use [Change Firmware UI](playbooks/change-firmware-ui.md) for LVGL layout,
runtime state, modals, display lifecycle, Home Assistant bindings, or
device-specific UI behavior.

## Release Workflow

Use [Change the Release Workflow](playbooks/change-release-workflow.md) for
release manifests, draft publication, firmware assets, workflow definitions,
public URLs, or release-confidence checks.

## Public Documentation

Edit handwritten pages under `docs/`. If the page is generated, use
[Source of Truth Contract](source-of-truth.md) to choose the authored input,
then use [Check Matrix](check-matrix.md) for verification.

## Unsure or Diagnosing a Failure

Use [Failure Cookbook](failure-cookbook.md) when behavior is already broken, or
[Check Matrix](check-matrix.md) when the changed paths are known but no playbook
matches.
