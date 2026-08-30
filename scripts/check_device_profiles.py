#!/usr/bin/env python3
"""Cross-check generated device profile outputs against devices/manifest.json."""

from __future__ import annotations

import json
import re
from pathlib import Path

import device_matrix
import generate_device_slots
from device_profiles import ROOT, load_device_profiles, public_device_capabilities, web_config
import check_public_firmware


WEB_OUTPUT_DIR = ROOT / "docs" / "public" / "webserver"
DEVICE_CAPABILITIES_JSON = ROOT / "docs" / "public" / "device-profiles.json"
DEVICE_DOCS_DIR = ROOT / "docs" / "generated" / "screens"
COMPAT_FIXTURES = ROOT / "product" / "v2" / "product_compatibility.json"
BUTTON_GRID_CARDS = ROOT / "components" / "espcontrol" / "button_grid_cards.h"
BUTTON_GRID_WEATHER_DRIVER = ROOT / "components" / "espcontrol" / "button_grid_weather_driver.h"
BUTTON_GRID_WEATHER_FORECAST = ROOT / "components" / "espcontrol" / "button_grid_weather_forecast.h"
WEB_SERVER_IDF_INIT = ROOT / "components" / "web_server_idf" / "__init__.py"
WEB_SERVER_IDF_CPP = ROOT / "components" / "web_server_idf" / "web_server_idf.cpp"
S3_DEVICE_YAML = ROOT / "devices" / "guition-esp32-s3-4848s040" / "device" / "device.yaml"
S3_ARTWORK_TRANSFER_CPP = ROOT / "components" / "artwork_image" / "s3_artwork_transfer.cpp"
PUBLIC_API_ENCRYPTION_PACKAGE = ROOT / "common" / "addon" / "api_encryption_dynamic.yaml"
PUBLIC_API_ENCRYPTION_REFERENCE = "common/addon/api_encryption_dynamic.yaml"
LEGACY_OTA_PARTITION_LAYOUTS = {
    "esp32-p4-86": "partitions_32mb_card_images.csv",
    "guition-esp32-p4-jc1060p470": "partitions_16mb_card_images.csv",
    "guition-esp32-p4-jc1060p470-v2": "partitions_16mb_card_images.csv",
    "guition-esp32-p4-jc4880p443": "partitions_16mb_card_images.csv",
    "guition-esp32-p4-jc8012p4a1": "partitions_16mb_card_images.csv",
    "guition-esp32-p4-jc8012p4a1-v2": "partitions_16mb_card_images.csv",
    "guition-esp32-s3-4848s040": "partitions_16mb_card_images.csv",
}
LEGACY_OTA_PARTITION_ROWS = {
    "partitions_16mb_card_images.csv": (
        "nvs,           data, nvs,     0x9000,    0xd000,",
        "otadata,       data, ota,     0x16000,   0x2000,",
        "app0,          app,  ota_0,   0x20000,   0x6f0000,",
        "app1,          app,  ota_1,   0x710000,  0x6f0000,",
        "card_images,   data, 0x40,    0xe00000,  0x200000,",
    ),
    "partitions_32mb_card_images.csv": (
        "nvs,           data, nvs,     0x9000,    0xd000,",
        "otadata,       data, ota,     0x16000,   0x2000,",
        "app0,          app,  ota_0,   0x20000,   0xef0000,",
        "app1,          app,  ota_1,   0xf10000,  0xef0000,",
        "card_images,   data, 0x40,    0x1e00000, 0x200000,",
    ),
}
REQUIRED_SETUP_ICON_GLYPHS = {
    r'"\U000F012C"': "mdi-check",
    r'"\U000F0996"': "mdi-progress-clock",
}
REQUIRED_LIGHT_CONTROL_ICON_GLYPHS = {
    r'"\U000F0425"': "mdi-power",
    r'"\U000F0766"': "mdi-circle-outline",
}
REQUIRED_CLIMATE_CARD_ICON_NAMES = {
    "Air Filter",
    "Fan",
    "Fire",
    "Power",
    "Snowflake",
    "Swap Horizontal",
    "Thermometer",
    "Thermostat",
    "Thermostat Auto",
    "Water",
}


def read_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def compatibility_required_slugs() -> list[str]:
    fixture = read_json(COMPAT_FIXTURES)
    return fixture["current"]["deviceProfiles"]["requiredSlugs"]


def docs_stem(capability: dict) -> str:
    return capability["docsPath"].rstrip("/").split("/")[-1]


def assert_profile_slugs(profile_slugs: list[str], values: list[str], label: str) -> None:
    assert values == profile_slugs, f"{label} slugs differ: {values} != {profile_slugs}"


def image_slot_capacity(profile: dict) -> int:
    return int(profile["capabilities"]["imageSlots"])


def test_zero_image_capacity_disables_all_image_card_pickers(profiles: dict[str, dict]) -> None:
    for slug, profile in profiles.items():
        if image_slot_capacity(profile) != 0:
            continue
        disabled = set(web_config(profile).get("disabledCardTypes", []))
        assert {"image", "media_cover_art"} <= disabled, (
            f"{slug}: zero image capacity must disable Image and Media Cover Art cards"
        )


def test_constrained_s3_supports_one_cover_art_card(profiles: dict[str, dict]) -> None:
    profile = profiles["guition-esp32-s3-4848s040"]
    disabled = set(web_config(profile).get("disabledCardTypes", []))
    assert image_slot_capacity(profile) == 1, "S3 must provide one low-memory artwork slot"
    assert "image" in disabled, "S3 must keep general Image cards unavailable"
    assert "media_cover_art" not in disabled, "S3 must expose Media Cover Art cards"


