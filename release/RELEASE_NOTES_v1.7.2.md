# Bambu SyKICK v1.7.2 — stable AMOLED release

Hardware: Waveshare ESP32-S3 Touch AMOLED 1.75 (466×466).
ESP-IDF: 6.0.2. The 2.8C LCD firmware stays at v1.6.2.

## Fix since v1.7.1

- Remove the top-center battery icon and percentage from **all** display pages, including Faces, Bambu, Live Camera, and menus.
- Hide both parts when each screen overlay is first created and whenever page visibility is updated. This also covers fresh devices with no saved settings.
- Keep battery monitoring, power management, and the short green USB charging effect.
- Restore the camera page's print-status note where the battery icon previously overlapped it.

## Install and rollback

- For a fresh Waveshare 1.75 AMOLED device, select **Stable v1.7.2 (1.75 AMOLED)** in the [web installer](https://skidazzit.github.io/Bambu-SyKICK/flash/).
- For devices already configured, update with `release/ota/printsphere_ota.bin` to keep Wi-Fi and printer settings. Factory flashing with `release/initial/printsphere_full.bin` may clear them.
- Stable v1.7.1 remains available under `release/initial/archive/printsphere_full-v1.7.1.bin` and `release/ota/archive/printsphere_ota-v1.7.1.bin`. Stable v1.7.0 is also retained as an older rollback option.
- Do not use AMOLED firmware on the 2.8C LCD device.

## Verification

- [Beta build and package checks passed](https://github.com/skidazzit/Bambu-SyKICK/actions/runs/40580926378).
- User confirmed the battery icon and percentage disappeared in the beta firmware on their ESP32-S3.
- Stable release binaries are built and published by the dedicated GitHub Actions workflow. Check that run's result before installing v1.7.2.
