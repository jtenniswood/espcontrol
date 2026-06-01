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
API_NAVIGATE_PATH = ROOT / "common" / "device" / "api_navigate.yaml"
TIME_ADDON_PATH = ROOT / "common" / "addon" / "time.yaml"
S3_DEVICE_PATH = ROOT / "devices" / "guition-esp32-s3-4848s040" / "device" / "device.yaml"
S3_PACKAGES_PATH = ROOT / "devices" / "guition-esp32-s3-4848s040" / "packages.yaml"
DEVICE_DEVICE_PATHS = tuple(sorted((ROOT / "devices").glob("*/device/device.yaml")))
DEVICE_PACKAGE_PATHS = tuple(sorted((ROOT / "devices").glob("*/packages.yaml")))
CONNECTIVITY_PATHS = (
    ROOT / "common" / "addon" / "connectivity.yaml",
    ROOT / "common" / "addon" / "connectivity_deployed.yaml",
    ROOT / "common" / "addon" / "connectivity_ethernet.yaml",
)
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
    if 'todo_cancel_pending_request("modal closed"' not in text:
        errors.append(f"{rel}: cancel pending todo item requests when the modal closes")
    if 'todo_cancel_pending_request("modal closed", false)' not in text:
        errors.append(f"{rel}: close todo modals without retrying their cancelled request")
    if '"send failed"' in text and 'ui.waiting_for_ha = true;' not in text:
        errors.append(f"{rel}: retry todo loads when Home Assistant disconnects during send")
    pending_match = re.search(
        r"if\s*\(\s*todo_request_state\(\)\.call_id\s*!=\s*0\s*\)\s*\{(?P<body>.*?)\n\s*\}",
        text,
        re.DOTALL,
    )
    if not pending_match or "ui.waiting_for_ha = true;" not in pending_match.group("body"):
        errors.append(f"{rel}: retry todo loads when another todo request is already pending")
    if text.count("todo_clear_request_state(call_id)") < 2:
        errors.append(f"{rel}: clear pending todo request state when responses arrive")
    if "ha_api_state_connected()" not in text:
        errors.append(f"{rel}: wait for Home Assistant state subscription before todo actions")
    callback_sections = [text]
    lite_marker = "#elif defined(ESPCONTROL_TODO_LITE) && ESPCONTROL_TODO_LITE"
    full_marker = "#else\n\nconstexpr uint32_t TODO_CARD_CTX_MAGIC"
    if lite_marker in text and full_marker in text:
        before_lite, lite_and_full = text.split(lite_marker, 1)
        lite, full = lite_and_full.split(full_marker, 1)
        callback_sections = [before_lite, lite, full]
    if any(section.count("ha_register_action_response_callback(") > 1 for section in callback_sections):
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


