"""Declarative task definitions for the EspControl check suite.

This module is intentionally data-only.  The runner imports it, but validators and
generators do not depend on the runner and keep their existing entry points.
"""

from __future__ import annotations

from dataclasses import dataclass


PROFILES = ("product", "fast", "ci", "all", "release")
DOMAINS = ("product", "firmware", "web", "docs", "workflow")


@dataclass(frozen=True)
class Task:
    id: str
    commands: tuple[tuple[str, ...], ...]
    dependencies: tuple[str, ...] = ()
    profiles: tuple[str, ...] = ()
    domains: tuple[str, ...] = ()
    inputs: tuple[str, ...] = ()
    generated_inputs: tuple[str, ...] = ()
    cache: str = "never"
    parallel_safe: bool = False
    cache_env: tuple[str, ...] = ()
    cache_tools: tuple[str, ...] = ()
    cache_inputs: tuple[str, ...] = ()


def task(
    task_id: str,
    *commands: tuple[str, ...],
    dependencies: tuple[str, ...] = (),
    profiles: tuple[str, ...] = (),
    domains: tuple[str, ...] = (),
    inputs: tuple[str, ...] = (),
    generated_inputs: tuple[str, ...] = (),
    cache: str = "deterministic",
    parallel_safe: bool = False,
    cache_env: tuple[str, ...] = (),
    cache_tools: tuple[str, ...] = (),
    cache_inputs: tuple[str, ...] = (),
) -> Task:
    return Task(
        task_id,
        commands,
        dependencies,
        profiles,
        domains,
        inputs,
        generated_inputs,
        cache,
        parallel_safe,
        cache_env,
        cache_tools,
        cache_inputs,
    )


PRODUCT = ("product", "fast", "ci", "all", "release")
FAST = ("fast", "ci", "all")
CI = ("ci", "all")
RELEASE = ("release",)
MAINTAINER_DOCS = ("dev-docs/**", "DEVELOPERS.md", "README.md", "product/README.md")
WEB_SOURCE_HELPERS = ("scripts/web_source.js", "scripts/build_web_bundle.js", "scripts/web_modules.json")
WEB_BUNDLE_INPUTS = ("devices/**", "common/addon/time.yaml")
WEB_BUNDLE_BUILD_HELPERS = (
    "scripts/build.py",
    "scripts/check_timezones.py",
    "scripts/device_profiles.py",
    "scripts/product_schema.py",
)


