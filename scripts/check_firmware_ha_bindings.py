#!/usr/bin/env python3
"""Guard firmware Home Assistant access behind button_grid_ha.h helpers."""

from __future__ import annotations

import argparse
import re
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_DIR = ROOT / "components" / "espcontrol"
CORE_INFRA_PATH = ROOT / "common" / "device" / "core_infra.yaml"
S3_DEVICE_PATH = ROOT / "devices" / "guition-esp32-s3-4848s040" / "device" / "device.yaml"
HA_BOUNDARY_ALLOWLIST = {
    "button_grid_ha.h",
}
DIRECT_HA_PATTERNS = (
    (re.compile(r"\bglobal_api_server\b"), "access Home Assistant API through button_grid_ha.h helpers"),
    (re.compile(r"->send_homeassistant_action\s*\("), "send Home Assistant actions through button_grid_ha.h helpers"),
    (re.compile(r"->subscribe_home_assistant_state\s*\("), "subscribe to Home Assistant state through button_grid_ha.h helpers"),
    (re.compile(r"->register_action_response_callback\s*\("), "register action callbacks through button_grid_ha.h helpers"),
)
STATE_HELPER_PATTERN = re.compile(
    r"inline\s+bool\s+ha_subscribe_state\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
ATTRIBUTE_HELPER_PATTERN = re.compile(
    r"inline\s+bool\s+ha_subscribe_attribute\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
TODO_GET_ITEMS_HELPER_PATTERN = re.compile(
    r"inline\s+bool\s+todo_begin_get_items_request\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
WEATHER_FORECAST_REQUEST_PATTERN = re.compile(
    r"inline\s+void\s+request_weather_forecast_entity\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
COVER_COMMAND_REQUEST_PATTERN = re.compile(
    r"inline\s+void\s+send_cover_command_action\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)


def firmware_ha_binding_errors(firmware_dir: Path, root: Path) -> list[str]:
    errors: list[str] = []
    for path in sorted(firmware_dir.glob("button_grid*.h")):
        if path.name in HA_BOUNDARY_ALLOWLIST:
            continue
        for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            for pattern, message in DIRECT_HA_PATTERNS:
                if pattern.search(line):
                    rel = path.relative_to(root)
                    errors.append(f"{rel}:{line_no}: {message}")
                    break
    return errors


def firmware_ha_boundary_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_ha.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    state_helper = STATE_HELPER_PATTERN.search(text)
    if not state_helper:
        errors.append(f"{rel}: missing ha_subscribe_state helper")
    elif "heap_available" in state_helper.group("body"):
        errors.append(f"{rel}: keep core HA state subscriptions off the low-heap guard")

    attribute_helper = ATTRIBUTE_HELPER_PATTERN.search(text)
    if not attribute_helper:
        errors.append(f"{rel}: missing ha_subscribe_attribute helper")
    elif "heap_available" in attribute_helper.group("body"):
        errors.append(f"{rel}: keep HA metadata attribute subscriptions off the low-heap guard")

    if "ha_cancel_action_response_callback" not in text or "handle_action_response" not in text:
        errors.append(f"{rel}: expose a helper to cancel stale HA action response callbacks")
    action_send_match = re.search(
        r"inline\s+bool\s+ha_action_send\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        re.DOTALL,
    )
    if not action_send_match:
        errors.append(f"{rel}: missing ha_action_send helper")
    elif "ha_api_state_connected()" not in action_send_match.group("body"):
        errors.append(f"{rel}: send Home Assistant actions only after state subscription is ready")

    return errors


def firmware_todo_request_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_todo.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    helper = TODO_GET_ITEMS_HELPER_PATTERN.search(text)
    if not helper:
        errors.append(f"{rel}: missing todo_begin_get_items_request helper")
        return errors

    body = helper.group("body")
    if '"todo.get_items"' not in body:
        errors.append(f"{rel}: todo_begin_get_items_request must call todo.get_items")
    if "wants_response" not in body or "response_template" not in body:
        errors.append(f"{rel}: todo.get_items requests must capture a compact response template")
    if "std::string response_template" in body:
        errors.append(f"{rel}: keep the todo response template alive until after the request is sent")
    if "TODO_RESPONSE_KEY_MAX_LEN" not in text or "TODO_RESPONSE_SUMMARY_MAX_LEN" not in text:
        errors.append(f"{rel}: bound todo response text before Home Assistant sends it")
    if "std::to_string(TODO_RESPONSE_TEXT_MAX_LEN)" not in text or "|length" not in text:
        errors.append(f"{rel}: cap rendered todo responses before Home Assistant sends them")
    if 'ha_action_add_data(req, "status"' in body:
        errors.append(f"{rel}: filter todo items in the response template, not in action data")
    if "TODO_REQUEST_TIMEOUT_MS" not in text or text.count("todo_cancel_stale_request()") < 2:
        errors.append(f"{rel}: bound pending todo item requests with a timeout")
    if "stale_request_cancelled = todo_cancel_stale_request()" not in text:
        errors.append(f"{rel}: periodically expire stale todo requests while the modal is open")
    if 'todo_cancel_pending_request("modal closed")' not in text:
        errors.append(f"{rel}: cancel pending todo item requests when the modal closes")
    if text.count("todo_clear_request_state(call_id)") < 2:
        errors.append(f"{rel}: clear pending todo request state when responses arrive")
    if "ha_api_state_connected()" not in text:
        errors.append(f"{rel}: wait for Home Assistant state subscription before todo actions")
    if text.count("ha_register_action_response_callback(") > 1:
        errors.append(f"{rel}: only todo list loading should register a response callback")
    return errors


def firmware_todo_disconnect_errors(firmware_dir: Path, core_infra_path: Path, root: Path) -> list[str]:
    todo_path = firmware_dir / "button_grid_todo.h"
    if not todo_path.exists() or not core_infra_path.exists():
        return []
    todo_rel = todo_path.relative_to(root)
    core_rel = core_infra_path.relative_to(root)
    todo_text = todo_path.read_text(encoding="utf-8")
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "todo_cancel_pending_request" not in todo_text:
        errors.append(f"{todo_rel}: expose a helper to cancel pending todo requests")
    if "todo_reload_active_modal" not in todo_text:
        errors.append(f"{todo_rel}: expose a helper to reload an open todo modal after HA reconnects")
    if "waiting_for_ha" not in todo_text or "todo_retry_waiting_modal" not in todo_text:
        errors.append(f"{todo_rel}: retry open todo modals that are waiting for Home Assistant")
    if "ctx->available) return" in todo_text:
        errors.append(f"{todo_rel}: allow todo modals to open while waiting for Home Assistant availability")
    if "apply_control_availability(ctx->btn, ctx->btn, ctx->available, false)" not in todo_text:
        errors.append(f"{todo_rel}: keep todo cards tappable while Home Assistant availability is pending")
    if "on_client_disconnected:" not in core_text or "todo_cancel_pending_request" not in core_text:
        errors.append(f"{core_rel}: cancel pending todo requests when the HA API disconnects")
    if "on_client_connected:" not in core_text or "todo_reload_active_modal" not in core_text:
        errors.append(f"{core_rel}: retry open todo modals when the HA API reconnects")
    if "todo_retry_waiting_modal" not in core_text:
        errors.append(f"{core_rel}: periodically retry todo modals waiting for Home Assistant")
    return errors


def firmware_weather_request_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_config.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    request = WEATHER_FORECAST_REQUEST_PATTERN.search(text)
    if not request:
        errors.append(f"{rel}: missing request_weather_forecast_entity helper")
        return errors
    body = request.group("body")
    if "ha_api_state_connected()" not in body:
        errors.append(f"{rel}: wait for Home Assistant state subscription before automatic forecast requests")
    if "ha_cancel_action_response_callback(req.call_id" not in text:
        errors.append(f"{rel}: cancel forecast response callbacks when sends fail")
    if "WEATHER_FORECAST_PENDING_MAX" not in text or "weather_forecast_track_pending" not in text:
        errors.append(f"{rel}: bound pending forecast response callbacks")
    if "weather_forecast_cancel_pending_requests" not in text:
        errors.append(f"{rel}: expose a helper to cancel pending forecast callbacks")
    return errors


def firmware_weather_disconnect_errors(firmware_dir: Path, core_infra_path: Path, root: Path) -> list[str]:
    config_path = firmware_dir / "button_grid_config.h"
    if not config_path.exists() or not core_infra_path.exists():
        return []
    core_rel = core_infra_path.relative_to(root)
    config_text = config_path.read_text(encoding="utf-8")
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "weather_forecast_cancel_pending_requests" in config_text and (
        "on_client_disconnected:" not in core_text or "weather_forecast_cancel_pending_requests" not in core_text
    ):
        errors.append(f"{core_rel}: cancel pending forecast callbacks when the HA API disconnects")
    return errors


def firmware_cover_request_errors(firmware_dir: Path, core_infra_path: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_actions.h"
    if not path.exists() or not core_infra_path.exists():
        return []
    rel = path.relative_to(root)
    core_rel = core_infra_path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    request = COVER_COMMAND_REQUEST_PATTERN.search(text)
    if not request:
        errors.append(f"{rel}: missing send_cover_command_action helper")
        return errors
    body = request.group("body")
    if "cover_stop_track_pending" not in text or "cover_stop_clear_pending" not in text:
        errors.append(f"{rel}: track pending cover stop callbacks")
    if "cover_stop_cancel_pending_request" not in text:
        errors.append(f"{rel}: expose a helper to cancel pending cover stop callbacks")
    if "cover_stop_tracked" not in body or "ha_cancel_action_response_callback(req.call_id" not in body:
        errors.append(f"{rel}: cancel cover stop callbacks when sends fail")
    if "on_client_disconnected:" not in core_text or "cover_stop_cancel_pending_request" not in core_text:
        errors.append(f"{core_rel}: cancel pending cover stop callbacks when the HA API disconnects")
    return errors


def firmware_s3_api_errors(device_path: Path, root: Path) -> list[str]:
    if not device_path.exists():
        return []
    rel = device_path.relative_to(root)
    text = device_path.read_text(encoding="utf-8")
    errors: list[str] = []

    queue_match = re.search(r"(?m)^\s*max_send_queue:\s*(\d+)\s*$", text)
    if not queue_match:
        errors.append(f"{rel}: set an explicit S3 native API send queue")
    elif int(queue_match.group(1)) < 12:
        errors.append(f"{rel}: keep the S3 native API send queue high enough for HA reconnect bursts")
    if "max_connections: 2" not in text:
        errors.append(f"{rel}: keep the S3 native API connection pool small")
    return errors


def run_scan() -> int:
    errors = firmware_ha_binding_errors(FIRMWARE_DIR, ROOT)
    errors.extend(firmware_ha_boundary_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_todo_request_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_todo_disconnect_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_weather_request_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_weather_disconnect_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_cover_request_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_s3_api_errors(S3_DEVICE_PATH, ROOT))
    if errors:
        print("Firmware Home Assistant binding check failed:")
        for error in errors:
            print(f"  {error}")
        return 1
    print("Firmware Home Assistant binding checks passed.")
    return 0


def expect_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        for filename, text in files.items():
            (firmware_dir / filename).write_text(text, encoding="utf-8")

        errors = firmware_ha_binding_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_ha_boundary_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        for filename, text in files.items():
            (firmware_dir / filename).write_text(text, encoding="utf-8")

        errors = firmware_ha_boundary_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_todo_request_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(text, encoding="utf-8")

        errors = firmware_todo_request_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_todo_disconnect_errors(
    name: str,
    todo_text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(todo_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_todo_disconnect_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_weather_request_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_config.h").write_text(text, encoding="utf-8")

        errors = firmware_weather_request_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_weather_disconnect_errors(
    name: str,
    config_text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_config.h").write_text(config_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_weather_disconnect_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_request_errors(
    name: str,
    action_text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_actions.h").write_text(action_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_cover_request_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_s3_api_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        device_path = root / "devices" / "guition-esp32-s3-4848s040" / "device" / "device.yaml"
        device_path.parent.mkdir(parents=True)
        device_path.write_text(text, encoding="utf-8")

        errors = firmware_s3_api_errors(device_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def run_self_test() -> int:
    expect_errors(
        "direct action send",
        {"button_grid_actions.h": "esphome::api::global_api_server->send_homeassistant_action(req);\n"},
        ("access Home Assistant API through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct state subscription",
        {"button_grid_media.h": "api->subscribe_home_assistant_state(entity, {}, cb);\n"},
        ("subscribe to Home Assistant state through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct callback registration",
        {"button_grid_alarm.h": "api->register_action_response_callback(id, cb);\n"},
        ("register action callbacks through button_grid_ha.h helpers",),
    )
    expect_errors(
        "helper boundary",
        {"button_grid_ha.h": "esphome::api::global_api_server->send_homeassistant_action(req);\n"},
        (),
    )
    expect_errors(
        "helper use",
        {"button_grid_media.h": "ha_subscribe_state(entity, cb);\n"},
        (),
    )
    expect_ha_boundary_errors(
        "missing callback cancel helper",
        {
            "button_grid_ha.h": (
                "inline bool ha_subscribe_state() {\n  return true;\n}\n"
                "inline bool ha_subscribe_attribute() {\n  return true;\n}\n"
                "inline bool ha_action_send() {\n  return ha_api_state_connected();\n}\n"
            )
        },
        ("expose a helper to cancel stale HA action response callbacks",),
    )
    expect_ha_boundary_errors(
        "action send only checks socket",
        {
            "button_grid_ha.h": (
                "inline bool ha_subscribe_state() {\n  return true;\n}\n"
                "inline bool ha_subscribe_attribute() {\n  return true;\n}\n"
                "inline bool ha_cancel_action_response_callback() {\n  handle_action_response(); return true;\n}\n"
                "inline bool ha_action_send() {\n  return ha_api_connected();\n}\n"
            )
        },
        ("send Home Assistant actions only after state subscription is ready",),
    )
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("must capture a compact response template" in error for error in errors), errors
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 2, call_id);\n'
            '  req.wants_response = true;\n'
            '  req.response_template = response_template;\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  ha_action_add_data(req, "status", "needs_action");\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("filter todo items in the response template" in error for error in errors), errors
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
            '  req.wants_response = true;\n'
            '  std::string response_template = todo_items_response_template(ctx->entity_id);\n'
            '  req.response_template = response_template;\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("keep the todo response template alive" in error for error in errors), errors
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
            '  req.wants_response = true;\n'
            '  req.response_template = response_template;\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("bound todo response text" in error for error in errors), errors
    expect_todo_request_errors(
        "unbounded rendered todo response",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_RESPONSE_TEXT_MAX_LEN = 1536;\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n',
        ("cap rendered todo responses",),
    )
    expect_todo_request_errors(
        "unbounded pending todo request",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  if (!ha_register_action_response_callback(req.call_id, cb)) return;\n'
        '}\n',
        ("bound pending todo item requests with a timeout",),
    )
    expect_todo_request_errors(
        "extra todo response callback",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline void todo_cancel_stale_request() {}\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '  ha_register_action_response_callback(other_call_id, cb);\n'
        '}\n',
        ("only todo list loading should register a response callback",),
    )
    expect_todo_request_errors(
        "timeout only checked while requesting",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline bool todo_cancel_stale_request() { return false; }\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '}\n',
        ("periodically expire stale todo requests",),
    )
    expect_todo_request_errors(
        "modal close leaves todo request pending",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline bool todo_cancel_stale_request() { return false; }\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void todo_modal_hide() {\n'
        '  ui = TodoModalUi();\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  bool stale_request_cancelled = todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '}\n',
        ("cancel pending todo item requests when the modal closes",),
    )
    expect_todo_disconnect_errors(
        "missing disconnect cleanup",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("cancel pending todo requests when the HA API disconnects",),
    )
    expect_todo_disconnect_errors(
        "missing reconnect retry",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n",
        "api:\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("retry open todo modals when the HA API reconnects",),
    )
    expect_todo_disconnect_errors(
        "missing waiting modal retry",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n",
        ("retry open todo modals that are waiting for Home Assistant",),
    )
    expect_todo_disconnect_errors(
        "availability blocks todo modal",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n"
        "inline void todo_card_open_modal(TodoCardCtx *ctx) {\n"
        "  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty() || !ctx->available) return;\n"
        "}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("allow todo modals to open while waiting",),
    )
    expect_todo_disconnect_errors(
        "availability disables todo tap",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n"
        "inline void todo_card_open_modal(TodoCardCtx *ctx) {\n"
        "  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty()) return;\n"
        "}\n"
        "inline void subscribe_todo_state(TodoCardCtx *ctx) {\n"
        "  apply_control_availability(ctx->btn, ctx->btn, ctx->available);\n"
        "}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("keep todo cards tappable",),
    )
    expect_weather_request_errors(
        "weather request during reconnect",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  if (!ha_api_available()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  ha_action_send(req);\n"
        "}\n",
        ("wait for Home Assistant state subscription",),
    )
    expect_weather_request_errors(
        "weather callback leak on send failure",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  if (!ha_action_send(req)) return;\n"
        "}\n",
        ("cancel forecast response callbacks",),
    )
    expect_weather_request_errors(
        "unbounded weather callbacks",
        "inline void request_weather_forecast_entity() {\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "}\n",
        ("bound pending forecast response callbacks",),
    )
    expect_weather_disconnect_errors(
        "missing weather disconnect cleanup",
        "inline void weather_forecast_cancel_pending_requests() {}\n",
        "api:\n  on_client_connected:\n    - lambda: refresh_weather_forecast_cards();\n",
        ("cancel pending forecast callbacks when the HA API disconnects",),
    )
    expect_cover_request_errors(
        "missing cover callback tracking",
        "inline void cover_stop_cancel_pending_request() {}\n"
        "inline void send_cover_command_action() {\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "}\n",
        "api:\n"
        "  on_client_disconnected:\n"
        "    - lambda: cover_stop_cancel_pending_request();\n",
        ("track pending cover stop callbacks",),
    )
    expect_cover_request_errors(
        "missing cover disconnect cleanup",
        "inline void cover_stop_track_pending() {}\n"
        "inline void cover_stop_clear_pending() {}\n"
        "inline void cover_stop_cancel_pending_request() {}\n"
        "inline void send_cover_command_action() {\n"
        "  bool cover_stop_tracked = true;\n"
        "  ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "}\n",
        "api:\n  on_client_connected:\n    - lambda: refresh_weather_forecast_cards();\n",
        ("cancel pending cover stop callbacks when the HA API disconnects",),
    )
    expect_s3_api_errors(
        "low S3 API queue",
        "api:\n  max_connections: 2\n  max_send_queue: 8\n",
        ("keep the S3 native API send queue high enough",),
    )
    print("Firmware Home Assistant binding self-tests passed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run guardrail self-tests")
    args = parser.parse_args()
    return run_self_test() if args.self_test else run_scan()


if __name__ == "__main__":
    raise SystemExit(main())