def test_public_device_capabilities(profile_slugs: list[str]) -> None:
    expected = public_device_capabilities()
    actual = read_json(DEVICE_CAPABILITIES_JSON)
    assert actual == expected, "public device capability JSON is stale"
    assert_profile_slugs(profile_slugs, [device["slug"] for device in actual["devices"]], "public capability")

    for capability in actual["devices"]:
        stem = docs_stem(capability)
        grid = (DEVICE_DOCS_DIR / f"{stem}-grid.md").read_text(encoding="utf-8")
        install = (DEVICE_DOCS_DIR / f"{stem}-install.md").read_text(encoding="utf-8")
        assert f'**{capability["slots"]} card slots**' in grid, f"{stem}: grid snippet missing slot count"
        assert f'{capability["grid"]["rows"]}-row x {capability["grid"]["cols"]}-column' in grid, (
            f"{stem}: grid snippet missing grid shape"
        )
        if capability.get("subpages", True):
            assert "[Subpage](/features/subpages)" in grid, f"{stem}: grid snippet missing subpage support"
        else:
            assert "Touch subpages are not available" in grid, f"{stem}: grid snippet missing no-subpage note"
        assert "| Capability | Value |" not in grid, f"{stem}: grid snippet must not include a device specs table"
        assert f'slug="{capability["installSlug"]}"' in install, f"{stem}: install snippet missing slug"


def test_generated_web(profiles: dict[str, dict]) -> None:
    bridge_path = WEB_OUTPUT_DIR / "www.js"
    path = WEB_OUTPUT_DIR / "embedded" / "www.js"
    assert bridge_path.is_file(), "shared generated web bridge is missing"
    assert path.is_file(), "embedded generated web bundle is missing"
    bridge = bridge_path.read_text(encoding="utf-8")
    assert "web-assets.json" in bridge, "shared hosted web URL does not use the asset manifest"
    text = path.read_text(encoding="utf-8")

    for slug, profile in profiles.items():
        assert slug in text, f"{slug}: shared generated web bundle is missing the device profile"
        loader_path = WEB_OUTPUT_DIR / slug / "www.js"
        loader = loader_path.read_text(encoding="utf-8")
        assert len(loader) < 1024, f"{slug}: compatibility loader unexpectedly contains a full web bundle"
        assert 'new URL("../www.js"' in loader and slug in loader, (
            f"{slug}: compatibility loader does not launch the shared bundle"
        )
        capacity = image_slot_capacity(profile)
        assert f"imageSlotCapacity:{capacity}" in text or f'"imageSlotCapacity":{capacity}' in text, (
            f"{slug}: generated web bundle has wrong image slot capacity"
        )

    core = (ROOT / "common" / "device" / "core_infra.yaml").read_text(encoding="utf-8")
    assert "webserver/www.js?device=${device_slug}" in core, "hosted web URL does not select a shared profile"
    assert 'ESPCONTROL_DEVICE_SLUG=\\"${device_slug}\\"' in core, "firmware build does not expose its profile slug"
    server = (ROOT / "components" / "web_server_idf" / "web_server_idf.cpp").read_text(encoding="utf-8")
    assert '\\"device_slug\\"' in server and "ESPCONTROL_DEVICE_PROFILE" in server, (
        "firmware metadata endpoint does not expose the shared web profile"
    )
    for slug in profiles:
        dev = (ROOT / "devices" / slug / "dev.yaml").read_text(encoding="utf-8")
        assert 'js_include: "../../docs/public/webserver/embedded/www.js"' in dev, (
            f"{slug}: local development firmware does not embed its offline editor"
        )
        for suffix in (".yaml", ".factory.yaml"):
            build = (ROOT / "builds" / f"{slug}{suffix}").read_text(encoding="utf-8")
            assert 'docs/public/webserver/embedded/www.js"' in build, f"{slug}{suffix}: firmware does not embed its offline editor"
        factory = (ROOT / "builds" / f"{slug}.factory.yaml").read_text(encoding="utf-8")
        assert "webserver/www.js?device=${device_slug}&v=${firmware_version}" in factory, (
            f"{slug}.factory.yaml: release firmware does not request its compatible hosted editor"
        )


def test_web_server_request_limits() -> None:
    init = WEB_SERVER_IDF_INIT.read_text(encoding="utf-8")
    server = WEB_SERVER_IDF_CPP.read_text(encoding="utf-8")
    header_limit = re.search(
        r'add_idf_sdkconfig_option\("CONFIG_HTTPD_MAX_REQ_HDR_LEN",\s*(\d+)\)',
        init,
    )
    assert header_limit and int(header_limit.group(1)) >= 4096, (
        "web server request-header limit must support modern browser headers"
    )
    assert "static constexpr size_t MAX_FORM_URLENCODED_BODY_LENGTH = 1024;" in server, (
        "form-encoded POST bodies must retain their independent 1024-byte limit"
    )
    assert "r->content_len > MAX_FORM_URLENCODED_BODY_LENGTH" in server, (
        "form-encoded POST body validation must use its independent limit"
    )
    assert "r->content_len > CONFIG_HTTPD_MAX_REQ_HDR_LEN" not in server, (
        "form-encoded POST bodies must not inherit the request-header limit"
    )


def test_s3_low_heap_policy() -> None:
    """Keep the S3 artwork path and web server within its internal-heap budget."""
    device = S3_DEVICE_YAML.read_text(encoding="utf-8")
    artwork = S3_ARTWORK_TRANSFER_CPP.read_text(encoding="utf-8")
    server = WEB_SERVER_IDF_CPP.read_text(encoding="utf-8")

    for option in (
        'CONFIG_SPIRAM_USE_MALLOC: "y"',
        'CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL: "4096"',
        'CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL: "32768"',
    ):
        assert option in device, f"S3 device profile is missing {option}"
    assert "HTTP_CLIENT_BUFFER_SIZE = 4 * 1024" in artwork, (
        "S3 artwork HTTP buffer must stay at 4 KiB"
    )
    assert "xTaskCreateWithCaps" in artwork and "MALLOC_CAP_SPIRAM" in artwork, (
        "S3 artwork transfer task must prefer a PSRAM stack"
    )
    assert "falling back to internal RAM" in artwork and "config.buffer_size = HTTP_CLIENT_BUFFER_SIZE" in artwork, (
        "S3 artwork transfer must retain an internal-stack fallback and bounded HTTP buffer"
    )
    conditional = server.split("#if defined(CONFIG_IDF_TARGET_ESP32S3)", 1)[1].split("#endif", 1)[0]
    s3_server, p4_server = conditional.split("#else", 1)
    assert "config.stack_size = 12288;" in s3_server and "config.max_open_sockets = 3;" in s3_server, (
        "S3 web server policy must use the reduced stack and socket count"
    )
    assert "config.stack_size = 16384;" in p4_server and "config.max_open_sockets = 5;" in p4_server, (
        "P4 web server policy must remain unchanged"
    )


