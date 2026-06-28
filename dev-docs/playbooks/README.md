# Task Playbooks

Use these playbooks when a request matches a common repo task. They are more
prescriptive than [Change Workflows](../change-workflows.md): start with the
listed source files, regenerate only when the playbook says to, and stop when
unexpected files change.

## Available Playbooks

- [Add or change a card type](add-card-type.md)
- [Add or change a supported device](add-supported-device.md)
- [Change fonts or icons](change-fonts-or-icons.md)
- [Change saved config](change-saved-config.md)

## Shared Rules

- Prefer source files over generated files.
- Commit generated files only when a listed generator produced them.
- Stop and inspect before keeping unrelated generated output.
- Ask before removing saved-config compatibility, changing public device
  support, or adding a new firmware font role.
- Do not edit generated files or generated sections directly. Use
  [Source of Truth Contract](../source-of-truth.md) to find the source file and
  generator.
