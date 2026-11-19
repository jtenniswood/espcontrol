function replaceOnce(source, original, replacement) {
  if (source.split(original).length !== 2) {
    throw new Error('ESP Web Tools changed its post-flash flow. Review the 7-inch V3 installer before upgrading.');
  }
  return source.replace(original, () => replacement);
}

// ESP Web Tools 10.4.0 has no hook for a reset that re-enumerates USB.
export function usbInstallerReset() {
  return {
    name: 'espcontrol-usb-installer-reset',
    config: () => ({ optimizeDeps: { exclude: ['esp-web-tools/dist/install-button.js'] } }),
    transform(source, id) {
      const modulePath = id.split('?', 1)[0];
      if (modulePath.endsWith('/esp-web-tools/dist/install-dialog.js')) {
        const finish = 'if (state.state === "finished" /* FlashStateType.FINISHED */) {';
        const next = 'this._state =\n                    supportsImprov && this._installErase';
        source = replaceOnce(source, finish, `${finish}
                if (state.restartedWithWatchdog && this._manifest.new_install_improv_wait_time === 0) {
                    // USB may return as a new port after the reset. Wi-Fi setup
                    // uses the panel hotspot, so serial access can end here.
                    this._client = null;
                    return;
                }`);
        return replaceOnce(source, next, `if (this.port.readable === null) {
                    this._closeDialog();
                    return;
                }
                ${next}`);
      }
      if (modulePath.endsWith('/esp-web-tools/dist/connect.js')) {
        return replaceOnce(source, '        port.close();', `        if (port.readable !== null || port.writable !== null) {
            port.close();
        }`);
      }
      if (!modulePath.endsWith('/esp-web-tools/dist/flash.js')) return;

      const reset = '    await hardResetDevice(transport, esploader);\n    console.log("DISCONNECT");';

      // A native USB core reset cannot re-sample BOOT after manual entry.
      // https://docs.espressif.com/projects/esptool/en/latest/esp32p4/troubleshooting.html#leaving-download-mode-in-usb-serial-jtag-mode
      source = replaceOnce(source, reset, `    const isSevenInchV3 = new URL(manifestPath, location.href).pathname
        .endsWith("/guition-esp32-p4-jc1060p470-v3/manifest.json");
    const chip = esploader.chip;
    const restartedWithWatchdog = isSevenInchV3 && chipFamily === "ESP32-P4" &&
        await chip.usesUsbJtagSerial(esploader) &&
        !(await esploader.readReg(chip.GPIO_STRAP_REG) & chip.GPIO_STRAP_SPI_BOOT_MASK);
    if (restartedWithWatchdog) {
        await chip.watchdogReset(esploader);
    } else {
        await hardResetDevice(transport, esploader);
    }
    console.log("DISCONNECT");`);
      return replaceOnce(source, '        message: "All done!",', `        message: "All done!",
        restartedWithWatchdog,`);
    },
  };
}