def test_native_panel_config_bindings(slug: str, profile: dict, device: str) -> None:
    espcontrol = re.search(r"(?ms)^espcontrol:\n(?P<body>(?:^  .*\n|^\s*$\n)*)", device)
    assert espcontrol, f"{slug}: device.yaml is missing its espcontrol block"
    body = espcontrol.group("body")
    if "  panel_config:\n" not in body:
        return

    slots = int(profile["slots"])
    bindings = [
        int(slot)
        for slot in re.findall(r"(?m)^      - config: button_(\d+)_config$", body)
    ]
    assert bindings == list(range(1, slots + 1)), (
        f"{slug}: native panel config bindings must cover slots 1-{slots} exactly; "
        f"found {bindings}"
    )

    chunk_rows = re.findall(r"(?m)^        subpage_chunks: \[([^\n]+)\]$", body)
    assert len(chunk_rows) == slots, (
        f"{slug}: native panel config must provide subpage chunks for all {slots} slots"
    )
    for slot, row in enumerate(chunk_rows, start=1):
        expected = [f"subpage_{slot}_config", f"subpage_{slot}_config_ext"] + [
            f"subpage_{slot}_config_ext_{suffix}" for suffix in range(2, 8)
        ]
        actual = [value.strip() for value in row.split(",")]
        assert actual == expected, (
            f"{slug}: slot {slot} native subpage bindings differ: {actual} != {expected}"
        )


def test_generated_yaml(profiles: dict[str, dict]) -> None:
    for slug, profile in profiles.items():
        package_path = ROOT / "devices" / slug / "packages.yaml"
        device_path = ROOT / "devices" / slug / "device" / "device.yaml"
        sensor_path = ROOT / "devices" / slug / "device" / "sensors.yaml"
        package = package_path.read_text(encoding="utf-8")
        device = device_path.read_text(encoding="utf-8")
        sensors = sensor_path.read_text(encoding="utf-8")
        assert f'device_slug: "{slug}"' in package, f"{slug}: packages.yaml missing device slug"
        assert f'firmware_manifest_slug: "{slug}"' in package, f"{slug}: packages.yaml missing manifest slug"
        assert f"cfg.num_slots = {profile['slots']};" in sensors, f"{slug}: sensors.yaml missing slot count"
        test_native_panel_config_bindings(slug, profile, device)
        label_lines = profile["web"]["btn"]["labelLines"]
        label_lines_tall = profile["web"]["btn"]["labelLinesDouble"]
        assert f"cfg.label_lines = {label_lines};" in sensors, (
            f"{slug}: sensors.yaml must use the web preview's one-high label line limit"
        )
        assert f"cfg.label_lines_tall = {label_lines_tall};" in sensors, (
            f"{slug}: sensors.yaml must use the web preview's tall-card label line limit"
        )
        capacity = image_slot_capacity(profile)
        if capacity > 0:
            package_name = "image_cards.yaml" if capacity == 4 else f"image_cards_{capacity}.yaml"
            assert package_name in package, f"{slug}: packages.yaml missing {package_name}"
            assert f"cfg.image_card_image_count = {capacity};" in sensors, (
                f"{slug}: sensors.yaml missing image-card downloader count"
            )
            assert f"id(image_card_download_{capacity})," in sensors, (
                f"{slug}: sensors.yaml missing final image-card tile downloader"
            )
            assert "cfg.image_card_modal_image = id(image_card_modal_download_1);" in sensors, (
                f"{slug}: sensors.yaml missing shared image-card modal downloader"
            )
        else:
            assert "image_cards:" not in package, f"{slug}: zero image-card profile should not include image cards"
            assert "cfg.image_card_image_count" not in sensors, (
                f"{slug}: zero image-card profile should not wire image-card downloaders"
            )
        if profile["firmware"].get("display", {}).get("infoOnly"):
            assert "cfg.info_only = true;" in sensors, f"{slug}: sensors.yaml missing info-only grid flag"


def test_public_api_encryption_policy(profile_slugs: list[str]) -> None:
    policy = PUBLIC_API_ENCRYPTION_PACKAGE.read_text(encoding="utf-8")
    assert policy == "api:\n  encryption: {}\n", (
        "public API encryption package must remain keyless and dynamically provisionable"
    )
    assert "key:" not in policy, "public firmware must not embed a shared API encryption key"
    assert "provisioning:" not in policy, "public firmware must not add a timed provisioning lockout"

    local_reference = f"api_encryption: !include ../{PUBLIC_API_ENCRYPTION_REFERENCE}"
    remote_reference = f"file: {PUBLIC_API_ENCRYPTION_REFERENCE}"
    for slug in profile_slugs:
        factory = (ROOT / "builds" / f"{slug}.factory.yaml").read_text(encoding="utf-8")
        public_config = (ROOT / "devices" / slug / "esphome.yaml").read_text(encoding="utf-8")
        dev = (ROOT / "devices" / slug / "dev.yaml").read_text(encoding="utf-8")
        local_build = (ROOT / "builds" / f"{slug}.yaml").read_text(encoding="utf-8")
        assert local_reference in factory, f"{slug}: factory firmware must support dynamic API encryption"
        assert remote_reference in public_config, f"{slug}: public config must support dynamic API encryption"
        assert PUBLIC_API_ENCRYPTION_REFERENCE not in dev, f"{slug}: dev firmware must remain plaintext"
        assert PUBLIC_API_ENCRYPTION_REFERENCE not in local_build, f"{slug}: local build must remain plaintext"

    core = (ROOT / "common" / "device" / "core_infra.yaml").read_text(encoding="utf-8")
    assert PUBLIC_API_ENCRYPTION_REFERENCE not in core, "shared core firmware must remain plaintext"

    manual_setup = (ROOT / "docs" / "getting-started" / "manual-esphome-setup.md").read_text(encoding="utf-8")
    assert manual_setup.count(remote_reference) == 3, (
        "manual WiFi, authenticated web, and Ethernet examples must include dynamic API encryption"
    )