def firmware_time_reconnect_errors(time_path: Path, root: Path) -> list[str]:
    if not time_path.exists():
        return []
    rel = time_path.relative_to(root)
    text = time_path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "id(homeassistant_time).update();" in text.replace(
        "if (ha_api_state_connected()) id(homeassistant_time).update();", ""
    ):
        errors.append(f"{rel}: guard Home Assistant time updates until state subscription is ready")
    if "on_client_connected:" in text and "ha_api_state_connected()" not in text:
        errors.append(f"{rel}: wait for Home Assistant state readiness before reconnect time sync")
    if "on_client_connected:" in text and "delay: 2s" not in text:
        errors.append(f"{rel}: defer Home Assistant time sync after API reconnect")
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
    if (
        'ha_cancel_action_response_callback(req.call_id, "send failed");' in body
        and 'weather_forecast_schedule_retry(entity_id, day, "send failed");' not in body
    ):
        errors.append(f"{rel}: retry weather forecast sends that fail after callback registration")
    if "WEATHER_FORECAST_PENDING_MAX" not in text or "weather_forecast_track_pending" not in text:
        errors.append(f"{rel}: bound pending forecast response callbacks")
    if "weather_forecast_cancel_pending_requests" not in text:
        errors.append(f"{rel}: expose a helper to cancel pending forecast callbacks")
    if (
        "uint32_t generation = ha_subscription_generation();" not in body
        or "generation != ha_subscription_generation()" not in body
    ):
        errors.append(f"{rel}: ignore stale forecast action responses after dashboard reconfiguration")
    if "WEATHER_FORECAST_RETRY_DELAY_MS" not in text or "weather_forecast_schedule_retry" not in text:
        errors.append(f"{rel}: retry failed weather forecast requests later")
    if (
        "response if response is defined and response is not none else none" not in text
        or "'forecast' in response_data" not in text
        or "response_data[entity] if response_data is not none and entity in response_data else none" not in text
    ):
        errors.append(f"{rel}: accept both direct and entity-keyed Home Assistant forecast response shapes")
    if "entity_response['temperature_unit']" not in text or "state_attr(entity, 'temperature_unit')" not in text:
        errors.append(f"{rel}: preserve forecast temperature units from response data or weather entity attributes")
    if (
        "native_temperature" not in text
        or "native_templow" not in text
        or "native_temperature_unit" not in text
    ):
        errors.append(f"{rel}: accept native Home Assistant forecast temperature fields and units")
    if (
        "max_temperature" not in text
        or "temperature_max" not in text
        or "max_temp" not in text
        or "min_temperature" not in text
        or "temperature_min" not in text
        or "min_temp" not in text
    ):
        errors.append(f"{rel}: accept max/min weather forecast temperature field aliases")
    if "item_unit" not in text or "today['temperature_unit']" not in text or "tomorrow['native_temperature_unit']" not in text:
        errors.append(f"{rel}: preserve forecast temperature units from individual forecast items")
    if "parse_weather_forecast_temp" in text and "std::isfinite(parsed)" not in text:
        errors.append(f"{rel}: reject non-finite weather forecast temperatures before rendering")
    if (
        "No usable forecast temperatures" in body
        and 'weather_forecast_schedule_retry(entity_id, day, "no usable forecast temperatures");' not in body
    ):
        errors.append(f"{rel}: retry weather forecasts when Home Assistant returns no usable temperatures")
    if (
        "weather_forecast_error_is_timeout" not in text
        or 'find("timed out")' not in text
        or "apply_weather_forecast_actions_required_for_entity" not in body
    ):
        errors.append(f"{rel}: detect Home Assistant forecast timeout errors robustly")
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


def firmware_weather_reconnect_errors(core_infra_path: Path, root: Path) -> list[str]:
    if not core_infra_path.exists():
        return []
    core_rel = core_infra_path.relative_to(root)
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    connected_match = re.search(
        r"(?ms)^  on_client_connected:\n(?P<body>.*?)(?:^  on_client_disconnected:|^interval:|\Z)",
        core_text,
    )
    if not connected_match:
        errors.append(f"{core_rel}: refresh weather forecasts after Home Assistant reconnects")
        return errors

    body = connected_match.group("body")
    for match in re.finditer(r"refresh_weather_forecast_cards\(\);", body):
        guard_window = body[max(0, match.start() - 160) : match.end()]
        if "ha_api_state_connected()" not in guard_window:
            errors.append(f"{core_rel}: wait for Home Assistant state readiness before forecast reconnect refreshes")
            break
    if "refresh_weather_forecast_cards();" in body and "delay: 20s" not in body:
        errors.append(f"{core_rel}: retry weather forecast refresh after slow S3 reconnects")
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


