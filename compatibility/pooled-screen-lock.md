# Screen Lock card projection

The pooled grid supports the Screen Lock card using the shared touchscreen PIN
state. Locked/unlocked icons, translated captions, checked styling and press
feedback reuse the fixed visible widget bank. Home and subpage cards need no
per-card state registry, subscription or timer.

Preparation requires both a shared-state reader and a local intent callback.
Paint reads the shared state; the grid's state-change poll observes lock/unlock
even when HA has not changed. Input admits a full binding and the saved
`screensaver_on_lock` flag. It does not create a PIN overlay, activate a
screensaver or send a service directly. The application scene must queue that
intent, then recheck the current binding, display mode and PIN configuration in
maintenance before applying the existing lock behavior.

The input remains usable offline. Missing presses, lost presses, navigation,
configuration replacement and display suspension cannot reuse an earlier
binding. Configured labels/icons remain ignored as in the existing Screen Lock
card. The shared saved-configuration normalizers remain unchanged: the existing
subpage normalizer strips `screensaver_on_lock`, so this stage does not enable
that option on subpages or change stored documents.

The prepared definition reuses existing union storage. Host and actual device
size checks distinguish metadata size from whole-device memory. Native LVGL
coverage cycles shared state and navigation 1,000 times per profile, checks
translated labels/icons and stale input, and counts retained resources.

Production scene wiring and compiled legacy retirement remain unfinished. After
integration, qualify the Guition 4848S040 S3 and a working 20-slot JC8012P4A1 P4
with PIN enabled/disabled, local and remote lock/unlock, optional screensaver,
HA reconnects and configuration replacement. Keep P4-86 reserved for USB
diagnosis. No physical memory saving is claimed by this stage.