def test_ota_preserves_deployed_partition_layouts() -> None:
    for slug, table_name in LEGACY_OTA_PARTITION_LAYOUTS.items():
        device_path = ROOT / "devices" / slug / "device" / "device.yaml"
        dev_path = ROOT / "devices" / slug / "dev.yaml"
        public_config_path = ROOT / "devices" / slug / "esphome.yaml"
        build_path = ROOT / "builds" / f"{slug}.yaml"
        factory_path = ROOT / "builds" / f"{slug}.factory.yaml"
        device = device_path.read_text(encoding="utf-8")
        dev = dev_path.read_text(encoding="utf-8")
        public_config = public_config_path.read_text(encoding="utf-8")
        build = build_path.read_text(encoding="utf-8")
        factory = factory_path.read_text(encoding="utf-8")
        assert "partitions:" not in device, (
            f"{slug}: device package must not require a local partition table from remote installs"
        )
        assert f'partitions: "../../common/device/{table_name}"' in dev, (
            f"{slug}: local development builds must retain the deployed {table_name} flash layout"
        )
        assert "partition_table:" not in public_config, (
            f"{slug}: published remote configuration must not reference a local partition table"
        )
        assert f'partitions: "../common/device/{table_name}"' in build, (
            f"{slug}: copied firmware builds must retain the deployed {table_name} flash layout"
        )
        assert (
            f'partitions: "../common/device/{table_name}"' in factory
            or f"!include {slug}.yaml" in factory
        ), f"{slug}: factory builds must retain the deployed {table_name} flash layout"

    for table_name, rows in LEGACY_OTA_PARTITION_ROWS.items():
        table = (ROOT / "common" / "device" / table_name).read_text(encoding="utf-8")
        for row in rows:
            assert row in table, f"{table_name}: missing deployed partition row {row}"


def test_upgrades_do_not_reset_saved_panel_config() -> None:
    display = (ROOT / "common" / "config" / "display.yaml").read_text(encoding="utf-8")
    generator = (ROOT / "scripts" / "generate_device_slots.py").read_text(encoding="utf-8")
    assert "panel_device_settings_reset_version" not in display, (
        "firmware upgrades must not add a stored reset marker for panel config"
    )
    assert "reset_existing_panel_settings" not in generator, (
        "generated device YAML must not include a boot-time panel config reset script"
    )

    for sensor_path in sorted((ROOT / "devices").glob("*/device/sensors.yaml")):
        text = sensor_path.read_text(encoding="utf-8")
        rel = sensor_path.relative_to(ROOT)
        assert "reset_existing_panel_settings" not in text, f"{rel}: must not reset saved panel config on boot"
        assert "id(button_order).publish_state(\"\")" not in text, f"{rel}: must not clear saved button order"
        assert not re.search(r"id\((?:button|subpage)_\d+_config(?:_ext(?:_\d+)?)?\)\.publish_state\(\"\"\)", text), (
            f"{rel}: must not clear saved button or subpage config"
        )


def test_local_voice_generation_uses_capability() -> None:
    voice_device = {
        "slug": "semantic-voice-test",
        "package": {"localVoiceServices": True},
    }
    standard_device = {
        "slug": "esp32-p4-86",
        "package": {"firmwareVersion": "dev"},
    }
    assert "open_device_volume_control" in "\n".join(
        generate_device_slots.voice_substitution_lines(voice_device)
    ), "local voice generation must follow the semantic capability"
    assert "open_device_volume_control" not in "\n".join(
        generate_device_slots.voice_substitution_lines(standard_device)
    ), "the device slug alone must not enable local voice generation"


def test_square_s3_reapplies_clock_bar_after_screen_changes() -> None:
    slug = "guition-esp32-s3-4848s040"
    sensors = (ROOT / "devices" / slug / "device" / "sensors.yaml").read_text(encoding="utf-8")
    device = (ROOT / "devices" / slug / "device" / "device.yaml").read_text(encoding="utf-8")
    assert (
        "grid_rebuild_all(slots, cfg, sp_cfgs, sp_ext, sp_ext2, sp_ext3, nullptr, nullptr, nullptr, nullptr,\n"
        "            id(button_order).state,\n"
        "            id(button_on_color).state,\n"
        "            id(main_page)->obj);\n"
        "      - script.execute: clock_bar_apply"
    ) in sensors, "S3 grid refresh must rebuild safely and reapply the fixed clock bar"
    assert (
        "grid_phase2(slots, cfg, sp_cfgs, sp_ext, sp_ext2, sp_ext3,\n"
        "              id(button_order).state,\n"
        "              id(button_on_color).state,\n"
        "              id(main_page)->obj);\n"
        "        - script.execute: clock_bar_apply"
    ) in sensors, "S3 boot setup must reapply the fixed clock bar after subpages are created"
    assert (
        "- script.execute: apply_screen_rotation\n"
        "        - script.execute: clock_bar_apply"
    ) in device, "S3 restored rotation must reapply the fixed clock bar"
    assert (
        "- script.execute: apply_screen_rotation\n"
        "              - script.execute: clock_bar_apply"
    ) in device, "S3 rotation changes must reapply the fixed clock bar"


