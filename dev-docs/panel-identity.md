# Panel identity storage

The deployed 16 MB and 32 MB layouts both contain a 2 MiB `card_images`
partition. Configuration uses two slots at its beginning. The final 16 KiB
(offset `0x1fc000`) is reserved for panel identity; configuration slot bounds
exclude this region through `panel_flash_layout.h`.

Identity opens this region as an independent ESP-IDF NVS store using a persistent
partition descriptor and `nvs_flash_init_partition_ptr`. This avoids sharing the
space consumed by ESPHome preferences and does not change the partition table,
so existing panels can receive the change over OTA. Shared NVS is never erased.

On first use, a valid identity record from the original `espcontrol_id` namespace
is copied into the dedicated store. A dedicated record, including an empty name,
takes precedence thereafter. Devices without `card_images` retain shared NVS
support; unexpected layouts fail closed. Downgrading to the initial naming
implementation reads its older shared-NVS record, not the dedicated store.

The ESPHome startup adapter is pinned to 2026.8.2. Review its Application
StringRef lifetime assumptions, partition ownership and NVS API usage when
upgrading ESPHome.
