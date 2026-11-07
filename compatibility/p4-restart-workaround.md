# P4 restart cache synchronization

The pinned ESP-IDF 5.5.6 restart path resets the other CPU before stalling it.
Resetting that CPU during cache synchronization can make the following cache
writeback fault. A retained P4 panic decoded against its exact firmware ELF
reaches `Cache_WriteBack_All` from `esp_restart_noos_inner`, with a store-access
fault at the cache synchronization registers. Espressif reproduced this race in
[issue 19171](https://github.com/espressif/esp-idf/issues/19171#issuecomment-6075256928).

P4 code generation wraps `esp_restart_noos` with the SDK's version-bound reset
path. It stalls the other core and waits for outstanding synchronization before
writeback. It retains the one-second RTC reset watchdog, timer watchdog handling,
external-stack switch, cache/remap shutdown, peripheral reset and final CPU
resets. It does not change OTA validation, rollback or configuration storage.
S3 does not link the wrapper. The version guard requires review when ESP-IDF
changes, so an upstream fix can replace this local backport.

Host tests exercise both CPU origins, pending cache operations, external stacks
and watchdog/platform variants, and reject the preceding reset-before-stall
sequence. Target builds must verify IRAM placement and the actual call from
`esp_restart` into the wrapper. Physical restart/OTA stress and original-dashboard
preservation remain separate acceptance checks; compilation and host models do
not establish that the hardware race is fixed.
