#!/usr/bin/env python3
"""Validate lightweight pull request process requirements."""

from __future__ import annotations

import argparse
import json
import os
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / ".github" / "pull_request_template.md"
DOCS_HEADING = "Documentation decision"
TESTING_HEADING = "Testing"
STATUS_HEADING = "PR status"
ISSUE_HEADING = "Issue handling"
ISSUE_CONFIRMATION = "Do not close related issues until the user confirms the fix works"
REQUIRED_BODY_HEADINGS = (DOCS_HEADING, TESTING_HEADING, STATUS_HEADING, ISSUE_HEADING)
TEMPLATE_CHECKLIST_ITEMS = (
    "Updated public docs or release-facing notes",
    "No docs needed because this does not change user-visible behavior/configuration",
    "Docs follow-up needed before merge",
    "Automated/local checks passed or were run where practical",
    "Device testing is required before merge",
    "Device testing is not required",
    "Ready to merge after user confirmation",
    ISSUE_CONFIRMATION,
)


def section_text(text: str, heading: str) -> str:
    match = re.search(rf"^## +{re.escape(heading)}\s*$", text, re.MULTILINE)
    if not match:
        raise AssertionError(f"Missing '## {heading}' section.")

    start = match.end()
    next_heading = re.search(r"^## +", text[start:], re.MULTILINE)
    end = start + next_heading.start() if next_heading else len(text)
    return text[start:end]


def checked_items(section: str) -> list[str]:
    return re.findall(r"^\s*- \[[xX]\] +(.+)$", section, re.MULTILINE)


def checklist_items(text: str) -> list[str]:
    return re.findall(r"^\s*- \[[ xX]\] +(.+)$", text, re.MULTILINE)


def has_heading(text: str, heading: str) -> bool:
    return bool(re.search(rf"^## +{re.escape(heading)}\s*$", text, re.MULTILINE))


def uses_pr_template(body: str) -> bool:
    heading_count = sum(has_heading(body, heading) for heading in REQUIRED_BODY_HEADINGS)
    if heading_count == len(REQUIRED_BODY_HEADINGS):
        return True

    if heading_count >= 2:
        return True

    template_checklist_count = sum(
        template_item in item
        for item in checklist_items(body)
        for template_item in TEMPLATE_CHECKLIST_ITEMS
    )
    return template_checklist_count >= 2


def docs_notes(section: str) -> str:
    match = re.search(r"^Docs notes:\s*$", section, re.MULTILINE)
    if not match:
        return ""

    notes = section[match.end() :].strip()
    notes = re.sub(r"^\s*-\s*$", "", notes, flags=re.MULTILINE).strip()
    return notes


def notes_body(section: str) -> str:
    notes = section.strip()
    notes = re.sub(r"^\s*-\s*$", "", notes, flags=re.MULTILINE).strip()
    return notes


def assert_template_ready() -> None:
    text = TEMPLATE.read_text(encoding="utf-8")
    section = section_text(text, DOCS_HEADING)
    items = re.findall(r"^\s*- \[[ xX]\] +(.+)$", section, re.MULTILINE)
    assert len(items) >= 3, "PR template must offer clear documentation choices."
    assert "Docs notes:" in section, "PR template must include docs notes."

    testing = section_text(text, TESTING_HEADING)
    testing_items = re.findall(r"^\s*- \[[ xX]\] +(.+)$", testing, re.MULTILINE)
    assert any("Device testing is required" in item for item in testing_items), (
        "PR template must include a device-testing-required choice."
    )
    assert any("Device testing is not required" in item for item in testing_items), (
        "PR template must include a device-testing-not-required choice."
    )
    assert "Affected display/device" in testing, "PR template must ask for affected devices."

    status = section_text(text, STATUS_HEADING)
    status_items = re.findall(r"^\s*- \[[ xX]\] +(.+)$", status, re.MULTILINE)
    assert len(status_items) >= 5, "PR template must include a clear PR status checklist."

    issue = section_text(text, ISSUE_HEADING)
    issue_items = re.findall(r"^\s*- \[[ xX]\] +(.+)$", issue, re.MULTILINE)
    assert any(ISSUE_CONFIRMATION in item for item in issue_items), (
        "PR template must include the related-issue confirmation."
    )


def assert_pr_body_ready(body: str) -> None:
    assert body.strip(), "Add a PR description that explains the purpose and testing notes."

    if not uses_pr_template(body):
        print("PR body does not use the checklist template; skipping template field validation.")
        return

    section = section_text(body, DOCS_HEADING)
    selected = checked_items(section)
    assert selected, (
        "Choose one item in the Documentation decision section so reviewers know "
        "whether docs changed or why they were not needed."
    )
    assert len(selected) == 1, "Choose exactly one Documentation decision item."

    selected_text = selected[0].lower()
    if "no docs needed" in selected_text or "follow-up" in selected_text:
        assert docs_notes(section), (
            "Add a short Docs notes explanation when docs were not updated or are "
            "planned as a follow-up."
        )

    testing = section_text(body, TESTING_HEADING)
    testing_choices = [
        item
        for item in checked_items(testing)
        if "Device testing is required" in item or "Device testing is not required" in item
    ]
    assert testing_choices, (
        "Choose whether device testing is required so the user knows whether they "
        "need to physically test hardware before merge."
    )
    assert len(testing_choices) == 1, "Choose exactly one device testing requirement."

    if "device testing is required" in testing_choices[0].lower():
        assert "Affected display/device" in testing, "PR body is missing the affected device prompt."
        after_prompt = testing.split("Affected display/device", 1)[1]
        assert re.search(r"^\s*-\s+\S+", after_prompt, re.MULTILINE), (
            "Name the affected display/device when device testing is required."
        )
    else:
        testing_notes = section_text(body, "Notes for testing")
        assert notes_body(testing_notes), (
            "Explain why physical device testing is not required in Notes for testing."
        )

    status = section_text(body, STATUS_HEADING)
    status_selected = checked_items(status)
    assert status_selected, "Choose one PR status item so the next action is obvious."
    assert len(status_selected) == 1, "Choose exactly one PR status item."

    issue = section_text(body, ISSUE_HEADING)
    issue_selected = checked_items(issue)
    assert any(ISSUE_CONFIRMATION in item for item in issue_selected), (
        "Confirm related issues will stay open until the user confirms the fix works."
    )


