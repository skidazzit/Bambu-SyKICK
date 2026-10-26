# Bambu SyKICK 1.75 AMOLED beta validation (historical)

> This document records an earlier beta image. The current 1.75 AMOLED stable release is [v1.7.0](../../release/RELEASE_NOTES_v1.7.0.md). The test artifact and safety notes below apply only to that older beta.

Build: `v1.6.2-sykick-beta.1`  
Target: Waveshare ESP32-S3 Touch AMOLED 1.75 (not the 2.8-inch LCD)  
ESP-IDF: `v6.0.2`  
Source commit: `3eca0c6628c4a261236891cc52f9660317f87e11`

## Download

[Successful build and test artifacts](https://github.com/skidazzit/Bambu-SyKICK/actions/runs/40215007783/artifacts/12607148590)

GitHub retains this workflow artifact for 30 days (through November 24, 2026). It contains:

| File | Size | SHA-256 |
| --- | ---: | --- |
| `printsphere_full.bin` | 4,257,008 bytes | `fdb4011d1b26d15456cb555e4f65e75712bad4ade898e1ce756d6844a85e6986` |
| `printsphere_ota.bin` | 3,142,896 bytes | `a9243442649253797322c92ebe888454933f79e3d1919a065ee5635a2c563ca0` |

The ZIP also includes `SHA256SUMS`. The image header and checksum checks passed in GitHub Actions. The binary reports `v1.6.2-sykick-beta.1`.

## Safety status

**Not yet approved for installer use.** Physical-device validation has not been done. Existing stable v1.6.2 binaries are untouched. This beta is not a replacement for stable firmware.

For initial hardware testing, use a spare 1.75-inch AMOLED ESP32-S3, preserve its current firmware/settings backup, and confirm the correct serial port. A factory image writes from flash offset `0x0` and can erase configuration. Do not flash the full image to a device you must keep configured.

## Manual hardware acceptance checklist

- [ ] Back up the spare device and verify backup size/hash.
- [ ] Confirm connected board is Waveshare ESP32-S3 Touch AMOLED 1.75.
- [ ] Flash the **factory** image on the spare board only.
- [ ] Cold boot, then verify no reset loop during a 60-minute idle test.
- [ ] Confirm Bambu SyKICK splash and `Bambu-SyKICK-Setup` hotspot.
- [ ] Test touch, display orientation, brightness, Wi-Fi setup, and saved settings across reboot.
- [ ] Test Bambu printer connection, print status, and screen updates.
- [ ] Use OTA image on the already-configured spare device, then verify settings stay saved.
- [ ] Confirm clean rollback to the known working firmware.
- [ ] Record device logs and approve the beta before adding public installer manifests.

Never mix 1.75-inch AMOLED and 2.8-inch LCD firmware. The beta remains locked in the public web installer until this checklist passes.
