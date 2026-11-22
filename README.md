# ESP32 PowerPoint Remote

A Bluetooth Low Energy (BLE) HID keyboard built around an ESP32-WROOM-32 and a two-axis analog joystick. The recommended firmware lets a Windows host treat the board as a keyboard, so it can send presentation-navigation keystrokes without a companion PC application.

**Recommended sketch:** [`optimized_presentation_remote.ino`](optimized_presentation_remote.ino). The repository also contains an earlier root-level sketch and pre-production examples; see [Repository contents](#repository-contents).

## Features

- BLE keyboard device named `PPT Remote`.
- Analog joystick navigation with averaged ADC readings, a dead zone, hysteresis/re-arm behavior, and a slide-change cooldown.
- Joystick push-button sends `F5` to start a slide show.
- The optimized sketch advertises for up to two minutes while waiting for pairing, remains active while connected, and waits up to two minutes for reconnection after disconnecting.
- After the pairing/reconnection window expires, the optimized sketch enters deep sleep. The configured wake path is the board's physical EN/RESET button or a power cycle.

## Hardware and wiring

Use an **ESP32-WROOM-32 / original ESP32 DevKit V1** with the GPIOs shown below. Do not assume the same pin mapping works on ESP32-C3, ESP32-S3, or other board variants.

| Joystick signal | ESP32 connection |
|---|---|
| VRx | GPIO34 |
| VRy | GPIO35 |
| SW (push-button) | GPIO32 |
| VCC | 3V3 |
| GND | GND |

Power the joystick from **3.3 V, not 5 V**. Its analog outputs must stay within the ESP32's input-voltage limits. The push-button is configured as active-low with the ESP32's internal pull-up.

## Controls

| Joystick action | Keystroke sent by the recommended sketch |
|---|---|
| Push UP or RIGHT | Right Arrow (next slide) |
| Push DOWN or LEFT | Left Arrow (previous slide) |
| Press joystick button | F5 (start slide show) |

Joystick orientation varies by module. If vertical directions are reversed, the sketch comments identify the two `yEvent` action lines that can be swapped. Do not change wiring and firmware mapping at the same time.

## Software prerequisites

- Arduino IDE.
- Espressif ESP32 board package.
- T-vK's **ESP32-BLE-Keyboard** library.
- **NimBLE-Arduino** library.

The current README setup uses NimBLE mode because the BLE Keyboard library's default backend may not compile with the selected ESP32 Arduino core. In the installed library's `BleKeyboard.h`, follow the library's documented method to enable `USE_NIMBLE`. This is a change to a third-party library file and may need to be repeated if that library is updated.

## Build and upload

1. Connect the ESP32 by USB.
2. Install the board package and libraries listed above.
3. Open `optimized_presentation_remote.ino` from this repository. Do not select a sketch from the pre-production folder for the normal build.
4. Select **ESP32 Dev Module** (for the matching ESP32-WROOM-32 board) and the correct serial port.
5. Start with an upload speed of 115200 if a faster upload is unreliable; the original project instructions also describe trying 921600 when supported by the board and USB connection.
6. Upload, then open Serial Monitor at **115200 baud** for startup, connection, action, and sleep messages.

## Safe first run

This device sends real keyboard events to the connected computer. Test it deliberately:

1. Close unsaved documents and avoid pairing while an important application or form has focus.
2. Pair the board, then open a blank Notepad window first.
3. Move the joystick in each direction and press the joystick button to confirm which keys are sent. The button sends F5; in the wrong focused application, that key may do something other than start a PowerPoint slide show.
4. Only then open the intended presentation and test navigation.
5. If the optimized sketch sleeps after its two-minute pairing/reconnect window, press the board's physical EN/RESET button or power-cycle it. The joystick button is not a wake button.

## Pairing and use

1. Reset or power the board. It advertises as **PPT Remote** for up to two minutes.
2. On Windows, open **Settings → Bluetooth & devices → Add device → Bluetooth**, then select **PPT Remote**.
3. Open a presentation and start the slide show. The joystick's push-button sends F5; joystick movement sends arrow-key events.
4. If Bluetooth disconnects, reconnect within the two-minute window to keep the optimized sketch awake.

## Configuration

The configuration block near the top of `optimized_presentation_remote.ino` contains:

- `PIN_JOY_VRX`, `PIN_JOY_VRY`, `PIN_JOY_SW` — GPIO assignments.
- `BLE_DEVICE_NAME` and `BLE_MANUFACTURER` — BLE identity.
- `ADC_DEADZONE`, `ADC_TRIGGER_MARGIN`, `ADC_SAMPLES` — input filtering and trigger thresholds.
- `SLIDE_CHANGE_COOLDOWN_MS` and `BUTTON_DEBOUNCE_MS` — timing.
- `PAIRING_WINDOW_MS` — pairing/reconnect timeout.
- `BLE_BATTERY_LEVEL` — reported battery percentage; this value is hardcoded and is **not** a measurement of a real battery.

## Troubleshooting

| Symptom | Checks |
|---|---|
| Device does not appear in Bluetooth settings | Confirm upload succeeded, the board reset, and NimBLE mode is enabled as required by the installed BLE Keyboard library |
| Random or phantom slide changes | Check joystick wiring and center voltage; increase `ADC_DEADZONE` or `ADC_TRIGGER_MARGIN` cautiously |
| More than one action per push | Increase `SLIDE_CHANGE_COOLDOWN_MS` and confirm the joystick returns to its neutral region |
| Vertical directions are reversed | Check joystick orientation and the `yEvent` mapping in `handleJoystick()` |
| Compilation fails in `BleKeyboard.cpp` around `String`/`std::string` | Recheck the library's NimBLE configuration and the selected board-core/library versions |
| Upload fails or reports a checksum error | Try a known-good data USB cable, a different port, a lower upload speed, and close any serial terminal using the port |
| Device does not wake from deep sleep | Use the board's EN/RESET button or power-cycle it; the joystick does not wake the optimized sketch |

## Limitations and verification

- The optimized sketch reports a fixed battery level; it does not measure battery voltage or charge.
- The repository documents Windows and PowerPoint as the target host. Compatibility with other operating systems, presentation apps, or Bluetooth stacks has not been independently verified in this maintenance pass.
- No compilation, pairing, keystroke, or hardware test was run as part of this documentation update.

## Repository contents

| Path | Role |
|---|---|
| [`optimized_presentation_remote.ino`](optimized_presentation_remote.ino) | Recommended BLE keyboard firmware with pairing/reconnect timeout and deep sleep |
| [`ESP32_PowerPoint_Remote.ino`](ESP32_PowerPoint_Remote.ino) | Earlier root-level BLE keyboard variant; use the recommended sketch unless you intentionally need to compare versions |
| [`pre production codes just for your info dont impliment/`](pre%20production%20codes%20just%20for%20your%20info%20dont%20impliment/) | Older candidate sketches and `joystick_raw_test.ino`; not the normal upload target |
| [`index.svg`](index.svg) | SVG project graphic |
| [`LICENSE (8)`](LICENSE%20(8)) | License text; the nonstandard filename may prevent GitHub's automatic license detection |
| [`README.md`](README.md) | This guide |

The image references from the original project are retained below.

<img width="1420" height="1420" alt="ESP32 PowerPoint remote assembly photo" src="https://github.com/user-attachments/assets/f3e16d0e-ec48-4c38-9554-0b52e99a9a26" />

<img width="1479" height="1479" alt="ESP32 PowerPoint remote wiring photo" src="https://github.com/user-attachments/assets/23a7b652-4e4a-491a-bb8a-4f1ce24c1784" />

## License

See [`LICENSE (8)`](LICENSE%20(8)).

---

## Maintenance & Audit

- **Audit date:** 11 November 2026
- **Documentation status:** README reviewed and audit record added on this date.
- **Scope and evidence:** This is a documentation-level maintenance pass. It does not establish that source code compiles, dependencies are current, security has been tested, or hardware/runtime behavior has been verified.
- **Code and tests:** No new functional code changes or test results are claimed by this README update. Run project-specific build, test, and hardware/runtime checks before relying on the project.
- **Maintenance baseline:** Documentation status recorded through 11 November 2026; revisit setup instructions, dependencies, safety limits, known limitations, and tests when the project changes.