# Declaration order is the stable tie-breaker used by the planner.
TASKS = (
    task("firmware-tests", ("cmake", "-E", "remove_directory", "build/tests/firmware"),
         ("cmake", "-S", "tests/firmware", "-B", "build/tests/firmware"),
         ("cmake", "--build", "build/tests/firmware"),
         ("ctest", "--test-dir", "build/tests/firmware", "--output-on-failure"), profiles=FAST,
         domains=("firmware",),
         inputs=(
             "tests/firmware/**",
             "components/espcontrol/button_grid_config_parser.h",
             "components/espcontrol/button_grid_subpages.h",
             "common/config/card_normalization_fixtures.json",
             "common/config/*_card_normalization_fixtures.json",
             "scripts/generate_saved_config_parser_test.py",
             "src/webserver/modules/config_codec.js",
         ), parallel_safe=True,
         cache="never"),
    task("web-unit", ("node", "--test", "tests/web/unit/**/*.test.js"), profiles=FAST,
         domains=("web",), inputs=("tests/web/unit/**", "src/webserver/**"), parallel_safe=True,
         cache_tools=("node",)),
    task("mutations", ("python3", "scripts/run_mutations.py"),
         domains=("firmware", "web"),
         inputs=("tests/mutations/**", "tests/firmware/**", "tests/web/unit/**", "scripts/run_mutations.py"),
         cache="never"),
    task("generated", ("python3", "scripts/build.py", "--check"), profiles=PRODUCT,
         domains=("product", "firmware", "web", "docs"), inputs=("common/**", "devices/**", "builds/**", "components/espcontrol/**", "src/webserver/**", "compatibility/**", "scripts/build.py", "scripts/build_web_bundle.js", "scripts/web_source.js", "scripts/web_modules.json"),
         generated_inputs=("components/espcontrol/*_generated.h", "docs/generated/**", "docs/public/**", "product/product_snapshot.json"),
         parallel_safe=True, cache="never"),
    task("device-manifest", ("python3", "scripts/check_device_manifest.py"),
         ("python3", "scripts/check_device_manifest.py", "--self-test"), profiles=FAST,
         domains=("product", "firmware"), inputs=("common/assets/**", "devices/**", "builds/**", "scripts/check_device_manifest.py"), parallel_safe=True),
    task("device-manifest-output", ("python3", "scripts/generate_device_manifest.py", "--check"), profiles=PRODUCT,
         domains=("product", "firmware", "docs"), inputs=("common/assets/**", "devices/catalog.json", "scripts/generate_device_manifest.py"),
         generated_inputs=("devices/manifest.json",), parallel_safe=True),
    task("product-schema", ("python3", "scripts/check_product_schema.py", "--self-test"),
         ("python3", "scripts/check_product_schema.py"), dependencies=("generated",), profiles=PRODUCT,
         domains=("product",), inputs=("common/config/**", "devices/**", "scripts/check_product_schema.py"), parallel_safe=True),
    task("product-snapshot", ("python3", "scripts/check_product_snapshot.py"), dependencies=("generated",), profiles=PRODUCT,
         domains=("product",), inputs=("common/config/**", "common/assets/**", "devices/**", "compatibility/**"),
         generated_inputs=("product/product_snapshot.json",), parallel_safe=True),
    task("local-artifacts", ("python3", "scripts/check_local_artifacts.py"),
         ("python3", "scripts/check_local_artifacts.py", "--self-test"), profiles=FAST,
         domains=("workflow",), inputs=("scripts/check_local_artifacts.py",), cache="never"),
    task("local-esphome", ("python3", "scripts/local_esphome.py", "--self-test"), profiles=FAST,
         domains=("firmware", "workflow"), inputs=("scripts/local_esphome.py",), cache="never"),
    task("dev-docs", ("python3", "scripts/check_dev_docs.py", "--check"), profiles=FAST,
         domains=("docs",), inputs=MAINTAINER_DOCS + (
             "scripts/check_dev_docs.py", "package.json", ".github/workflows/**",
             "common/config/card_contract.json", "src/webserver/types/**",
             "components/espcontrol/button_grid*.h",
         ), cache_inputs=(
             "common/**", "components/**", "compatibility/**", "devices/**",
             "docs/**", "product/**", "scripts/**", "src/**",
         ), parallel_safe=True),
    task("pr-process", ("python3", "scripts/check_pr_process.py", "--self-test"), profiles=FAST,
         domains=("workflow",), inputs=(".github/**", "scripts/check_pr_process.py"), cache="never"),
    task("pr-testing-guidance", ("python3", "scripts/pr_testing_guidance.py", "--self-test"), profiles=FAST,
         domains=("workflow",), inputs=(".github/**", "scripts/pr_testing_guidance.py"), cache="never"),
    task("config", ("node", "scripts/check_config_formats.js"), profiles=FAST,
         domains=("product", "web"), inputs=("common/config/**", "src/webserver/**", "compatibility/fixtures/product_compatibility.json", "scripts/check_config_formats.js") + WEB_SOURCE_HELPERS + WEB_BUNDLE_INPUTS,
         cache_inputs=WEB_BUNDLE_BUILD_HELPERS, parallel_safe=True),
    task("backup-contract", ("node", "scripts/check_backup_contract.js"), dependencies=("generated",), profiles=PRODUCT,
         domains=("product", "web", "firmware"), inputs=("compatibility/**", "common/config/**", "src/webserver/**", "components/**", "scripts/check_backup_contract.js") + WEB_SOURCE_HELPERS, parallel_safe=True),
    task("model-contract", ("node", "scripts/check_model_contract.js"), dependencies=("generated",), profiles=PRODUCT,
         domains=("product", "web"), inputs=("src/webserver/model/**", "src/webserver/contracts/**", "compatibility/fixtures/product_compatibility.json", "scripts/check_model_contract.js"), parallel_safe=True),
    task("memory-monitor", ("python3", "scripts/monitor_display_memory.py", "--self-test"), profiles=FAST,
         domains=("firmware",), inputs=("scripts/monitor_display_memory.py",), parallel_safe=True),
    task("cover-art-contract", ("python3", "scripts/check_cover_art_contract.py"), profiles=FAST,
         domains=("firmware",), inputs=("common/device/screen_cover_art.yaml", "components/espcontrol/cover_art.h", "components/artwork_image/artwork_image.cpp", "scripts/check_cover_art_contract.py"),
         parallel_safe=True, cache_tools=("c++",)),
    task("web-smoke", ("node", "scripts/check_web_smoke.js"),
         ("node", "scripts/check_web_migration_baseline.js"), dependencies=("generated", "device-manifest-output"), profiles=PRODUCT,
         domains=("web", "product"), inputs=("src/webserver/**", "scripts/check_web_smoke.js", "scripts/check_web_migration_baseline.js", "compatibility/fixtures/web_migration_baseline.json") + WEB_SOURCE_HELPERS, generated_inputs=("docs/public/webserver/**",), parallel_safe=True),
    task("types", ("npm", "exec", "--", "tsc", "--noEmit"), profiles=FAST,
         domains=("web",), inputs=("src/**/*.ts", "tsconfig.json", "package-lock.json"),
         parallel_safe=True, cache_tools=("node_modules/.bin/tsc",)),
    task("firmware-parser", ("python3", "scripts/check_firmware_parser.py"), dependencies=("device-slots",), profiles=FAST,
         domains=("firmware",), inputs=("components/**", "common/config/*_card_normalization_fixtures.json", "scripts/check_firmware_parser.py"),
         parallel_safe=True, cache_tools=("c++", "g++", "clang++")),
    task("firmware-modals", ("python3", "scripts/check_firmware_modals.py"),
         ("python3", "scripts/check_firmware_modals.py", "--self-test"), profiles=FAST,
         domains=("firmware",), inputs=("common/**", "components/**", "scripts/check_firmware_modals.py", "scripts/generate_device_slots.py"), parallel_safe=True),
    task("firmware-display-tokens", ("python3", "scripts/check_firmware_display_tokens.py"),
         ("python3", "scripts/check_firmware_display_tokens.py", "--self-test"), profiles=FAST,
         domains=("firmware",), inputs=("components/**", "scripts/check_firmware_display_tokens.py"), parallel_safe=True),
    task("firmware-ha-bindings", ("python3", "scripts/check_firmware_ha_bindings.py"),
         ("python3", "scripts/check_firmware_ha_bindings.py", "--self-test"), dependencies=("device-slots",), profiles=FAST,
         domains=("firmware",), inputs=("common/**", "components/**", "devices/**", "scripts/check_firmware_ha_bindings.py"), parallel_safe=True),
    task("firmware-card-runtime", ("python3", "scripts/check_firmware_card_runtime.py"),
         ("python3", "scripts/check_firmware_card_runtime.py", "--self-test"), dependencies=("generated",), profiles=PRODUCT,
         domains=("firmware", "product"), inputs=("components/**", "common/config/card_contract.json", "scripts/check_firmware_card_runtime.py"), parallel_safe=True),
    task("firmware-release", ("python3", "scripts/check_firmware_release.py"), profiles=FAST + RELEASE,
         domains=("firmware", "workflow"), inputs=("builds/**", "devices/**", ".github/esphome.env", ".github/workflows/release.yml", "scripts/check_firmware_release.py"), cache="never"),
    task("device-matrix", ("python3", "scripts/check_device_matrix.py"), profiles=FAST,
         domains=("firmware", "product"), inputs=("common/assets/**", "builds/**", "devices/**", "scripts/check_device_matrix.py"), parallel_safe=True),
    task("device-profiles", ("python3", "scripts/check_device_profiles.py"), dependencies=("generated", "device-slots"), profiles=PRODUCT,
         domains=("firmware", "product"), inputs=("common/**", "components/**", "devices/**", "src/webserver/**", "compatibility/**", "scripts/check_device_profiles.py"), generated_inputs=("docs/public/device-profiles.json", "docs/public/webserver/**", "docs/generated/screens/**"), parallel_safe=True),
    task("release-confidence", ("python3", "scripts/check_release_confidence.py"),
         dependencies=("generated", "device-manifest-output"), profiles=PRODUCT,
         domains=("product", "workflow"), inputs=("builds/**", "devices/**", "scripts/check_release_confidence.py"),
         generated_inputs=("devices/manifest.json", "docs/public/**", "docs/generated/**"), cache="never"),
    task("release-changelog", ("python3", "scripts/check_release_changelog.py"), profiles=FAST + RELEASE,
         domains=("docs", "workflow"), inputs=("docs/**", "scripts/check_release_changelog.py"), cache="never"),
    task("card-contract-outputs", ("python3", "scripts/check_card_contract_outputs.py"), dependencies=("generated",), profiles=PRODUCT,
         domains=("product", "firmware", "web", "docs"), inputs=("common/config/card_contract.json", "scripts/check_card_contract_outputs.py"), parallel_safe=True),
    task("device-slots", ("python3", "scripts/generate_device_slots.py", "--check"), profiles=PRODUCT,
         domains=("firmware", "product"), inputs=("common/assets/**", "devices/**", "scripts/generate_device_slots.py"), generated_inputs=("devices/*/packages.yaml", "devices/*/device/sensors.yaml"), parallel_safe=True),
    task("icon-groups", ("python3", "scripts/check_icon_groups.py"), dependencies=("generated",), profiles=FAST,
         domains=("firmware", "product", "docs"),
         inputs=("common/assets/**", "devices/**", "docs/.vitepress/theme/components/IconGallery.vue", "scripts/check_icon_groups.py"), parallel_safe=True),
    task("timezones", ("python3", "scripts/check_timezones.py"),
         ("python3", "scripts/check_timezones.py", "--self-test"), profiles=FAST,
         domains=("firmware", "web"), inputs=("common/**", "src/webserver/**", "components/espcontrol/sun_calc.h", "scripts/check_timezones.py"),
         parallel_safe=True, cache="never"),
    task("public-firmware-script", ("python3", "scripts/check_public_firmware.py", "--self-test"), profiles=PRODUCT,
         domains=("firmware", "workflow"), inputs=("scripts/**", "docs/public/**"), parallel_safe=True),
    task("web-browser-smoke", ("node", "scripts/check_web_browser_smoke.js"), dependencies=("generated", "device-manifest-output"), profiles=CI,
         domains=("web",), inputs=("src/webserver/**", "devices/**", "common/addon/time.yaml", "scripts/check_web_browser_smoke.js", "package-lock.json") + WEB_SOURCE_HELPERS,
         generated_inputs=("docs/public/webserver/**",),
         cache_env=("PLAYWRIGHT_BROWSERS_PATH", "PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD")),
    task("docs-build", ("npm", "run", "docs:build"), dependencies=("generated",), profiles=("all", "release"),
         domains=("docs",), inputs=("docs/**",) + MAINTAINER_DOCS + ("package-lock.json",),
         generated_inputs=("docs/generated/**",), cache="never"),
)
