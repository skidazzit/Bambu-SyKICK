# Bambu SyKICK v1.7.0 — stable AMOLED release

Hardware: Waveshare ESP32-S3 Touch AMOLED 1.75 (466×466, ESP32-S3).
ESP-IDF: 6.0.2. This release does not replace the stable 2.8C LCD firmware (v1.6.2).

## New in this release

- Bambu SyKICK home face with 10% larger eyes and Look Around gaze animation. The eye in the direction of the gaze grows 35%.
- Main face and idle options support Look Around, including the Sad / Sleepy / Look Around idle cycle.
- Option C Control Center layout with Montserrat labels, quick brightness and volume sliders, sortable menu order, Wi-Fi, Clock, restart, and confirmed factory reset.
- Automatic Bambu dashboard on a new print, held while preparing, printing, or paused.
- Finished, failed, or stopped jobs keep the printer view for 10 minutes before returning to Faces.
- An upward swipe switches between Faces and printer dashboard. The downward swipe still opens Control Center. Horizontal swipes within the printer's page carousel remain available.
- The printer screen remains awake during the print and 10-minute post-print hold.

## Installation

- New device: use the Bambu SyKICK web installer and choose **Stable v1.7.0 (1.75 AMOLED)**.
- Existing device: use `release/ota/printsphere_ota.bin` via Web Config to retain Wi-Fi, printer, and display settings.
- Full `release/initial/printsphere_full.bin` image is only for factory USB flashing and can erase saved settings.
- Never flash the AMOLED image to the 2.8C LCD board.

## Verification and rollback

- The latest beta firmware compiled, packaged, and passed image header and SHA-256 checks in GitHub Actions run [40265261545](https://github.com/skidazzit/Bambu-SyKICK/actions/runs/40265261545).
- The final v1.7.0 stable build and image version check passed in [GitHub Actions run 40266824870](https://github.com/skidazzit/Bambu-SyKICK/actions/runs/40266824870).
- The earlier beta passed a 60-minute device test and OTA settings retention test. The newest print-focus and swipe changes had a passing build, but no separate device test was reported before this promotion.
- The former stable source is preserved on branch `stable/pre-v1.7.0`. Its v1.6.2 binaries are retained in the `release/initial/archive/` and `release/ota/archive/` folders.
