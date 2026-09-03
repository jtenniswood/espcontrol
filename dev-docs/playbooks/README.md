# Task Playbooks

Playbooks own operational guidance: what to edit, what to regenerate, which
checks to run, and when to stop. Use the [Task Router](../task-router.md) when
the request does not already identify a workflow.

## Available Playbooks

- [Add or change a card type](add-card-type.md)
- [Add or change a supported device](add-supported-device.md)
- [Change fonts or icons](change-fonts-or-icons.md)
- [Change saved config](change-saved-config.md)
- [Change the web configurator](change-web-configurator.md)
- [Change firmware UI](change-firmware-ui.md)
- [Change the release workflow](change-release-workflow.md)

## Shared Rules

- Prefer authored sources over generated files.
- Commit generated files only when a listed generator produced them.
- Stop and inspect before keeping unrelated generated output.
- Ask before removing saved-config compatibility, changing public device
  support, or adding a new firmware font role.
- Do not edit generated files or generated sections directly. Use
  [Source of Truth Contract](../source-of-truth.md) to find the source file and
  generator.
