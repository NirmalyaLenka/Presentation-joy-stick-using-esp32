# ESP32 PowerPoint Remote

A wireless PowerPoint presentation remote built from an ESP32-WROOM-32 and a
cheap 2-axis analog joystick module. It pairs with Windows as a normal
Bluetooth keyboard, so no companion app or receiver dongle is needed - once
paired, it just works with PowerPoint (and honestly, with anything else that
responds to arrow keys and F5).

This is a from-scratch build. I looked for an existing repository close
enough to use as a base and didn't find one - see "Why not fork an existing
project" below for the repos I checked and why none of them fit. The
joystick-handling and debounce code here is original; the Bluetooth HID
layer is the well-established `T-vK/ESP32-BLE-Keyboard` library, run in
NimBLE mode.

## Hardware

| Component | Notes |
|---|---|
| ESP32-WROOM-32 DevKit V1 | 30 or 38-pin dev board. This is the board the wiring/pin choices below assume. |
| 2-axis analog joystick module | The common breakout with VRx, VRy, SW, +5V, GND pins (KY-023 or equivalent). |
| Power source | See "Power options" below - a USB power bank is the simplest safe choice; a LiPo + charger circuit is more compact but riskier to get wrong. |
| Enclosure (optional) | Not covered here - not tested. |

### Power options

**Option A - USB power bank (recommended if you just want it working):**
Any small USB power bank plus a USB-A to Micro-USB/USB-C cable (matching
your board's port) into the ESP32's onboard USB connector. The board's
onboard regulator handles everything. Zero risk of wiring mistakes.

**Option B - Integrated LiPo (more compact, more can go wrong):**
A 3.7V LiPo cell through a TP4056-based charging/protection module, then
through a low-dropout 3.3V regulator (e.g. AMS1117-3.3 or MCP1700-3302)
feeding the ESP32's **3V3 pin directly** (not VIN, not 5V).

> **Important:** Never connect a raw LiPo cell (up to 4.2V when full)
> directly to the ESP32's 3V3 pin or any GPIO - the chip's absolute maximum
> is 3.6V and you will damage it. Always go through a regulator. This
> wiring pattern is standard practice for battery-powered ESP32 builds, but
> I have not built and tested this specific charging circuit myself, so
> double-check your regulator and protection IC's datasheets before wiring
> it up. If in doubt, use Option A.

A power switch (SPDT slide switch), if used, should sit between the
regulator's output and the ESP32's 3V3 pin - not between the battery and
the charging module - so the battery can still charge over USB while the
remote itself is switched off.

## Wiring (Option A/B joystick wiring is identical either way)

| Joystick pin | ESP32 GPIO | Notes |
|---|---|---|
| VRx | GPIO34 | ADC1, input-only, no strapping function - safe default |
| VRy | GPIO35 | ADC1, input-only, no strapping function - safe default |
| SW  | GPIO32 | Digital input, internal pull-up enabled in firmware |
| VCC | 3V3 | |
| GND | GND | |

GPIO34/35 were chosen deliberately: they're ADC1-capable (ADC1 keeps
working correctly alongside Bluetooth; ADC2 can conflict with the radio),
they're input-only so there's no risk of accidentally driving them as
outputs, and they're not boot-strapping pins (unlike GPIO0, 2, 5, 12, 15),
so the joystick can't interfere with the board entering upload/boot mode.
GPIO32 was chosen for the button for the same "not a strapping pin, ADC1
group" reasoning, used here as a plain digital input.

## Controls

| Joystick action | Result |
|---|---|
| Push UP | Next slide |
| Push RIGHT | Next slide |
| Push DOWN | Previous slide |
| Push LEFT | Previous slide |
| Press the stick button | Start Slide Show (F5) |

Internally, "next" always sends the Right Arrow key and "previous" always
sends the Left Arrow key, regardless of which joystick direction triggered
it. See the comment above `sendNextSlide()`/`sendPreviousSlide()` in the
firmware for why - short version: PowerPoint's own Up/Down arrow bindings
in Slide Show mode are the reverse of what this project wants (Up = go
back, Down = advance), so forwarding Up/Down literally would make the
joystick behave backwards on the vertical axis. Left/Right sidesteps that.

If your joystick module happens to be wired or oriented so that UP reads
as a *low* voltage instead of high (this varies by module), swap the two
`if (yEvent == ...)` lines in `handleJoystick()` - no rewiring needed.

## Installation

1. Install the ESP32 board package in Arduino IDE (Boards Manager -> search
   "esp32" -> install the package by Espressif Systems).
