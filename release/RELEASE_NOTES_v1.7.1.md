# Bambu SyKICK v1.7.1 — stable AMOLED release

Hardware: Waveshare ESP32-S3 Touch AMOLED 1.75 (466×466, ESP32-S3).
ESP-IDF: 6.0.2. The 2.8C LCD firmware remains on v1.6.2.

## Changes since v1.7.0

- Faces are 20% larger than in v1.7.0; the Look Around gaze and eye-growth animation remain intact.
- Removed the redundant **Faces Home** link from Control Center and Sort Menu. Faces are still opened by the existing screen gesture.
- Sort Menu has five movable entries. Upgrading preserves the relative order of existing items saved in the old six-item menu.
- Printer camera snapshots refresh automatically every second on the live-view page.
- The camera image path accepts 240×240, 320×240, 400×224, and larger JPEGs scaled to the display, with safer decode bounds.
- The camera page includes a printer light On/Off control.
- New prints bring the printer dashboard forward. The dashboard returns to Faces five minutes after the print ends or fails, or after five minutes without a print when opened manually.

## Install and rollback

- Choose **Stable v1.7.1 (1.75 AMOLED)** in the Bambu SyKICK web installer for a new device.
- To update a configured device, use the v1.7.1 OTA image from `release/ota/printsphere_ota.bin`. OTA normally retains Wi-Fi, printer profiles, and display settings.
- Factory flashing uses `release/initial/printsphere_full.bin` and may clear saved settings.
- The earlier stable v1.7.0 full and OTA images remain at `release/initial/archive/printsphere_full-v1.7.0.bin` and `release/ota/archive/printsphere_ota-v1.7.0.bin`. The web installer provides a rollback choice.
- Do not flash the 1.75 AMOLED firmware onto the 2.8C LCD device.

## Verification

- Beta source build, image header, and checksum checks passed in [GitHub Actions run 40427084302](https://github.com/skidazzit/Bambu-SyKICK/actions/runs/40427084302).
- Stable v1.7.1 image build and release checks are recorded in [GitHub Actions run 40434349160](https://github.com/skidazzit/Bambu-SyKICK/actions/runs/40434349160).
- The user confirmed the latest menu changes passed testing on the ESP32-S3. Live camera auto-refresh was also reported working.
- The release is limited to the Waveshare 1.75 AMOLED hardware target.