def firmware_s3_api_errors(
    device_path: Path,
    s3_packages_path: Path,
    core_infra_path: Path,
    api_navigate_path: Path,
    package_paths: tuple[Path, ...],
    root: Path,
) -> list[str]:
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
    connections_match = re.search(r"(?m)^\s*max_connections:\s*(\d+)\s*$", text)
    if not connections_match:
        errors.append(f"{rel}: set an explicit S3 native API connection pool")
    elif int(connections_match.group(1)) < 3:
        errors.append(f"{rel}: keep enough S3 native API slots for HA reconnects after OTA")
    if "ESPCONTROL_DISABLE_TODO=1" not in text:
        errors.append(f"{rel}: keep the S3 todo list disabled until its HA action response path is stable")

    if api_navigate_path.exists():
        api_rel = api_navigate_path.relative_to(root)
        api_text = api_navigate_path.read_text(encoding="utf-8")
        if "action: navigate" not in api_text:
            errors.append(f"{api_rel}: keep the Home Assistant navigate action in the dedicated API package")
    else:
        errors.append(f"{api_navigate_path.relative_to(root)}: missing dedicated Home Assistant navigate API package")

    if core_infra_path.exists():
        core_rel = core_infra_path.relative_to(root)
        core_text = core_infra_path.read_text(encoding="utf-8")
        if "action: navigate" in core_text:
            errors.append(f"{core_rel}: keep the navigate action out of core_infra so S3 can omit it")

    if s3_packages_path.exists():
        s3_rel = s3_packages_path.relative_to(root)
        s3_packages = s3_packages_path.read_text(encoding="utf-8")
        if "api_navigate" in s3_packages or "api_navigate.yaml" in s3_packages:
            errors.append(f"{s3_rel}: omit the Home Assistant navigate API action on S3")

    for package_path in package_paths:
        if package_path == s3_packages_path or not package_path.exists():
            continue
        package_rel = package_path.relative_to(root)
        package_text = package_path.read_text(encoding="utf-8")
        if "api_navigate" not in package_text or "api_navigate.yaml" not in package_text:
            errors.append(f"{package_rel}: include the dedicated Home Assistant navigate API package")
    return errors


def firmware_todo_disabled_errors(device_paths: tuple[Path, ...], root: Path) -> list[str]:
    errors: list[str] = []
    for path in device_paths:
        if not path.exists():
            continue
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        if "ESPCONTROL_DISABLE_TODO=1" not in text:
            errors.append(f"{rel}: keep the todo list disabled on every device")
    return errors


def firmware_connectivity_api_errors(paths: tuple[Path, ...], root: Path) -> list[str]:
    errors: list[str] = []
    for path in paths:
        if not path.exists():
            continue
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        if re.search(r"(?m)^\s*api\.connected:\s*$", text):
            errors.append(f"{rel}: wait for Home Assistant state subscription, not any API client")
        if "on_client_connected:" in text and "ha_api_state_connected()" not in text:
            errors.append(f"{rel}: only navigate after a Home Assistant state connection is ready")
    return errors