def test_rotation_refresh_rebuilds_subpages() -> None:
    slugs = (
        "guition-esp32-p4-jc1060p470",
        "guition-esp32-p4-jc1060p470-v2",
        "guition-esp32-p4-jc4880p443",
        "guition-esp32-p4-jc8012p4a1",
        "guition-esp32-p4-jc8012p4a1-v2",
        "esp32-p4-86",
        "guition-esp32-s3-4848s040",
    )
    for slug in slugs:
        sensors = (ROOT / "devices" / slug / "device" / "sensors.yaml").read_text(encoding="utf-8")
        refresh_script = sensors.split("  - id: refresh_button_grid", 1)[1].split(
            "  - id: refresh_subpage_grid", 1)[0]
        assert "grid_rebuild_all(slots, cfg," in refresh_script, (
            f"{slug}: rotation refresh must rebuild secondary cards safely"
        )


def test_seven_inch_width_compensation_rotates_with_screen() -> None:
    profiles = load_device_profiles()
    for slug in (
        "guition-esp32-p4-jc1060p470",
        "guition-esp32-p4-jc1060p470-v2",
    ):
        profile = profiles[slug]
        assert profile["rotation"]["rotateWidthCompensation"], (
            f"{slug}: portrait rotation must move pixel compensation to the rotated axis"
        )
        assert profile["firmware"]["display"]["widthCompensationPercent"] == 95, (
            f"{slug}: 7-inch pixel compensation must remain at 95%"
        )
        assert profile["firmware"]["display"]["textWidthCompensationPercent"] == 100, (
            f"{slug}: 7-inch text must retain its natural proportions"
        )
        sensors = (ROOT / "devices" / slug / "device" / "sensors.yaml").read_text(encoding="utf-8")
        assert "cfg.width_compensation_percent = 95;" in sensors, (
            f"{slug}: generated firmware is missing 7-inch pixel compensation"
        )
        assert "cfg.width_compensation_vertical = portrait;" in sensors, (
            f"{slug}: generated firmware does not rotate the compensation axis in portrait"
        )
        assert "apply_text_width_compensation(id(display_time));" in sensors, (
            f"{slug}: clock-bar text must use the independent text compensation policy"
        )


def test_subpage_config_changes_schedule_live_refresh() -> None:
    templates = {
        "common/config/button_template.yaml": 8,
        "common/config/button_template_4chunk.yaml": 4,
    }
    for relative_path, subpage_config_count in templates.items():
        text = (ROOT / relative_path).read_text(encoding="utf-8")
        assert text.count("- script.execute: refresh_subpage_grid") == subpage_config_count + 1, (
            f"{relative_path}: the parent and every subpage config chunk must refresh the secondary page"
        )

    for sensors_path in sorted((ROOT / "devices").glob("*/device/sensors.yaml")):
        sensors = sensors_path.read_text(encoding="utf-8")
        assert "  - id: refresh_subpage_grid" in sensors, (
            f"{sensors_path}: missing secondary-page refresh script"
        )
        refresh_script = sensors.split("  - id: refresh_subpage_grid", 1)[1].split("\nesphome:", 1)[0]
        assert "grid_rebuild_all(slots, cfg," in refresh_script, (
            f"{sensors_path}: secondary-page refresh must rebuild card subscriptions"
        )

    grid_runtime = (ROOT / "components/espcontrol/button_grid_grid.h").read_text(encoding="utf-8")
    assert "inline bool grid_rebuild_all(" in grid_runtime, (
        "secondary-page refresh must use the full runtime cleanup path"
    )
    assert "navigation_active_subpage_slot()" in grid_runtime, (
        "secondary-page refresh must restore the page that was active before rebuilding"
    )


def web_screen_width_percent(profile: dict) -> float:
    width = str(profile["web"]["screen"]["width"]).strip()
    assert width.endswith("%"), f"{profile['public']['name']}: web screen width must be a percentage"
    return float(width[:-1])


def parse_resolution(profile: dict) -> tuple[int, int]:
    resolution = str(profile["public"]["resolution"]).strip()
    match = re.fullmatch(r"([1-9]\d*)\s*x\s*([1-9]\d*)", resolution)
    assert match, f"{profile['slug']}: public resolution must look like '1024 x 600'"
    return int(match.group(1)), int(match.group(2))


def parse_aspect(profile: dict, key_path: str, value: str) -> tuple[int, int]:
    match = re.fullmatch(r"([1-9]\d*)/([1-9]\d*)", str(value).strip())
    assert match, f"{profile['slug']}: {key_path} must look like '1024/600'"
    return int(match.group(1)), int(match.group(2))


def orientation_for(width: int, height: int) -> str:
    if width == height:
        return "Square"
    return "Landscape" if width > height else "Portrait"


def assert_same_ratio(slug: str, label: str, left: tuple[int, int], right: tuple[int, int]) -> None:
    assert left[0] * right[1] == left[1] * right[0], (
        f"{slug}: {label} must match the public screen resolution"
    )


def test_web_screen_aspect_matches_public_resolution() -> None:
    profiles = load_device_profiles()
    for slug, profile in profiles.items():
        resolution = parse_resolution(profile)
        assert profile["public"]["orientation"] == orientation_for(*resolution), (
            f"{slug}: public orientation must match public resolution"
        )
        screen = parse_aspect(profile, "web.screen.aspect", profile["web"]["screen"]["aspect"])
        assert_same_ratio(slug, "web.screen.aspect", screen, resolution)

        portrait = profile["web"].get("portrait")
        if portrait:
            portrait_screen = parse_aspect(
                profile,
                "web.portrait.screen.aspect",
                portrait["screen"]["aspect"],
            )
            assert_same_ratio(
                slug,
                "web.portrait.screen.aspect",
                portrait_screen,
                (resolution[1], resolution[0]),
            )


def test_web_grid_spacing_matches_across_screen_sizes() -> None:
    profiles = load_device_profiles()
    expected = None
    for slug, profile in profiles.items():
        grid = profile["web"]["grid"]
        rendered_gap = float(grid["gap"]) * web_screen_width_percent(profile) / 100.0
        if expected is None:
            expected = rendered_gap
        assert abs(rendered_gap - expected) <= 0.01, (
            f"{slug}: web preview grid spacing must match the other generated screen layouts"
        )


