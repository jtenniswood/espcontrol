# EspControl Developer Reference

Internal reference for contributors and maintainers. End-user installation and
usage documentation lives in the root `README.md` and under `docs/`.

## Getting Started

- [Development Environment](development-environment.md) - install and verify the
  complete local toolchain.
- [Working Tree Rules](working-tree-rules.md) - protect unrelated work and keep
  changes focused.

## Workflows

- [Task Router](task-router.md) - choose the correct workflow for a change.
- [Task Playbooks](playbooks/README.md) - exact edit, generation, stop, and
  verification steps.
- [Check Matrix](check-matrix.md) - choose checks from the paths changed.
- [Failure Cookbook](failure-cookbook.md) - diagnose common failure patterns.
- [Checks and Releases](checks-and-releases.md) - understand the check graph,
  release boundary, and confidence model.

## Architecture

- [Architecture](architecture.md) - how the product surfaces fit together.
- [Source of Truth Contract](source-of-truth.md) - authored and generated file
  ownership.
- [Compatibility Contract](compatibility-contract.md) - upgrade-sensitive
  formats and behavior.
- [Architecture Decision Records](adr/README.md) - accepted decisions and their
  historical context.

## Subsystems

- [Card Contract](card-contract.md)
- [Card Type Map](card-type-map.md)
- [Saved-Configuration Normalization](saved-config-normalization.md)
- [Web Configurator](web-configurator.md)
- [Firmware UI](firmware.md)
- [Devices and Builds](devices-and-builds.md)
- [Modal Layout System](modal-layout-system.md)
- [Display Lifecycle Transition Contract](display-lifecycle.md)
- [Cover Art Mode](cover-art-mode.md)
- [Font Guidelines](font-guidelines.md)

## Historical Records

- [Historical Records Index](history/README.md) - dated investigations and
  measurement baselines that are not current operational guidance.