def run_scan() -> int:
    errors = firmware_ha_binding_errors(FIRMWARE_DIR, ROOT)
    errors.extend(firmware_ha_boundary_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_todo_request_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_todo_disconnect_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_time_reconnect_errors(TIME_ADDON_PATH, ROOT))
    errors.extend(firmware_weather_request_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_weather_disconnect_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_weather_reconnect_errors(CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_cover_request_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(
        firmware_s3_api_errors(
            S3_DEVICE_PATH,
            S3_PACKAGES_PATH,
            CORE_INFRA_PATH,
            API_NAVIGATE_PATH,
            DEVICE_PACKAGE_PATHS,
            ROOT,
        )
    )
    errors.extend(firmware_todo_disabled_errors(DEVICE_DEVICE_PATHS, ROOT))
    errors.extend(firmware_connectivity_api_errors(CONNECTIVITY_PATHS, ROOT))
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


def expect_todo_disabled_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        paths = []
        for filename, text in files.items():
            path = root / filename
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
            paths.append(path)

        errors = firmware_todo_disabled_errors(tuple(paths), root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_time_reconnect_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        time_path = root / "common" / "addon" / "time.yaml"
        time_path.parent.mkdir(parents=True)
        time_path.write_text(text, encoding="utf-8")

        errors = firmware_time_reconnect_errors(time_path, root)
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


def expect_weather_reconnect_errors(name: str, core_text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        core_path = root / "common" / "device" / "core_infra.yaml"
        core_path.parent.mkdir(parents=True)
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_weather_reconnect_errors(core_path, root)
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


def expect_s3_api_errors(
    name: str,
    text: str,
    expected: tuple[str, ...],
    s3_packages_text: str = "packages:\n  device: !include device/device.yaml\n",
    core_text: str = "api:\n  reboot_timeout: 0s\n",
    api_navigate_text: str = "api:\n  actions:\n    - action: navigate\n",
    extra_packages: dict[str, str] | None = None,
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        device_path = root / "devices" / "guition-esp32-s3-4848s040" / "device" / "device.yaml"
        s3_packages_path = root / "devices" / "guition-esp32-s3-4848s040" / "packages.yaml"
        core_path = root / "common" / "device" / "core_infra.yaml"
        api_navigate_path = root / "common" / "device" / "api_navigate.yaml"
        device_path.parent.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        device_path.write_text(text, encoding="utf-8")
        s3_packages_path.write_text(s3_packages_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")
        api_navigate_path.write_text(api_navigate_text, encoding="utf-8")
        package_paths = [s3_packages_path]
        for filename, package_text in (extra_packages or {}).items():
            package_path = root / "devices" / filename / "packages.yaml"
            package_path.parent.mkdir(parents=True)
            package_path.write_text(package_text, encoding="utf-8")
            package_paths.append(package_path)

        errors = firmware_s3_api_errors(
            device_path,
            s3_packages_path,
            core_path,
            api_navigate_path,
            tuple(package_paths),
            root,
        )
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_connectivity_api_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "addon" / "connectivity.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_connectivity_api_errors((path,), root)
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
    expect_todo_request_errors(
        "modal close retries cancelled request",
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
        '  todo_cancel_pending_request("modal closed");\n'
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
        ("close todo modals without retrying their cancelled request",),
    )
    expect_todo_request_errors(
        "todo send failed has no retry",
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
        '  todo_cancel_pending_request("modal closed");\n'
        '  ui = TodoModalUi();\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  bool stale_request_cancelled = todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '  if (!ha_action_send(req)) {\n'
        '    todo_cancel_request(req.call_id, "send failed");\n'
        '    todo_modal_set_status("Could not load");\n'
        '  }\n'
        '}\n',
        ("retry todo loads when Home Assistant disconnects during send",),
    )
    expect_todo_request_errors(
        "pending todo request leaves modal loading",
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
        '  todo_cancel_pending_request("modal closed");\n'
        '  ui = TodoModalUi();\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  bool stale_request_cancelled = todo_cancel_stale_request();\n'
        '  if (todo_request_state().call_id != 0) {\n'
        '    return;\n'
        '  }\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '  if (!ha_action_send(req)) {\n'
        '    todo_cancel_request(req.call_id, "send failed");\n'
        '    ui.waiting_for_ha = true;\n'
        '  }\n'
        '}\n',
        ("retry todo loads when another todo request is already pending",),
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
    expect_time_reconnect_errors(
        "home assistant time sync runs on raw api connect",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        id(homeassistant_time).update();\n",
        ("guard Home Assistant time updates", "wait for Home Assistant state readiness", "defer Home Assistant time sync"),
    )
    expect_time_reconnect_errors(
        "home assistant time sync waits for state readiness",
        "api:\n"
        "  on_client_connected:\n"
        "    - delay: 2s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) id(homeassistant_time).update();\n"
        "script:\n"
        "  - id: time_update\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          if (ha_api_state_connected()) id(homeassistant_time).update();\n",
        (),
    )
    expect_weather_request_errors(
        "weather request during reconnect",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
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
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  if (!ha_action_send(req)) return;\n"
        "}\n",
        ("cancel forecast response callbacks",),
    )
    expect_weather_request_errors(
        "weather send failure missing retry",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"setup failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  if (!ha_action_send(req)) {\n"
        "    weather_forecast_clear_pending(req.call_id);\n"
        "    ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "    weather_forecast_send_next_queued();\n"
        "  }\n"
        "}\n",
        ("retry weather forecast sends that fail after callback registration",),
    )
    expect_weather_request_errors(
        "weather payload without temperatures missing retry",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"setup failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  bool valid = false;\n"
        "  if (!valid) {\n"
        "    ESP_LOGW(\"weather_forecast\", \"No usable forecast temperatures for %s\", entity_id.c_str());\n"
        "  }\n"
        "}\n",
        ("retry weather forecasts when Home Assistant returns no usable temperatures",),
    )
    expect_weather_request_errors(
        "unbounded weather callbacks",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "}\n",
        ("bound pending forecast response callbacks",),
    )
    expect_weather_request_errors(
        "missing delayed weather request retry",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "  ha_action_send(req);\n"
        "}\n",
        ("retry failed weather forecast requests later",),
    )
    expect_weather_request_errors(
        "missing robust weather timeout detection",
        "inline void request_weather_forecast_entity() {\n"
        "  if (!weather_forecast_actions_ready()) return;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
        "}\n",
        ("detect Home Assistant forecast timeout errors robustly",),
    )
    expect_weather_disconnect_errors(
        "missing weather disconnect cleanup",
        "inline void weather_forecast_cancel_pending_requests() {}\n",
        "api:\n  on_client_connected:\n    - lambda: refresh_weather_forecast_cards();\n",
        ("cancel pending forecast callbacks when the HA API disconnects",),
    )
    expect_weather_reconnect_errors(
        "unguarded weather reconnect refresh",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        refresh_weather_forecast_cards();\n"
        "    - delay: 20s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n",
        ("wait for Home Assistant state readiness",),
    )
    expect_weather_reconnect_errors(
        "missing delayed weather reconnect retry",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n",
        ("retry weather forecast refresh",),
    )
    expect_weather_reconnect_errors(
        "guarded weather reconnect refresh with delayed retry",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n"
        "    - delay: 20s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n",
        (),
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
        "api:\n  max_connections: 3\n  max_send_queue: 8\n",
        ("keep the S3 native API send queue high enough",),
    )
    expect_s3_api_errors(
        "low S3 API connections",
        "api:\n  max_connections: 2\n  max_send_queue: 12\n",
        ("keep enough S3 native API slots",),
    )
    expect_s3_api_errors(
        "S3 todo enabled",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_TODO_LITE=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("keep the S3 todo list disabled",),
    )
    expect_todo_disabled_errors(
        "todo enabled on one device",
        {
            "devices/a/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n",
            "devices/b/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_TODO_LITE=1\"\n",
        },
        ("keep the todo list disabled",),
    )
    expect_todo_disabled_errors(
        "todo disabled on all devices",
        {
            "devices/a/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n",
            "devices/b/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n",
        },
        (),
    )
    expect_s3_api_errors(
        "S3 includes navigate API package",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("omit the Home Assistant navigate API action on S3",),
        s3_packages_text="packages:\n  api_navigate: !include ../../common/device/api_navigate.yaml\n",
    )
    expect_s3_api_errors(
        "navigate action left in shared core",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("keep the navigate action out of core_infra",),
        core_text="api:\n  actions:\n    - action: navigate\n",
    )
    expect_s3_api_errors(
        "P4 package missing navigate API package",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("include the dedicated Home Assistant navigate API package",),
        extra_packages={"esp32-p4-86": "packages:\n  device: !include device/device.yaml\n"},
    )
    expect_connectivity_api_errors(
        "raw api connected navigation",
        "wifi:\n"
        "  on_connect:\n"
        "    - if:\n"
        "        condition:\n"
        "          api.connected:\n"
        "api:\n"
        "  on_client_connected:\n"
        "    - script.execute: navigate_after_api\n",
        ("wait for Home Assistant state subscription", "only navigate after a Home Assistant state connection"),
    )
    expect_connectivity_api_errors(
        "home assistant state connected navigation",
        "wifi:\n"
        "  on_connect:\n"
        "    - if:\n"
        "        condition:\n"
        "          lambda: 'return ha_api_state_connected();'\n"
        "api:\n"
        "  on_client_connected:\n"
        "    - delay: 2s\n"
        "    - if:\n"
        "        condition:\n"
        "          lambda: 'return ha_api_state_connected();'\n",
        (),
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