def test_setup_icon_glyphs() -> None:
    glyphs = (ROOT / "common" / "assets" / "icon_glyphs.yaml").read_text(encoding="utf-8")
    for glyph, icon_name in REQUIRED_SETUP_ICON_GLYPHS.items():
        assert glyph in glyphs, f"shared icon font missing {icon_name} for OTA update screen"
    for glyph, icon_name in REQUIRED_LIGHT_CONTROL_ICON_GLYPHS.items():
        assert glyph in glyphs, f"shared icon font missing {icon_name} for light control modal"


def test_climate_card_icon_glyphs() -> None:
    icons = read_json(ROOT / "common" / "assets" / "icons.json")
    icon_by_name = {icon["name"]: icon for icon in icons["icons"]}
    glyphs = (ROOT / "common" / "assets" / "climate_card_icon_glyphs.yaml").read_text(encoding="utf-8")

    for icon_name in sorted(REQUIRED_CLIMATE_CARD_ICON_NAMES):
        icon = icon_by_name[icon_name]
        glyph = rf'"\U{icon["codepoint"]:>08s}"'
        assert glyph in glyphs, f"climate card icon font missing {icon_name}"

    for font_path in sorted((ROOT / "devices").glob("*/device/fonts.yaml")):
        text = font_path.read_text(encoding="utf-8")
        rel = font_path.relative_to(ROOT)
        card_match = re.search(
            r"id: font_icon_card\n\s*size: \d+\n\s*bpp: \d+\n\s*glyphs: !include (.+)",
            text,
        )
        assert card_match, f"{rel}: missing font_icon_card"
        assert card_match.group(1).strip().endswith("common/assets/climate_card_icon_glyphs.yaml"), (
            f"{rel}: font_icon_card should use climate_card_icon_glyphs.yaml"
        )