def pr_body_from_event(event_path: str | None) -> str | None:
    if not event_path:
        return None

    event = json.loads(Path(event_path).read_text(encoding="utf-8"))
    pull_request = event.get("pull_request")
    if not isinstance(pull_request, dict):
        return None

    return pull_request.get("body") or ""


def run_self_test() -> None:
    valid = """## Documentation decision

- [x] Updated public docs or release-facing notes for user-visible behavior/configuration.
- [ ] No docs needed because this does not change user-visible behavior/configuration.
- [ ] Docs follow-up needed before merge; explain below.

Docs notes:

- Updated docs/features/example.md.

## Testing

- [x] Automated/local checks passed or were run where practical.
- [x] Device testing is required before merge.
- [ ] Device testing is not required; explain why in Notes for testing.

Affected display/device, if applicable:

- Guition JC1060P470

## Notes for testing

- Flash the 7-inch display and confirm the home screen renders.

## PR status

- [ ] Checks running or waiting.
- [ ] Needs automated fix.
- [x] Ready for device test.
- [ ] Device test failed; details below.
- [ ] Device tested successfully.
- [ ] Ready to merge after user confirmation.

## Issue handling

- [x] Do not close related issues until the user confirms the fix works.
"""
    assert_pr_body_ready(valid)

    no_docs = valid.replace("- [x] Updated", "- [ ] Updated")
    no_docs = no_docs.replace("- [ ] No docs needed", "- [x] No docs needed")
    no_docs = no_docs.replace("- Updated docs/features/example.md.", "- Internal script-only change.")
    assert_pr_body_ready(no_docs)

    missing_choice = valid.replace("- [x] Updated", "- [ ] Updated")
    try:
        assert_pr_body_ready(missing_choice)
    except AssertionError:
        pass
    else:
        raise AssertionError("self-test expected missing documentation choice to fail")

    missing_reason = no_docs.replace("- Internal script-only change.", "-")
    try:
        assert_pr_body_ready(missing_reason)
    except AssertionError:
        pass
    else:
        raise AssertionError("self-test expected missing docs reason to fail")

    no_device_test = valid.replace("- [x] Device testing is required", "- [ ] Device testing is required")
    no_device_test = no_device_test.replace("- [ ] Device testing is not required", "- [x] Device testing is not required")
    no_device_test = no_device_test.replace(
        "- Flash the 7-inch display and confirm the home screen renders.",
        "- Workflow-only change.",
    )
    assert_pr_body_ready(no_device_test)

    missing_device_choice = valid.replace("- [x] Device testing is required", "- [ ] Device testing is required")
    try:
        assert_pr_body_ready(missing_device_choice)
    except AssertionError:
        pass
    else:
        raise AssertionError("self-test expected missing device testing choice to fail")

    multiple_status = valid.replace("- [ ] Checks running or waiting.", "- [x] Checks running or waiting.")
    try:
        assert_pr_body_ready(multiple_status)
    except AssertionError:
        pass
    else:
        raise AssertionError("self-test expected multiple PR statuses to fail")

    freeform = """## Purpose

Explain the change in plain language.

## Testing

- python3 scripts/build.py --check
"""
    assert_pr_body_ready(freeform)

    freeform_with_template_words = """## Purpose

Device testing is not required because this only changes docs.

## Testing

- Reviewed the wording locally.
"""
    assert_pr_body_ready(freeform_with_template_words)

    freeform_with_template_checkbox = """## Purpose

Documentation-only wording change.

## Testing

- [x] Device testing is not required because this only changes docs.
"""
    assert_pr_body_ready(freeform_with_template_checkbox)

    partial_template = valid.replace("## PR status", "## Next steps")
    try:
        assert_pr_body_ready(partial_template)
    except AssertionError as error:
        assert "Missing '## PR status' section." in str(error)
    else:
        raise AssertionError("self-test expected partial checklist template to fail")

    empty_freeform = """

"""
    try:
        assert_pr_body_ready(empty_freeform)
    except AssertionError:
        pass
    else:
        raise AssertionError("self-test expected empty PR body to fail")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--event-path", default=os.environ.get("GITHUB_EVENT_PATH"))
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    assert_template_ready()

    if args.self_test:
        run_self_test()
        print("PR process self-test passed.")
        return 0

    body = pr_body_from_event(args.event_path)
    if body is None:
        print("PR process check skipped outside pull_request events.")
        return 0

    try:
        assert_pr_body_ready(body)
    except AssertionError as error:
        print(f"::error::{error}")
        return 1

    print("PR process check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
