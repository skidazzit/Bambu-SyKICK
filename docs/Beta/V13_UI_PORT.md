# SyKICK v13 full UI port — beta branch

Branch: `beta/v13-menus`. Target: Waveshare ESP32-S3 Touch AMOLED 1.75 only.

## Source reference

The authoritative prior UI is the user's `SyKICK-Bambu-Bot-v13-PrintSphere-Dashboard-Mac.zip` (Oct 25), with `main/boards/orbit-face-amoled/sykick_display.cc`, `sykick_display.h`, and `face_options.h` inside its source archive. It is based on the earlier Xiaozhi/Orbit app and **cannot** be linked into this PrintSphere fork unchanged: the app, board, audio, settings, and UI lifecycles differ.

## Desired navigation

- Main face screen: same v13 eyes, colors, expressions, randomizer, idle face.
- Swipe up on face: Bambu printer dashboard. Keep PrintSphere printer pages, AMS, preview, camera, and controls.
- Swipe left/right: navigate face, printer dashboard, clock, and weekly weather as appropriate.
- Swipe down: Control Center (brightness slider, speaker, Wi-Fi, clock, weather, settings).
- No up/down swipe brightness changes.
- Settings: Face Options (Randomize/Mood/Single, main face, idle face, delay, nine colors), Display (brightness/idle dim), Sound, Time/Clock, Weekly Weather, Language, Wi-Fi, About.
- Preserve printer activity rules: stay bright during active/paused prints; restore wake brightness; wait 10 minutes after completed print before returning to idle faces.
- Persist settings in a namespaced NVS store without altering current printer profile configuration.

## Work gates

1. **Gesture isolation** — remove vertical brightness gesture without changing horizontal printer swipes or long-press portal PIN. Committed.
2. **Face view and state** — port v13 face geometry, animations, color/mood selection, idle/sleep behavior and persistent settings as independent module.
3. **Control Center** — overlay with touch targets and brightness slider, sound, Wi-Fi, clock and settings.
4. **Menus and pages** — port Face Options, Display, Time, Language, Wi-Fi, About, Weather; connect each setting to a real app function. Do not ship placeholders.
5. **Printer navigation** — add entry/exit between face and PrintSphere while retaining AMS, camera, progress and print controls.
6. **Firmware QA** — ESP-IDF 6.0.2 compile, memory budget, static checks, spare hardware run, config persistence, 60-minute test and OTA rollback.

## Safety

The public stable v1.6.2 binaries and public installer stay unchanged. New code remains on this beta branch until compilation and hardware validation pass. Do not flash this branch while the original beta 60-minute test is running.