def test_weather_card_visual_matches_preview() -> None:
    cards = BUTTON_GRID_CARDS.read_text(encoding="utf-8")
    weather_driver = BUTTON_GRID_WEATHER_DRIVER.read_text(encoding="utf-8")
    weather_visuals = cards + weather_driver
    styles = (ROOT / "src" / "webserver" / "application" / "styles.ts").read_text(encoding="utf-8")
    subpages = (ROOT / "components" / "espcontrol" / "button_grid_subpages.h").read_text(encoding="utf-8")
    weather_forecast = BUTTON_GRID_WEATHER_FORECAST.read_text(encoding="utf-8")
    controls = (ROOT / "src" / "webserver" / "application" / "controls_fields.ts").read_text(encoding="utf-8")
    assert "sp-type-badge" not in styles + controls, "web previews should omit card-type badges"
    assert "set_weather_card_badge" not in weather_visuals, (
        "device weather cards should not show the hidden web preview type badge"
    )
    assert 'set_weather_card_badge(s, "Weather Cloudy")' not in weather_visuals, (
        "current weather device card should not render a visible weather badge"
    )
    assert 'lv_label_set_display_text(slot.text_lbl, espcontrol_i18n("Cloudy"))' in weather_driver, (
        "current weather device card should render the same label as the web preview"
    )
    assert 'set_weather_card_badge(s, "Weather Partly Cloudy")' not in weather_visuals, (
        "forecast weather device card should not render a visible forecast badge"
    )
    assert '"HA Actions"' not in weather_forecast, (
        "forecast weather errors should keep the configured/default label like the web preview"
    )
    assert 'lv_label_set_display_text(slot.unit_lbl, display_temperature_unit_symbol())' in weather_driver, (
        "forecast weather placeholder should show the configured unit like the web preview"
    )
    assert 'lv_label_set_display_text(ref.unit_lbl, normalized_unit.c_str())' in weather_forecast, (
        "forecast weather unavailable state should keep showing the configured unit"
    )
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    setup_start = grid.find("inline void setup_card_visual")
    setup_end = grid.find("inline bool bind_basic_sensor_card", setup_start)
    setup_visual = grid[setup_start:setup_end] if setup_start >= 0 and setup_end >= 0 else ""
    assert (
        "inline void reset_card_slot_dynamic_children" in grid
        and "lv_obj_del(child);" in grid
        and "lv_obj_set_user_data(s.sensor_container, nullptr);" in grid
        and "lv_obj_clear_state(s.btn, LV_STATE_CHECKED);" in grid
        and "lv_obj_clear_state(s.btn, LV_STATE_DISABLED);" in grid
        and "lv_obj_set_style_opa(s.btn, LV_OPA_COVER, LV_PART_MAIN);" in grid
        and "reset_card_slot_dynamic_children(s);" in setup_visual
    ), "weather cards must clear stale widget children, active states, and opacity before rendering"
    assert (
        "lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);" in setup_visual
        and "lv_obj_align(s.sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);" in setup_visual
        and "lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);" in setup_visual
    ), "weather cards must reset icons top-left and values/labels to their standard positions before rendering"
    assert "inline std::string normalize_weather_state" in weather_forecast, (
        "current weather device cards should normalize equivalent weather state spellings before mapping icons"
    )
    assert 'if (normalized == "partly-cloudy") return "partlycloudy";' in weather_forecast, (
        "current weather device cards should accept the dashed partly-cloudy spelling"
    )
    assert 'if (normalized.compare(0, 8, "weather-") == 0) normalized = normalized.substr(8);' in weather_forecast, (
        "current weather device cards should accept web weather icon names as state aliases"
    )
    assert 'if (normalized.compare(0, 4, "mdi-") == 0) normalized = normalized.substr(4);' in weather_forecast, (
        "current weather device cards should accept web Material Design weather class names as state aliases"
    )
    assert 'if (normalized == "night") return "clear-night";' in weather_forecast, (
        "current weather device cards should map the web Weather Night icon name to clear night"
    )
    assert 'normalized == "night-cloudy"' in weather_forecast and 'return "night-partly-cloudy";' in weather_forecast, (
        "current weather device cards should accept night cloudy aliases for the web weather icon"
    )
    assert 'normalized == "sunny-off"' in weather_forecast and 'return "unavailable";' in weather_forecast, (
        "current weather device cards should map the web unavailable weather icon name"
    )
    assert 'normalized == "unknown"' in weather_forecast and 'return "unavailable";' in weather_forecast, (
        "current weather device cards should render unknown states with the unavailable weather icon"
    )
    assert 'if (b.type == "weather" && !card_runtime_weather_forecast_precision(b.precision))' in subpages, (
        "subpage weather cards must normalize invalid weather modes like main grid cards"
    )
    for alias, state in (
        ("blizzard", "snowy-heavy"),
        ("broken-clouds", "cloudy"),
        ("clear", "sunny"),
        ("clear-day", "sunny"),
        ("drizzle", "rainy"),
        ("few-clouds", "partlycloudy"),
        ("foggy", "fog"),
        ("freezing-rain", "snowy-rainy"),
        ("heavy-rain", "pouring"),
        ("heavy-showers", "pouring"),
        ("heavy-snow", "snowy-heavy"),
        ("light-rain", "rainy"),
        ("mostly-clear", "sunny"),
        ("mostly-clear-night", "clear-night"),
        ("mostly-cloudy", "cloudy"),
        ("mostly-sunny", "sunny"),
        ("night-clear", "clear-night"),
        ("overcast", "cloudy"),
        ("partly-cloudy-day", "partlycloudy"),
        ("cloudy-night", "night-partly-cloudy"),
        ("few-clouds-night", "night-partly-cloudy"),
        ("mostly-cloudy-night", "night-partly-cloudy"),
        ("partly-cloudy-night", "night-partly-cloudy"),
        ("partly-sunny", "partlycloudy"),
        ("possibly-rainy-day", "rainy"),
        ("possibly-rainy-night", "rainy"),
        ("possibly-sleet-day", "snowy-rainy"),
        ("possibly-sleet-night", "snowy-rainy"),
        ("possibly-snow-day", "snowy"),
        ("possibly-snow-night", "snowy"),
        ("possibly-thunderstorm-day", "lightning-rainy"),
        ("possibly-thunderstorm-night", "lightning-rainy"),
        ("rain", "rainy"),
        ("sleet", "snowy-rainy"),
        ("snow", "snowy"),
        ("scattered-clouds", "cloudy"),
        ("showers", "rainy"),
        ("storm", "lightning"),
        ("stormy", "lightning"),
        ("thunderstorm", "lightning"),
        ("thunderstorms", "lightning"),
    ):
        assert f'if (normalized == "{alias}") return "{state}";' in weather_forecast or (
            f'normalized == "{alias}"' in weather_forecast and f'return "{state}";' in weather_forecast
        ), f"current weather device cards should normalize provider alias {alias} to {state}"
    for state, icon_name, label in (
        ("cloudy-alert", "Weather Cloudy Alert", "Cloudy Alert"),
        ("dust", "Weather Dust", "Dust"),
        ("hazy", "Weather Hazy", "Hazy"),
        ("hurricane", "Weather Hurricane", "Hurricane"),
        ("night-partly-cloudy", "Weather Night Cloudy", "Partly Cloudy Night"),
        ("partly-lightning", "Weather Partly Lightning", "Partly Lightning"),
        ("partly-rainy", "Weather Partly Rainy", "Partly Rainy"),
        ("partly-snowy", "Weather Partly Snowy", "Partly Snowy"),
        ("partly-snowy-rainy", "Weather Partly Snowy Rainy", "Partly Snow And Rain"),
        ("snowy-heavy", "Weather Snowy Heavy", "Heavy Snow"),
        ("sunny-alert", "Weather Sunny Alert", "Sunny Alert"),
        ("sunset", "Weather Sunset", "Sunset"),
        ("sunset-down", "Weather Sunset Down", "Sunset Down"),
        ("sunset-up", "Weather Sunset Up", "Sunset Up"),
        ("tornado", "Weather Tornado", "Tornado"),
    ):
        assert f'if (normalized == "{state}") return find_icon("{icon_name}");' in weather_forecast, (
            f"current weather device card should map {state} to the matching web weather icon"
        )
        assert f'if (normalized == "{state}") return espcontrol_i18n(std::string("{label}"));' in weather_forecast, (
            f"current weather device card should label {state} like the web preview"
        )


def test_weather_card_mode_visibility_reset() -> None:
    weather_driver = BUTTON_GRID_WEATHER_DRIVER.read_text(encoding="utf-8")
    match = re.search(
        r"inline bool weather_driver_setup_visual\([\s\S]*?\n\}",
        weather_driver,
    )
    assert match, "current weather setup is missing"
    body = match.group(0)
    assert "lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN)" in body, (
        "current weather cards must restore the icon after forecast mode hid it"
    )
    assert "lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN)" in body, (
        "current weather cards must hide the forecast sensor row"
    )


def test_grid_phase2_uses_cleaned_spanned_layout() -> None:
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    match = re.search(
        r"inline void grid_phase2\([\s\S]*?ESP_LOGI\(\"sensors\", \"Phase 2: done",
        grid,
    )
    assert match, "shared grid phase 2 is missing"
    body = match.group(0)
    assert "OrderResult parsed, order;" in body and "clear_spanned_cells(parsed, NS, COLS, order);" in body, (
        "phase 2 must bind weather/card state using the same cleaned spanned layout as the preview"
    )
    assert "int idx = order.positions[pos];" in body, (
        "phase 2 must skip grid cells covered by larger cards"
    )