2. Install the `ESP32 BLE Keyboard` library by T-vK (Library Manager, or
   download the zip from its GitHub releases page and add it via
   Sketch -> Include Library -> Add .ZIP Library).
3. Install the `NimBLE-Arduino` library by h2zero (Library Manager).
4. Open the installed `ESP32-BLE-Keyboard` library's `BleKeyboard.h` file
   and uncomment the line `#define USE_NIMBLE` near the top. **This step
   is required** - see "Why NimBLE mode" below.
5. Open `src/ESP32_PowerPoint_Remote.ino` in Arduino IDE.
6. Tools -> Board -> select "ESP32 Dev Module".
7. Tools -> Upload Speed -> 921600 (drop to 115200 if you get upload
   errors on a particular USB cable/adapter).
8. Connect the ESP32, select the correct port, and upload.

Full board-config details are in "Library + Software Setup" below.

### Why NimBLE mode

`T-vK/ESP32-BLE-Keyboard`'s last tagged release is v0.3.0 (Sep 2021), and
its default (non-NimBLE) Bluetooth backend does not compile against
current ESP32 Arduino core versions (3.x) - `BLEDevice::init()` and
`BLECharacteristic::setValue()` changed their expected argument types
(`String` vs `std::string`) since then, and the library's default path was
never updated to match. This is a known, still-open issue in the
library's issue tracker (issues #270, #291, #312, #313), not something I'm
guessing at.

The library's NimBLE mode swaps in the independently-maintained
`NimBLE-Arduino` library (by h2zero) as the Bluetooth backend instead of
the ESP32 core's built-in BLE stack, which sidesteps the problem entirely
and also uses noticeably less RAM and flash. That's why step 4 above is
listed as required, not optional, and why the recommended board package
version below is chosen to pair cleanly with it.

## Library + Software Setup

