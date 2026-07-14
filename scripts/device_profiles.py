#!/usr/bin/env python3
"""Shared device profile reader for build, docs, and validation tooling."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
DEVICE_MANIFEST = ROOT / "devices" / "manifest.json"
DEVICE_CATALOG = ROOT / "devices" / "catalog.json"
COMMON_ASSETS = ROOT / "common" / "assets"
DEVICES_DIR = ROOT / "devices"

VALID_CHIP_FAMILIES = {"ESP32-P4", "ESP32-S3"}
VALID_DRAG_MODES = {"swap", "displace"}
VALID_ROTATIONS = {"0", "90", "180", "270"}
VALID_DISPLAY_MODES = {"color"}
REQUIRED_FONT_ROLES = (
    "icon",
    "sensor",
    "largeSensor",
    "mediaTitle",
    "volumeNumber",
    "volumeLabel",
)
COVER_ART_SUBSTITUTION_KEYS = (
    "cover_art_size",
    "cover_art_decode_size",
    "cover_art_x",
    "cover_art_y",
    "cover_art_accent_x",
    "cover_art_accent_y",
    "cover_art_accent_width",
    "cover_art_accent_height",
    "cover_art_accent_bg_opa",
    "cover_art_accent_opa",
    "cover_art_panel_x",
    "cover_art_panel_y",
    "cover_art_panel_width",
    "cover_art_panel_height",
    "cover_art_panel_pad_top",
    "cover_art_panel_pad_bottom",
    "cover_art_panel_pad_left",
    "cover_art_panel_pad_right",
    "cover_art_panel_pad_row",
    "cover_art_title_font",
    "cover_art_title_max_height",
    "cover_art_title_line_space",
    "cover_art_artist_font",
    "cover_art_artist_pad_top",
    "cover_art_artist_long_mode",
    "cover_art_time_font",
    "cover_art_time_pad_top",
    "cover_art_progress_width",
    "cover_art_progress_height",
    "cover_art_text_color",
    "cover_art_square_overlay",
    "cover_art_live_image_updates",
)
COVER_ART_FONT_KEYS = (
    "cover_art_title_font",
    "cover_art_artist_font",
    "cover_art_time_font",
)
FONT_ID_RE = re.compile(r"^\s+id:\s+([A-Za-z0-9_]+)\s*$", re.MULTILINE)


class DeviceProfileError(RuntimeError):
    pass


PROFILE_CATEGORIES = (
    "platform",
    "display",
    "fonts",
    "network",
    "artwork",
    "audio",
    "input",
)
CANONICAL_DEVICE_KEYS = (
    "slots",
    "public",
    "layout",
    "rotation",
    "internalRelays",
    "web",
    "firmware",
)
CANONICAL_PUBLIC_KEYS = ("name", "docsPath", "screenSize", "resolution", "orientation")
CANONICAL_FIRMWARE_KEYS = ("build", "fonts", "display", "package")
CANONICAL_PACKAGE_KEYS = (
    "firmwareVersion",
    "subpageConfigChunks",
    "substitutions",
    "deviceFontPackageKey",
    "touchscreenPackage",
    "localVoiceServices",
    "networkCoprocessor",
    "esp32C6FirmwareUpdate",
    "ethernetSelectable",
    "extraPackages",
    "backlightPwmFrequency",
    "apiNavigateAction",
)


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def load_json(path: Path) -> Any:
    try:
        with path.open(encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError as exc:
        raise DeviceProfileError(f"manifest not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise DeviceProfileError(f"{rel(path)} is not valid JSON: {exc}") from exc


def load_manifest_data(path: Path = DEVICE_MANIFEST) -> dict[str, Any]:
    data = load_json(path)
    if not isinstance(data, dict):
        raise DeviceProfileError(f"{rel(path)} must contain a JSON object")
    return data


def _catalog_error(slug: str, path: str, message: str) -> DeviceProfileError:
    field = path or "<root>"
    return DeviceProfileError(f"{slug}: {field}: {message}")


def _reject_nulls(value: Any, slug: str, path: str, contributor: str) -> None:
    if value is None:
        raise _catalog_error(slug, path, f"null values are not allowed (from {contributor})")
    if isinstance(value, dict):
        for key, child in value.items():
            child_path = f"{path}.{key}" if path else key
            _reject_nulls(child, slug, child_path, contributor)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _reject_nulls(child, slug, f"{path}[{index}]", contributor)


def _merge_profile(
    target: dict[str, Any],
    incoming: dict[str, Any],
    slug: str,
    contributor: str,
    owners: dict[str, str],
    path: str = "",
) -> None:
    for key, value in incoming.items():
        field = f"{path}.{key}" if path else key
        if key not in target:
            target[key] = copy.deepcopy(value)
            if isinstance(value, dict):
                _record_leaf_owners(value, field, contributor, owners)
            else:
                owners[field] = contributor
            continue
        current = target[key]
        if isinstance(current, dict) and isinstance(value, dict):
            _merge_profile(current, value, slug, contributor, owners, field)
            continue
        if current == value:
            continue
        previous = owners.get(field, "an earlier profile")
        raise _catalog_error(
            slug,
            field,
            f"conflicting profile values from {previous} and {contributor}",
        )


def _record_leaf_owners(value: dict[str, Any], path: str, contributor: str, owners: dict[str, str]) -> None:
    for key, child in value.items():
        field = f"{path}.{key}"
        if isinstance(child, dict):
            _record_leaf_owners(child, field, contributor, owners)
        else:
            owners[field] = contributor


def _merge_config(target: dict[str, Any], config: dict[str, Any], slug: str, path: str = "") -> None:
    for key, value in config.items():
        field = f"{path}.{key}" if path else key
        if key in target:
            if isinstance(target[key], dict) and isinstance(value, dict):
                _merge_config(target[key], value, slug, field)
                continue
            raise _catalog_error(slug, field, "config collides with an inherited profile field")
        target[key] = copy.deepcopy(value)


def _apply_overrides(target: dict[str, Any], overrides: dict[str, Any], slug: str, path: str = "") -> None:
    for key, value in overrides.items():
        field = f"{path}.{key}" if path else key
        if key not in target:
            raise _catalog_error(slug, field, "override must replace an existing inherited field")
        current = target[key]
        if isinstance(value, dict):
            if not isinstance(current, dict):
                raise _catalog_error(slug, field, "object override does not match the inherited field")
            _apply_overrides(current, value, slug, field)
        else:
            target[key] = copy.deepcopy(value)


def _ordered_object(value: dict[str, Any], keys: tuple[str, ...]) -> dict[str, Any]:
    ordered = {key: value[key] for key in keys if key in value}
    ordered.update((key, child) for key, child in value.items() if key not in ordered)
    return ordered


def canonicalize_device(device: dict[str, Any]) -> dict[str, Any]:
    device = _ordered_object(device, CANONICAL_DEVICE_KEYS)
    public = device.get("public")
    if isinstance(public, dict):
        device["public"] = _ordered_object(public, CANONICAL_PUBLIC_KEYS)
    firmware = device.get("firmware")
    if isinstance(firmware, dict):
        firmware = _ordered_object(firmware, CANONICAL_FIRMWARE_KEYS)
        package = firmware.get("package")
        if isinstance(package, dict):
            firmware["package"] = _ordered_object(package, CANONICAL_PACKAGE_KEYS)
        device["firmware"] = firmware
    return device


def compose_catalog_data(data: Any) -> dict[str, Any]:
    if not isinstance(data, dict):
        raise DeviceProfileError("devices/catalog.json must contain a JSON object")
    _reject_nulls(data, "catalog", "", "catalog")
    unknown_top = set(data) - {"settings", "profiles", "devices"}
    if unknown_top:
        raise DeviceProfileError(f"devices/catalog.json has unknown fields: {', '.join(sorted(unknown_top))}")
    settings = data.get("settings", {})
    profiles = data.get("profiles", {})
    devices = data.get("devices")
    if not isinstance(settings, dict):
        raise DeviceProfileError("devices/catalog.json: settings must be an object")
    if not isinstance(profiles, dict):
        raise DeviceProfileError("devices/catalog.json: profiles must be an object")
    unknown_categories = set(profiles) - set(PROFILE_CATEGORIES)
    if unknown_categories:
        raise DeviceProfileError(
            "devices/catalog.json: unknown profile categories: " + ", ".join(sorted(unknown_categories))
        )
    for category in PROFILE_CATEGORIES:
        if not isinstance(profiles.get(category, {}), dict):
            raise DeviceProfileError(f"devices/catalog.json: profiles.{category} must be an object")
    if not isinstance(devices, dict) or not devices:
        raise DeviceProfileError("devices/catalog.json: devices must be a non-empty object")

    expanded: dict[str, Any] = {"settings": copy.deepcopy(settings), "devices": {}}
    for slug, entry in devices.items():
        if not isinstance(entry, dict):
            raise _catalog_error(slug, "<root>", "device entry must be an object")
        unknown_fields = set(entry) - {"profiles", "config", "overrides"}
        if unknown_fields:
            raise _catalog_error(slug, "<root>", f"unknown fields: {', '.join(sorted(unknown_fields))}")
        selections = entry.get("profiles", {})
        config = entry.get("config", {})
        overrides = entry.get("overrides", {})
        if not isinstance(selections, dict):
            raise _catalog_error(slug, "profiles", "must be an object")
        if not isinstance(config, dict):
            raise _catalog_error(slug, "config", "must be an object")
        if not isinstance(overrides, dict):
            raise _catalog_error(slug, "overrides", "must be an object")
        unknown = set(selections) - set(PROFILE_CATEGORIES)
        if unknown:
            raise _catalog_error(slug, "profiles", f"unknown categories: {', '.join(sorted(unknown))}")

        composed: dict[str, Any] = {}
        owners: dict[str, str] = {}
        for category in PROFILE_CATEGORIES:
            selected = selections.get(category, [])
            names = [selected] if isinstance(selected, str) else selected
            if not isinstance(names, list) or not all(isinstance(name, str) and name for name in names):
                raise _catalog_error(slug, f"profiles.{category}", "must be a profile name or list of names")
            for name in names:
                profile = profiles.get(category, {}).get(name)
                contributor = f"profiles.{category}.{name}"
                if not isinstance(profile, dict):
                    raise _catalog_error(slug, f"profiles.{category}", f"missing profile reference {name!r}")
                _merge_profile(composed, profile, slug, contributor, owners)
        _apply_overrides(composed, overrides, slug)
        _merge_config(composed, config, slug)
        expanded["devices"][slug] = canonicalize_device(composed)
    return expanded


def load_catalog_data(path: Path = DEVICE_CATALOG) -> dict[str, Any]:
    return compose_catalog_data(load_json(path))


def font_ids_from(path: Path) -> set[str]:
    if not path.exists():
        return set()
    return set(FONT_ID_RE.findall(path.read_text(encoding="utf-8")))


def common_font_ids() -> set[str]:
    ids: set[str] = set()
    for path in sorted(COMMON_ASSETS.glob("*.yaml")):
        ids.update(font_ids_from(path))
    return ids


def is_positive_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool) and value > 0


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def device_error(slug: str, message: str) -> str:
    return f"{slug}: {message}"


def require_object(slug: str, errors: list[str], value: Any, name: str) -> dict[str, Any] | None:
    if not isinstance(value, dict):
        errors.append(device_error(slug, f"{name} must be an object"))
        return None
    return value


def validate_layout(slug: str, device: dict[str, Any], errors: list[str]) -> None:
    layout = require_object(slug, errors, device.get("layout"), "layout")
    if layout is None:
        return

    slots = device.get("slots")
    if not is_positive_int(slots):
        errors.append(device_error(slug, "slots must be a positive integer"))

    for key in ("cols", "rows"):
        if not is_positive_int(layout.get(key)):
            errors.append(device_error(slug, f"layout.{key} must be a positive integer"))

    if "portraitCols" in layout and not is_positive_int(layout.get("portraitCols")):
        errors.append(device_error(slug, "layout.portraitCols must be a positive integer when set"))

    firmware_grid = layout.get("firmwareGrid")
    if not isinstance(firmware_grid, str) or not re.fullmatch(r"[1-9]\d*x[1-9]\d*", firmware_grid):
        errors.append(device_error(slug, "layout.firmwareGrid must look like '3x5'"))

    if is_positive_int(slots) and is_positive_int(layout.get("cols")) and is_positive_int(layout.get("rows")):
        expected_slots = layout["cols"] * layout["rows"]
        if slots != expected_slots:
            errors.append(
                device_error(
                    slug,
                    f"slots must equal layout.cols * layout.rows ({slots} != {expected_slots})",
                )
            )


def validate_public(slug: str, device: dict[str, Any], errors: list[str]) -> None:
    public = require_object(slug, errors, device.get("public"), "public")
    if public is None:
        return
    for key in ("name", "docsPath", "screenSize", "resolution", "orientation"):
        if not isinstance(public.get(key), str) or not public.get(key):
            errors.append(device_error(slug, f"public.{key} must be a non-empty string"))
    docs_path = public.get("docsPath")
    if isinstance(docs_path, str) and not docs_path.startswith("/screens/"):
        errors.append(device_error(slug, "public.docsPath must start with /screens/"))


def public_docs_stem(docs_path: str) -> str:
    return docs_path.rstrip("/").split("/")[-1]


def validate_public_docs_paths(devices: dict[str, Any], errors: list[str]) -> None:
    seen: dict[str, str] = {}
    for slug, device in sorted(devices.items()):
        if not isinstance(slug, str) or not isinstance(device, dict):
            continue
        public = device.get("public")
        if not isinstance(public, dict):
            continue
        docs_path = public.get("docsPath")
        if not isinstance(docs_path, str) or not docs_path:
            continue
        stem = public_docs_stem(docs_path)
        previous = seen.get(stem)
        if previous is not None:
            errors.append(device_error(slug, f"public.docsPath stem duplicates {previous}: {stem}"))
            continue
        seen[stem] = slug


def validate_build(slug: str, firmware: dict[str, Any] | None, errors: list[str]) -> None:
    if firmware is None:
        return
    build = require_object(slug, errors, firmware.get("build"), "firmware.build")
    if build is None:
        return
    chip = build.get("chip")
    if not isinstance(chip, str) or not chip:
        errors.append(device_error(slug, "firmware.build.chip must be a non-empty string"))
    elif chip not in VALID_CHIP_FAMILIES:
        valid = ", ".join(sorted(VALID_CHIP_FAMILIES))
        errors.append(device_error(slug, f"firmware.build.chip must be one of {valid}"))


def validate_fonts(
    slug: str,
    device: dict[str, Any],
    shared_font_ids: set[str],
    errors: list[str],
) -> None:
    firmware = require_object(slug, errors, device.get("firmware"), "firmware")
    if firmware is None:
        return
    validate_build(slug, firmware, errors)
    fonts = require_object(slug, errors, firmware.get("fonts"), "firmware.fonts")
    if fonts is None:
        return

    available_ids = set(shared_font_ids)
    available_ids.update(font_ids_from(DEVICES_DIR / slug / "device" / "fonts.yaml"))

    for role in REQUIRED_FONT_ROLES:
        value = fonts.get(role)
        if not isinstance(value, str) or not value:
            errors.append(device_error(slug, f"firmware.fonts.{role} must be a non-empty font id"))

    for role, font_id in sorted(fonts.items()):
        if not isinstance(font_id, str) or not font_id:
            errors.append(device_error(slug, f"firmware.fonts.{role} must be a non-empty font id"))
        elif font_id not in available_ids:
            errors.append(device_error(slug, f"firmware.fonts.{role} references unknown font id {font_id!r}"))

    display = firmware.get("display", {})
    cover_art = display.get("coverArt") if isinstance(display, dict) else None
    if isinstance(cover_art, dict):
        for key in COVER_ART_FONT_KEYS:
            font_id = cover_art.get(key)
            if isinstance(font_id, str) and font_id and font_id not in available_ids:
                errors.append(device_error(slug, f"firmware.display.coverArt.{key} references unknown font id {font_id!r}"))


def validate_display(slug: str, device: dict[str, Any], errors: list[str]) -> None:
    firmware = device.get("firmware")
    if not isinstance(firmware, dict):
        return
    display = require_object(slug, errors, firmware.get("display"), "firmware.display")
    if display is None:
        return

    if not isinstance(display.get("wrapTallLabels"), bool):
        errors.append(device_error(slug, "firmware.display.wrapTallLabels must be true or false"))
    if "infoOnly" in display and not isinstance(display["infoOnly"], bool):
        errors.append(device_error(slug, "firmware.display.infoOnly must be true or false when set"))

    if "mode" in display and display["mode"] not in VALID_DISPLAY_MODES:
        valid = ", ".join(sorted(VALID_DISPLAY_MODES))
        errors.append(device_error(slug, f"firmware.display.mode must be one of {valid} when set"))

    for key in ("widthCompensationPercent", "volumeWidthCompensationPercent"):
        if key in display and not is_number(display[key]):
            errors.append(device_error(slug, f"firmware.display.{key} must be a number when set"))
    if "imageCardDownloaders" in display:
        value = display["imageCardDownloaders"]
        if not isinstance(value, int) or isinstance(value, bool) or value < 0 or value > 6:
            errors.append(device_error(slug, "firmware.display.imageCardDownloaders must be an integer from 0 to 6 when set"))
    if "imageCardDiagnostics" in display and not isinstance(display["imageCardDiagnostics"], bool):
        errors.append(device_error(slug, "firmware.display.imageCardDiagnostics must be true or false when set"))
    if "refreshRebuildsSubpages" in display and not isinstance(display["refreshRebuildsSubpages"], bool):
        errors.append(device_error(slug, "firmware.display.refreshRebuildsSubpages must be true or false when set"))

    cover_art = require_object(slug, errors, display.get("coverArt"), "firmware.display.coverArt")
    if cover_art is not None:
        missing = sorted(set(COVER_ART_SUBSTITUTION_KEYS) - set(cover_art))
        extra = sorted(set(cover_art) - set(COVER_ART_SUBSTITUTION_KEYS))
        if missing:
            errors.append(device_error(slug, "firmware.display.coverArt is missing: " + ", ".join(missing)))
        if extra:
            errors.append(device_error(slug, "firmware.display.coverArt has unknown keys: " + ", ".join(extra)))
        for key in COVER_ART_SUBSTITUTION_KEYS:
            value = cover_art.get(key)
            if not isinstance(value, str) or not value:
                errors.append(device_error(slug, f"firmware.display.coverArt.{key} must be a non-empty string"))

    correction = display.get("colorCorrection")
    if correction is not None:
        if not isinstance(correction, dict):
            errors.append(device_error(slug, "firmware.display.colorCorrection must be an object when set"))
        else:
            for key in ("redPercent", "greenPercent", "bluePercent"):
                if key in correction and not is_number(correction[key]):
                    errors.append(device_error(slug, f"firmware.display.colorCorrection.{key} must be a number when set"))


def validate_rotation(slug: str, device: dict[str, Any], errors: list[str]) -> None:
    if "rotation" not in device:
        return

    rotation = require_object(slug, errors, device.get("rotation"), "rotation")
    if rotation is None:
        return

    if not isinstance(rotation.get("enabled"), bool):
        errors.append(device_error(slug, "rotation.enabled must be true or false"))

    options = rotation.get("options")
    if not isinstance(options, list) or not options:
        errors.append(device_error(slug, "rotation.options must be a non-empty list"))
    else:
        seen: set[str] = set()
        for option in options:
            if not isinstance(option, str) or option not in VALID_ROTATIONS:
                errors.append(
                    device_error(
                        slug,
                        "rotation.options may only contain '0', '90', '180', and '270'",
                    )
                )
            elif option in seen:
                errors.append(device_error(slug, f"rotation.options contains duplicate value {option!r}"))
            seen.add(option)

    default = rotation.get("default")
    if default is not None:
        if not isinstance(default, str) or default not in VALID_ROTATIONS:
            errors.append(device_error(slug, "rotation.default must be '0', '90', '180', or '270' when set"))
        elif isinstance(options, list) and default not in options:
            errors.append(device_error(slug, "rotation.default must be one of rotation.options"))

    if "displayOffset" in rotation and not is_number(rotation["displayOffset"]):
        errors.append(device_error(slug, "rotation.displayOffset must be a number when set"))
    if "rotateWidthCompensation" in rotation and not isinstance(rotation["rotateWidthCompensation"], bool):
        errors.append(device_error(slug, "rotation.rotateWidthCompensation must be true or false when set"))


def validate_internal_relays(slug: str, device: dict[str, Any], errors: list[str]) -> None:
    if "internalRelays" not in device:
        return

    relays = device.get("internalRelays")
    if not isinstance(relays, list):
        errors.append(device_error(slug, "internalRelays must be a list"))
        return

    keys: set[str] = set()
    for index, relay in enumerate(relays, start=1):
        prefix = f"internalRelays[{index}]"
        if not isinstance(relay, dict):
            errors.append(device_error(slug, f"{prefix} must be an object"))
            continue
        key = relay.get("key")
        label = relay.get("label")
        if not isinstance(key, str) or not key:
            errors.append(device_error(slug, f"{prefix}.key must be a non-empty string"))
        elif key in keys:
            errors.append(device_error(slug, f"internalRelays contains duplicate key {key!r}"))
        else:
            keys.add(key)
        if not isinstance(label, str) or not label:
            errors.append(device_error(slug, f"{prefix}.label must be a non-empty string"))


def validate_package(slug: str, device: dict[str, Any], errors: list[str]) -> None:
    firmware = device.get("firmware")
    if not isinstance(firmware, dict):
        return

    package = require_object(slug, errors, firmware.get("package"), "firmware.package")
    if package is None:
        return

    for key in ("firmwareVersion", "deviceFontPackageKey"):
        if key in package and (not isinstance(package.get(key), str) or not package.get(key)):
            errors.append(device_error(slug, f"firmware.package.{key} must be a non-empty string when set"))

    for key in (
        "networkCoprocessor",
        "ethernetSelectable",
        "improvSerial",
        "touchscreenPackage",
        "localVoiceServices",
        "apiNavigateAction",
        "esp32C6FirmwareUpdate",
    ):
        if key in package and not isinstance(package[key], bool):
            errors.append(device_error(slug, f"firmware.package.{key} must be true or false when set"))

    if "subpageConfigChunks" in package:
        chunks = package["subpageConfigChunks"]
        if not isinstance(chunks, int) or isinstance(chunks, bool) or chunks not in (4, 8):
            errors.append(device_error(slug, "firmware.package.subpageConfigChunks must be 4 or 8 when set"))

    substitutions = package.get("substitutions")
    if not isinstance(substitutions, dict) or not substitutions:
        errors.append(device_error(slug, "firmware.package.substitutions must be a non-empty object"))
    else:
        for key, value in sorted(substitutions.items()):
            if not isinstance(key, str) or not key:
                errors.append(device_error(slug, "firmware.package.substitutions keys must be non-empty strings"))
            if not isinstance(value, str) or not value:
                errors.append(device_error(slug, f"firmware.package.substitutions.{key} must be a non-empty string"))
        card_gap = substitutions.get("main_page_card_gap")
        if not isinstance(card_gap, str) or not re.fullmatch(r'"[1-9][0-9]*"', card_gap):
            errors.append(
                device_error(
                    slug,
                    'firmware.package.substitutions.main_page_card_gap must be a quoted positive pixel value, for example "\\"10\\""',
                )
            )

    if package.get("ethernetSelectable") or "backlightPwmFrequency" in package:
        frequencies = require_object(
            slug,
            errors,
            package.get("backlightPwmFrequency"),
            "firmware.package.backlightPwmFrequency",
        )
        if frequencies is not None:
            for key in ("wifi", "ethernet"):
                if not isinstance(frequencies.get(key), str) or not frequencies.get(key):
                    errors.append(
                        device_error(
                            slug,
                            f"firmware.package.backlightPwmFrequency.{key} must be a non-empty string",
                        )
                    )


def validate_screen_box(slug: str, errors: list[str], value: Any, name: str) -> None:
    screen = require_object(slug, errors, value, name)
    if screen is None:
        return
    for key in ("width", "aspect"):
        if not isinstance(screen.get(key), str) or not screen.get(key):
            errors.append(device_error(slug, f"{name}.{key} must be a non-empty string"))
    width = screen.get("width")
    if isinstance(width, str) and not re.fullmatch(r"(?:[1-9]\d*(?:\.\d+)?|0\.\d+)%", width):
        errors.append(device_error(slug, f"{name}.width must be a percentage, for example '100%'"))
    aspect = screen.get("aspect")
    if isinstance(aspect, str) and not re.fullmatch(r"[1-9]\d*/[1-9]\d*", aspect):
        errors.append(device_error(slug, f"{name}.aspect must look like '1024/600'"))


def validate_numeric_fields(slug: str, errors: list[str], value: Any, name: str, fields: tuple[str, ...]) -> None:
    block = require_object(slug, errors, value, name)
    if block is None:
        return
    for key in fields:
        if not is_number(block.get(key)):
            errors.append(device_error(slug, f"{name}.{key} must be a number"))


def validate_web(slug: str, device: dict[str, Any], errors: list[str]) -> None:
    web = require_object(slug, errors, device.get("web"), "web")
    if web is None:
        return

    if web.get("dragMode") not in VALID_DRAG_MODES:
        errors.append(device_error(slug, "web.dragMode must be swap or displace"))
    if not isinstance(web.get("dragAnimation"), bool):
        errors.append(device_error(slug, "web.dragAnimation must be true or false"))
    if "infoOnly" in web and not isinstance(web["infoOnly"], bool):
        errors.append(device_error(slug, "web.infoOnly must be true or false when set"))
    if "coverArtSquareOverlay" in web and not isinstance(web["coverArtSquareOverlay"], bool):
        errors.append(device_error(slug, "web.coverArtSquareOverlay must be true or false when set"))
    disabled_card_types = web.get("disabledCardTypes", [])
    if not isinstance(disabled_card_types, list) or not all(
        isinstance(value, str) and value for value in disabled_card_types
    ):
        errors.append(device_error(slug, "web.disabledCardTypes must be a list of non-empty strings"))


    validate_screen_box(slug, errors, web.get("screen"), "web.screen")

    portrait = web.get("portrait")
    if portrait is not None:
        portrait_obj = require_object(slug, errors, portrait, "web.portrait")
        if portrait_obj is not None:
            for key in ("cols", "rows"):
                if not is_positive_int(portrait_obj.get(key)):
                    errors.append(device_error(slug, f"web.portrait.{key} must be a positive integer"))
            validate_screen_box(slug, errors, portrait_obj.get("screen"), "web.portrait.screen")

    topbar = require_object(slug, errors, web.get("topbar"), "web.topbar")
    if topbar is not None:
        for key in ("height", "fontSize"):
            if not is_number(topbar.get(key)):
                errors.append(device_error(slug, f"web.topbar.{key} must be a number"))
        if not isinstance(topbar.get("padding"), str) or not topbar.get("padding"):
            errors.append(device_error(slug, "web.topbar.padding must be a non-empty string"))

    grid = require_object(slug, errors, web.get("grid"), "web.grid")
    if grid is not None:
        for key in ("top", "compactTop", "left", "right", "bottom", "gap"):
            if not is_number(grid.get(key)):
                errors.append(device_error(slug, f"web.grid.{key} must be a number"))
        if not isinstance(grid.get("fr"), str) or not grid.get("fr"):
            errors.append(device_error(slug, "web.grid.fr must be a non-empty string"))

    btn = require_object(slug, errors, web.get("btn"), "web.btn")
    if btn is not None:
        for key in ("radius", "padding", "iconSize", "labelSize"):
            if not is_number(btn.get(key)):
                errors.append(device_error(slug, f"web.btn.{key} must be a number"))
        if "borderWidth" in btn and not is_number(btn.get("borderWidth")):
            errors.append(device_error(slug, "web.btn.borderWidth must be a number when set"))
        if "labelWeight" in btn and not is_positive_int(btn.get("labelWeight")):
            errors.append(device_error(slug, "web.btn.labelWeight must be a positive integer when set"))
        for key in ("labelLines", "labelLinesDouble"):
            if not is_positive_int(btn.get(key)):
                errors.append(device_error(slug, f"web.btn.{key} must be a positive integer"))

    validate_numeric_fields(slug, errors, web.get("emptyCell"), "web.emptyCell", ("radius",))
    validate_numeric_fields(slug, errors, web.get("sensorBadge"), "web.sensorBadge", ("top", "right", "fontSize"))
    validate_numeric_fields(slug, errors, web.get("subpageBadge"), "web.subpageBadge", ("bottom", "right", "fontSize"))


def validate_manifest_data(data: Any, shared_font_ids: set[str] | None = None) -> list[str]:
    errors: list[str] = []
    if not isinstance(data, dict):
        return [f"{rel(DEVICE_MANIFEST)} must contain a JSON object"]

    devices = data.get("devices")
    if not isinstance(devices, dict) or not devices:
        return [f"{rel(DEVICE_MANIFEST)} must contain a non-empty devices object"]

    validate_public_docs_paths(devices, errors)
    shared_font_ids = common_font_ids() if shared_font_ids is None else shared_font_ids
    for slug, device in sorted(devices.items()):
        if not isinstance(slug, str) or not slug:
            errors.append("device keys must be non-empty strings")
            continue
        if not isinstance(device, dict):
            errors.append(device_error(slug, "device entry must be an object"))
            continue
        validate_public(slug, device, errors)
        validate_layout(slug, device, errors)
        validate_fonts(slug, device, shared_font_ids, errors)
        validate_display(slug, device, errors)
        validate_rotation(slug, device, errors)
        validate_internal_relays(slug, device, errors)
        validate_package(slug, device, errors)
        validate_web(slug, device, errors)

    return errors


def normalized_device_profile(slug: str, device: dict[str, Any], settings: dict[str, Any]) -> dict[str, Any]:
    firmware = device["firmware"]
    return {
        "slug": slug,
        "slots": device["slots"],
        "public": copy.deepcopy(device["public"]),
        "layout": copy.deepcopy(device["layout"]),
        "rotation": copy.deepcopy(device.get("rotation") or {}),
        "internalRelays": copy.deepcopy(device.get("internalRelays") or []),
        "web": copy.deepcopy(device["web"]),
        "firmware": {
            "build": copy.deepcopy(firmware["build"]),
            "fonts": copy.deepcopy(firmware["fonts"]),
            "display": copy.deepcopy(firmware.get("display") or {}),
            "package": copy.deepcopy(firmware["package"]),
        },
        "settings": copy.deepcopy(settings),
    }


def load_device_profiles(path: Path = DEVICE_MANIFEST) -> dict[str, dict[str, Any]]:
    data = load_catalog_data() if path.resolve() == DEVICE_MANIFEST.resolve() else load_manifest_data(path)
    errors = validate_manifest_data(data)
    if errors:
        raise DeviceProfileError("\n".join(errors))
    settings = {
        "largeSensorUnitOffsetPercent": -10,
        **data.get("settings", {}),
    }
    return {
        slug: normalized_device_profile(slug, device, settings)
        for slug, device in data["devices"].items()
    }


def web_features(profile: dict[str, Any]) -> dict[str, Any]:
    features: dict[str, Any] = {}
    package = profile["firmware"]["package"]
    rotation = profile.get("rotation") or {}
    if rotation.get("enabled"):
        features["screenRotation"] = True
        features["screenRotationOptions"] = rotation.get("options", [])
        if "default" in rotation:
            features["screenRotationDefault"] = rotation["default"]
        if "displayOffset" in rotation:
            features["screenRotationDisplayOffset"] = rotation["displayOffset"]
    if profile.get("internalRelays"):
        features["internalRelays"] = copy.deepcopy(profile["internalRelays"])
    if package.get("localVoiceServices"):
        features["voiceServices"] = True
    if package.get("subpageConfigChunks"):
        features["subpageConfigChunks"] = package["subpageConfigChunks"]
    return features


def web_config(profile: dict[str, Any]) -> dict[str, Any]:
    layout = profile["layout"]
    features = web_features(profile)
    display = profile["firmware"].get("display") or {}
    image_card_limit = display.get("imageCardDownloaders", 4)
    cfg: dict[str, Any] = {
        "slots": profile["slots"],
        "cols": layout["cols"],
        "rows": layout["rows"],
        "screenSize": profile["public"]["screenSize"],
        "largeSensorUnitOffsetPercent": profile["settings"]["largeSensorUnitOffsetPercent"],
        "imageCardLimit": image_card_limit,
    }
    for key, value in profile["web"].items():
        cfg[key] = copy.deepcopy(value)
        if key == "dragAnimation" and features:
            cfg["features"] = copy.deepcopy(features)
    if image_card_limit == 0:
        disabled = list(cfg.get("disabledCardTypes") or [])
        if "image" not in disabled:
            disabled.append("image")
        cfg["disabledCardTypes"] = disabled
    if features and "features" not in cfg:
        cfg["features"] = copy.deepcopy(features)
    return cfg


def slot_device(profile: dict[str, Any]) -> dict[str, Any]:
    layout = profile["layout"]
    firmware = profile["firmware"]
    fonts = firmware["fonts"]
    display = firmware["display"]
    rotation = profile["rotation"]
    slot = {
        "slug": profile["slug"],
        "slots": profile["slots"],
        "cols": layout["cols"],
        "grid": layout["firmwareGrid"],
        "icon_font": fonts["icon"],
        "sensor_font": fonts["sensor"],
        "large_sensor_font": fonts["largeSensor"],
        "large_sensor_unit_offset_percent": profile["settings"]["largeSensorUnitOffsetPercent"],
        "media_title_font": fonts["mediaTitle"],
        "media_control_title_font": fonts.get("mediaControlTitle"),
        "volume_number_font": fonts["volumeNumber"],
        "volume_label_font": fonts["volumeLabel"],
        "cover_art": copy.deepcopy(display["coverArt"]),
        "climate_card_icon_font": fonts.get("climateCardIcon"),
        "subpage_chevron_font": fonts.get("subpageChevron"),
        "climate_option_title_font": fonts.get("climateOptionTitle"),
        "climate_option_value_font": fonts.get("climateOptionValue"),
        "wrap_tall_labels": display["wrapTallLabels"],
        "info_only": bool(display.get("infoOnly")),
        "display_mode": display.get("mode", "color"),
        "package": firmware.get("package"),
    }
    if "portraitCols" in layout:
        slot["portrait_cols"] = layout["portraitCols"]
    if display.get("widthCompensationPercent", 100) != 100:
        slot["width_compensation_percent"] = display["widthCompensationPercent"]
    if display.get("volumeWidthCompensationPercent", 100) != 100:
        slot["volume_width_compensation_percent"] = display["volumeWidthCompensationPercent"]
    if display.get("mediaArtworkWidthCompensationPercent", 100) != 100:
        slot["media_artwork_width_compensation_percent"] = display["mediaArtworkWidthCompensationPercent"]
    if display.get("subpageChevronX", 0) != 0:
        slot["subpage_chevron_x"] = display["subpageChevronX"]
    if display.get("subpageChevronY", 2) != 2:
        slot["subpage_chevron_y"] = display["subpageChevronY"]
    if display.get("subpageChevronTextWidthPercent", 94) != 94:
        slot["subpage_chevron_text_width_percent"] = display["subpageChevronTextWidthPercent"]
    if display.get("colorCorrection"):
        correction = display["colorCorrection"]
        slot["color_correction"] = {
            "red": correction.get("redPercent", 100),
            "green": correction.get("greenPercent", 100),
            "blue": correction.get("bluePercent", 100),
        }
    if display.get("imageCardDownloaders", 4) != 4:
        slot["image_card_downloaders"] = display["imageCardDownloaders"]
    if display.get("imageCardDiagnostics"):
        slot["image_card_diagnostics"] = True
    if display.get("refreshRebuildsSubpages"):
        slot["refresh_rebuilds_subpages"] = True
    if rotation.get("rotateWidthCompensation"):
        slot["rotate_width_compensation"] = True
    return slot


def slot_devices(path: Path = DEVICE_MANIFEST) -> list[dict[str, Any]]:
    return [slot_device(profile) for profile in load_device_profiles(path).values()]


def public_device_capability(profile: dict[str, Any]) -> dict[str, Any]:
    package = profile["firmware"]["package"]
    capability = {
        "slug": profile["slug"],
        "installSlug": profile["slug"],
        "name": profile["public"]["name"],
        "docsPath": profile["public"]["docsPath"],
        "screenSize": profile["public"]["screenSize"],
        "resolution": profile["public"]["resolution"],
        "orientation": profile["public"]["orientation"],
        "slots": profile["slots"],
        "grid": {
            "rows": profile["layout"]["rows"],
            "cols": profile["layout"]["cols"],
        },
        "chipFamily": profile["firmware"]["build"]["chip"],
        "relays": len(profile["internalRelays"]),
        "rotation": bool((profile.get("rotation") or {}).get("enabled")),
        "ethernetManualInstall": bool(package.get("ethernetSelectable")),
        "subpages": "subpage" not in profile["web"].get("disabledCardTypes", []),
    }
    display = profile["firmware"].get("display") or {}
    return capability


def public_device_capabilities(path: Path = DEVICE_MANIFEST) -> dict[str, Any]:
    profiles = load_device_profiles(path)
    return {
        "generatedFrom": "devices/manifest.json",
        "devices": [
            public_device_capability(profile)
            for profile in profiles.values()
        ],
    }