def test_card_label_line_clamp_matches_preview_on_subpages() -> None:
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    assert "lv_obj_set_height(label, LV_SIZE_CONTENT);" in grid, (
        "short card labels must retain their natural height and bottom alignment"
    )
    assert "lv_obj_set_style_max_height(label, max_height, LV_PART_MAIN);" in grid, (
        "card labels must clip only after reaching the configured line limit"
    )
    assert "apply_card_label_line_clamp(back_slot.text_lbl, cfg, sp_ord.back_row_span);" in grid, (
        "subpage back labels must follow the configured line limit"
    )
    assert "refresh_card_layout(sub_slot, sb_cfg, cfg, rs, cs);" in grid, (
        "subpage card labels must follow the configured line limit before card-specific geometry is restored"
    )


def test_spanned_cards_refresh_after_clock_bar_padding_changes() -> None:
    clock_bar = (ROOT / "components" / "espcontrol" / "clock_bar.h").read_text(encoding="utf-8")
    layout = (ROOT / "components" / "espcontrol" / "button_grid_layout.h").read_text(encoding="utf-8")
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    assert "struct ClockBarResponsiveGridCard" in clock_bar, (
        "spanned card dimensions must be tracked outside the one-time grid placement pass"
    )
    assert "clock_bar_refresh_responsive_grid_cards();" in clock_bar, (
        "clock-bar padding changes must resize registered wide/tall/large cards"
    )
    assert "clock_bar_register_responsive_grid_card(" in layout, (
        "wide/tall/large cards must register their measured grid span"
    )
    assert "clock_bar_clear_responsive_grid_cards(main_page_obj);" in grid, (
        "main-grid refreshes must replace old responsive card registrations"
    )


def test_temperature_unit_changes_refresh_weather_cards() -> None:
    config = (ROOT / "components" / "espcontrol" / "button_grid_config.h").read_text(encoding="utf-8")
    match = re.search(
        r"inline void refresh_temperature_unit_labels\(\)[\s\S]*?\n\}",
        config,
    )
    assert match, "temperature unit label refresh helper is missing"
    body = match.group(0)
    assert "notify_dashboard_content_changed()" in body, (
        "temperature unit changes must refresh weather cards"
    )


def test_current_weather_state_keeps_normal_card_visuals() -> None:
    subscriptions = (ROOT / "components" / "espcontrol" / "button_grid_subscriptions.h").read_text(encoding="utf-8")
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    weather_driver = BUTTON_GRID_WEATHER_DRIVER.read_text(encoding="utf-8")
    match = re.search(
        r"inline void subscribe_weather_state\([\s\S]*?\n\}",
        subscriptions,
    )
    assert match, "current weather state subscription is missing"
    body = match.group(0)
    assert "apply_control_availability" not in body, (
        "current weather cards must not dim or disable themselves for unavailable entity states"
    )
    assert "notify_dashboard_content_changed()" in body, "current weather state changes must notify the dashboard"
    assert "uint32_t generation = ha_subscription_generation();" in body and "generation != ha_subscription_generation()" in body, (
        "current weather callbacks must ignore stale subscriptions after dashboard reconfiguration"
    )
    assert "bump_ha_subscription_generation();" in grid, (
        "dashboard reconfiguration must invalidate stale current weather subscriptions"
    )
    assert "weather_forecast_cancel_pending_requests();" in grid, (
        "dashboard reconfiguration must cancel stale weather forecast action responses"
    )
    assert grid.count("if (bind_basic_sensor_card(") >= 2, (
        "main-grid and subpage weather cards must use the same shared binding path"
    )
    assert (
        "if (weather_driver_shows_forecast(config)) return true;" in weather_driver
        and "subscribe_weather_state(slot.icon_lbl, slot.text_lbl, config.entity)" in weather_driver
    ), "subpage weather cards must use the same weather binding as main-grid weather cards"


def test_firmware_matrices(profile_slugs: list[str]) -> None:
    profiles = load_device_profiles()
    release = device_matrix.release_matrix(profiles)
    nightly = device_matrix.nightly_matrix(profiles)
    pr = device_matrix.pr_matrix(profiles)
    assert_profile_slugs(profile_slugs, [entry["slug"] for entry in release["include"]], "release matrix")
    assert_profile_slugs(profile_slugs, [entry["slug"] for entry in nightly["include"]], "nightly matrix")
    assert_profile_slugs(profile_slugs, [entry["slug"] for entry in pr["include"]], "PR matrix")


def test_public_firmware_slugs(profile_slugs: list[str]) -> None:
    assert sorted(profile_slugs) == check_public_firmware.load_slugs(ROOT / "devices" / "manifest.json")


def main() -> int:
    profiles = load_device_profiles()
    profile_slugs = list(profiles.keys())
    assert profile_slugs == compatibility_required_slugs(), "current compatibility device slug fixture is stale"
    test_public_device_capabilities(profile_slugs)
    test_generated_web(profiles)
    test_web_server_request_limits()
    test_s3_low_heap_policy()
    test_zero_image_capacity_disables_all_image_card_pickers(profiles)
    test_constrained_s3_supports_one_cover_art_card(profiles)
    test_generated_yaml(profiles)
    test_public_api_encryption_policy(profile_slugs)
    test_ota_preserves_deployed_partition_layouts()
    test_upgrades_do_not_reset_saved_panel_config()
    test_local_voice_generation_uses_capability()
    test_square_s3_reapplies_clock_bar_after_screen_changes()
    test_rotation_refresh_rebuilds_subpages()
    test_seven_inch_width_compensation_rotates_with_screen()
    test_subpage_config_changes_schedule_live_refresh()
    test_web_screen_aspect_matches_public_resolution()
    test_web_grid_spacing_matches_across_screen_sizes()
    test_setup_icon_glyphs()
    test_weather_card_visual_matches_preview()
    test_weather_card_mode_visibility_reset()
    test_grid_phase2_uses_cleaned_spanned_layout()
    test_card_label_line_clamp_matches_preview_on_subpages()
    test_spanned_cards_refresh_after_clock_bar_padding_changes()
    test_temperature_unit_changes_refresh_weather_cards()
    test_current_weather_state_keeps_normal_card_visuals()
    test_firmware_matrices(profile_slugs)
    test_public_firmware_slugs(profile_slugs)
    print("Device profile cross-checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
