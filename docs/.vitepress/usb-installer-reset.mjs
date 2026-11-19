// ESP Web Tools 10.4.0 has no post-flash reset hook. Adapt its successful
// write path at bundle time, while keeping its existing reset on errors.
export function usbInstallerReset() {
  return {
    name: 'espcontrol-usb-installer-reset',
    config: () => ({ optimizeDeps: { exclude: ['esp-web-tools/dist/install-button.js'] } }),
    transform(source, id) {
      if (!id.endsWith('/esp-web-tools/dist/flash.js')) return;

      const reset = '    await hardResetDevice(transport, esploader);\n    console.log("DISCONNECT");';
      if (source.split(reset).length !== 2) {
        throw new Error('ESP Web Tools changed its post-flash reset. Review the 7-inch V3 reset before upgrading.');
      }

      // A native USB core reset cannot re-sample BOOT after manual entry.
      // https://docs.espressif.com/projects/esptool/en/latest/esp32p4/troubleshooting.html#leaving-download-mode-in-usb-serial-jtag-mode
      return source.replace(reset, `    const isSevenInchV3 = new URL(manifestPath, location.href).pathname
        .endsWith("/guition-esp32-p4-jc1060p470-v3/manifest.json");
    const chip = esploader.chip;
    if (isSevenInchV3 && chipFamily === "ESP32-P4" &&
        await chip.usesUsbJtagSerial(esploader) &&
        !(await esploader.readReg(chip.GPIO_STRAP_REG) & chip.GPIO_STRAP_SPI_BOOT_MASK)) {
        await chip.watchdogReset(esploader);
    } else {
        await hardResetDevice(transport, esploader);
    }
    console.log("DISCONNECT");`);
    },
  };
}