| Item | Recommendation |
|---|---|
| Arduino IDE | 2.x (latest stable) |
| ESP32 board package | Latest 3.x release, with NimBLE mode enabled (see above). If you hit unrelated build issues on 3.x, board package v2.0.17 is the last widely-used 2.x release and works with the library in its default (non-NimBLE) mode too - but 3.x + NimBLE is the forward-compatible combination and what this project targets. |
| ESP32 BLE Keyboard library | T-vK/ESP32-BLE-Keyboard, v0.3.0, **NimBLE mode enabled** |
| NimBLE-Arduino library | h2zero/NimBLE-Arduino, latest 1.4.x |
| Board selection | Tools -> Board -> ESP32 Arduino -> "ESP32 Dev Module" |
| Upload speed | 921600 |
| Flash size | 4MB (32Mb) - default for most DevKit V1 boards |
| Partition scheme | "Default 4MB with spiffs" |
| PSRAM | Disabled (standard WROOM-32 DevKit boards don't have PSRAM) |

If you'd rather use PlatformIO, `platformio.ini` in this repo already sets
`-D USE_NIMBLE` and pulls in both libraries at pinned versions - no manual
editing of library source required in that workflow.

## Windows + PowerPoint Setup

1. On the ESP32, power it on - it starts BLE advertising immediately.
2. On Windows: Settings -> Bluetooth & devices -> Add device -> Bluetooth.
3. The device will appear as **"PPT Remote"** (or whatever you set
   `BLE_DEVICE_NAME` to in the firmware's configuration section).
4. Pair it. No PIN/passcode is required for this HID profile.
5. Open your presentation in PowerPoint.
6. Press the joystick button to start the slide show (F5), or start it
   normally from PowerPoint's Slide Show tab.
7. Use the joystick to move forward/back through slides as described
   under "Controls" above.

Windows treats the ESP32 as a generic Bluetooth keyboard - PowerPoint
receives the arrow-key and F5 presses exactly as if they came from a
physical keyboard, so no PowerPoint-side setting or add-in is involved.

## Project Structure

```
ESP32-PowerPoint-Remote/
├── src/
│   └── ESP32_PowerPoint_Remote.ino   # firmware (open this in Arduino IDE)
├── platformio.ini                    # optional PlatformIO build config
├── README.md
├── LICENSE
└── .gitignore
```

## Testing

1. **First boot test** - upload the sketch, open Serial Monitor at 115200
   baud, confirm you see "BLE advertising started."
2. **Bluetooth pairing test** - confirm "PPT Remote" appears in Windows'
   Bluetooth device list and pairs without errors.
3. **Keyboard HID test** - with a text editor focused on the PC, wiggle
   the joystick; you should NOT see arrow-key characters typed (arrow
   keys don't insert characters), but pressing the joystick button should
   have no visible effect either since F5 isn't a text key - use the
   PowerPoint test below to confirm keys are actually arriving.
4. **Joystick test** - open PowerPoint in edit mode (not slide show), move
   the joystick and watch the Serial Monitor log which action fired; each
   push in one direction should log exactly one action, not several.
5. **PowerPoint test** - start a slide show, confirm UP/RIGHT advances and
   DOWN/LEFT goes back, and that the button starts/restarts the show.
6. **Reconnection test** - turn off Bluetooth on the PC (or move out of
   range), then re-enable it; the remote should reconnect without needing
   to be re-paired or power-cycled.
7. **Battery/power test** - if using Option B wiring, verify with a
   multimeter that the ESP32's 3V3 pin reads a stable 3.3V before ever
   connecting the joystick or trusting it with your board.

## Troubleshooting

**ESP32 doesn't appear in Windows' Bluetooth list**
Confirm NimBLE mode is actually enabled (`#define USE_NIMBLE` uncommented
in `BleKeyboard.h`) and that the sketch uploaded without errors. Also
confirm you're looking for a *Bluetooth* device, not a Wi-Fi network -
this project only uses BLE, not Wi-Fi.

**Joystick moves randomly / phantom slide changes with the stick untouched**
Increase `ADC_DEADZONE` and/or `ADC_TRIGGER_MARGIN` in the firmware's
configuration section. Cheap joystick potentiometers can have a noisy or
off-center rest position; the averaging (`ADC_SAMPLES`) and hysteresis
built into the code should handle typical noise, but very worn or
low-quality modules may need a wider deadzone.

**Multiple slides get skipped from one push**
Increase `SLIDE_CHANGE_COOLDOWN_MS`. The hysteresis logic already requires
the stick to return to center before it can fire again, so this shouldn't
normally happen - if it does, it points to unusually noisy ADC readings
oscillating across both thresholds within one sample.

**Bluetooth keeps disconnecting**
Check the power supply - brownouts during BLE transmission are a common
cause of drops on boards powered from a weak or overloaded USB port/power
bank. Also try reducing distance/obstructions between the ESP32 and PC.

**Compilation errors mentioning `std::string` / `String` conversion in
`BleKeyboard.cpp`**
You're hitting the known non-NimBLE incompatibility described above under
"Why NimBLE mode" - enable NimBLE mode.

**PowerPoint not responding to key presses**
Confirm the ESP32 is actually paired *and connected* (not just paired -
Windows can show a device as paired but not currently connected). Also
confirm PowerPoint's window has focus; like any keyboard, this remote's
keystrokes go wherever Windows currently has focus, not specifically to
PowerPoint.

**ESP32 won't enter upload mode / upload fails**
Hold the BOOT button on the dev board while upload starts, or lower the
upload speed to 115200 in Tools -> Upload Speed if using a slower/longer
USB cable.

## Limitations / not tested

- The LiPo charging circuit under "Power options" describes standard
  practice but has not been built and verified by me on this specific
  hardware - test carefully before trusting it unattended.
- Only tested for Windows + PowerPoint pairing/behavior as described;
  other Bluetooth HID hosts (macOS, Linux, Keynote, Google Slides) were
  not part of this project's scope and aren't covered here.
- No enclosure/case design is included.
- Battery level reporting is hardcoded to 100% (`BLE_BATTERY_LEVEL`) - it
  does not reflect an actual measured charge level, since no
  battery-voltage sensing circuit is part of this build.

## Future improvements

- Real battery voltage sensing (via a second ADC pin and a resistor
  divider) reported through the HID battery-level characteristic.
- Deep-sleep on inactivity to extend battery life.
- A configurable "previous/next" swap without needing to re-flash.

## Why not fork an existing project

I searched for an ESP32 + joystick + BLE HID + PowerPoint remote and
compared the closest candidates I found (see below). None combined an
ESP32-WROOM-32 target, real 2-axis joystick input, and BLE *keyboard* HID
(as opposed to gamepad/joystick HID, which PowerPoint doesn't listen to)
in one working, current project, so I built this from the maintained
building block that does the actual Bluetooth-keyboard heavy lifting
(`T-vK/ESP32-BLE-Keyboard`) rather than force-fitting an unrelated repo.

## License

MIT - see [LICENSE](LICENSE).
