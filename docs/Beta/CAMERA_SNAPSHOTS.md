# Beta camera snapshots — Waveshare ESP32-S3 1.75" AMOLED

This beta shows a camera still on the final Bambu printer page. While a print is running, it requests a fresh image about every 3 seconds (network and decode time add to this). When the print is idle, tap the camera page to refresh by hand.

## P2S setup

The P2S provides an RTSP stream. ESP32 firmware fetches **HTTP JPEG snapshots**, so it needs a local bridge such as go2rtc:

1. Keep the P2S camera stream connected to go2rtc.
2. Check that your bridge can return a JPEG: `http://GO2RTC-HOST:1984/api/frame.jpeg?src=bambu_camera&width=400&height=224`
3. Open **Bambu SyKICK Web Config → Device Settings → Camera Snapshots**.
4. Paste your own full JPEG URL; use **Hybrid** or **Local** connection mode and save. The device restarts.
5. Start a print and open the **last printer page**. New stills should appear every ~3 seconds.

The URL stays in device settings (NVS) and is not stored in GitHub. Keep go2rtc on your trusted LAN. The firmware requires plain HTTP and does not support RTSP directly.

## Notes

- Frames are decoded to RGB565 and fit within 400×224 without stretching or upscaling. Smaller frames remain centered.
- Verified sizing targets: 240×240 → 224×224, 320×240 → 296×224, 400×224 unchanged, 1920×1080 → 392×224.
- The JPEG scaler supports output sizes divisible by 8, with no more than 8× downscaling. Use go2rtc to resize very large source images, such as 4K.
- The bottom camera-page control replaces layer progress. Tap **LIGHT ON / LIGHT OFF** to switch the printer chamber light using the current Bambu connection; unsupported models show LIGHT N/A.
- Frames are requested only while that page is open. Fetches repeat during active prints; tap to request one when idle.
- A failed fetch keeps the most recent frame and reports a camera error.
- The native JPEG camera path remains for older printer models without a custom URL.
- Stable builds are unchanged. Use the beta workflow artifacts for test files.

## Device test checklist

- [ ] JPEG URL saves and survives reboot.
- [ ] Last printer page shows a photo during a print.
- [ ] Try 240×240, 320×240, 400×224, and 1920×1080 JPEGs.
- [ ] Light button toggles ON/OFF and reports the current state.
- [ ] Image changes roughly every 3 seconds.
- [ ] Leaving the camera page stops requests.
- [ ] Idle state stops automatic requests; tap still works.
- [ ] Wi-Fi outage shows a useful state and restores after reconnect.
- [ ] Existing stable firmware remains available.
