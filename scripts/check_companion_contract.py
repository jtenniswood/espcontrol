#!/usr/bin/env python3
"""Check Companion's authored contract, generated outputs, and security boundary."""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "product/v2/companion_capabilities.json"
MANIFEST = ROOT / "product/generated/companion_manifest.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"Companion contract check failed: {message}")


def main() -> int:
    contract = json.loads(CONTRACT.read_text())
    protocol = contract["protocol"]
    security = contract["security"]
    messages = protocol["messages"]
    message_ids = [message["id"] for message in messages]
    mode_ids = [mode["id"] for mode in contract["cardModes"]]

    require(protocol["version"] == 3, "the reset transport must remain protocol v3")
    require(protocol["path"] == "/companion/v3", "the v3 endpoint changed unexpectedly")
    require(len(message_ids) == len(set(message_ids)), "protocol message IDs are not unique")
    require(len(mode_ids) == len(set(mode_ids)), "card mode IDs are not unique")
    require(security["pairingAuthorization"] == "physical_presence", "pairing must require physical access")
    require(not security["browserExposesPairingCode"], "the browser must not expose pairing codes")

    required_messages = {
        "hello", "pair.request", "pair.accepted", "auth.request", "auth.accepted",
        "capabilities", "catalogue.request", "catalogue.page", "action.invoke",
        "action.result", "value.set", "value.state", "focus.changed",
        "timezone.changed", "now_playing", "system_metrics", "artwork.begin",
        "artwork.ack", "artwork.end", "artwork.abort", "artwork.request", "error",
    }
    require(set(message_ids) == required_messages, "the typed message registry is incomplete")

    manifest = json.loads(MANIFEST.read_text())
    require(manifest["source"] == str(CONTRACT.relative_to(ROOT)), "generated manifest source is wrong")
    require(manifest["generator"] == "python3 scripts/build.py companion", "generated command is wrong")
    outputs = [ROOT / path for path in manifest["outputs"]]
    require(len(outputs) == 3 and all(path.is_file() for path in outputs), "generated outputs are missing")

    for output in outputs:
        text = output.read_text()
        require(protocol["path"] in text, f"{output.relative_to(ROOT)} omits the protocol path")
        for message_id in message_ids:
            require(message_id in text, f"{output.relative_to(ROOT)} omits {message_id}")
        for mode_id in mode_ids:
            require(mode_id in text, f"{output.relative_to(ROOT)} omits card mode {mode_id}")

    transport_sources = [
        ROOT / "components/companion/companion.cpp",
        ROOT / "macos/Companion/Sources/Companion/CompanionConnection.swift",
    ]
    legacy_tokens = ("PAIR|", "AUTH|", "CAPS|", "ACTIONS|", "INVOKE|", "RESULT|",
                     "VALUE|", "FOCUS|", "TZ|", "METRICS|", "NOWPLAYING|")
    for source in transport_sources:
        text = source.read_text()
        require(not any(token in text for token in legacy_tokens),
                f"{source.relative_to(ROOT)} contains a legacy delimiter message")

    firmware = transport_sources[0].read_text()
    browser = (ROOT / "src/webserver/application/settings_companion_section.ts").read_text()
    require("register_companion_pairing_callbacks" not in firmware,
            "firmware still exposes remote pairing activation")
    require("Pairing code:" not in browser and "copyButton" not in browser,
            "browser UI still renders pairing credentials")

    print("Companion contract, generated outputs, and physical pairing boundary passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
