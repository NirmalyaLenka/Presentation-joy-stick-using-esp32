# ESP32 PowerPoint Remote
 
A wireless PowerPoint presentation remote built from an ESP32-WROOM-32 and a
2-axis analog joystick (HW-504 or equivalent). It pairs with Windows as a
normal Bluetooth keyboard - no companion app, dongle, or PowerPoint add-in
needed.
 
Firmware file: `optimized_presentation_remote.ino`
 # please use the 'optimized_presentation_remote.ino' file 
## Hardware
 
| Component | Notes |
|---|---|
| ESP32-WROOM-32 DevKit V1 | 30 or 38-pin dev board |
| HW-504 (or equivalent) 2-axis analog joystick | VRx, VRy, SW, VCC, GND pins |
| Power source | USB power bank, or a properly regulated LiPo setup |
 
## Wiring
 
| Joystick pin | ESP32 GPIO |
|---|---|
| VRx | GPIO34 |
| VRy | GPIO35 |
| SW  | GPIO32 |
| VCC | 3V3 (not 5V) |
| GND | GND |
 
Power the joystick from **3V3**, not 5V. At 5V supply the VRx/VRy outputs
can swing close to 5V at full deflection, which exceeds the ESP32 ADC
pins' safe input voltage. Running the module from 3.3V keeps it safely
within range and the joystick works identically.
 
GPIO34/35/32 were chosen because they're ADC1-capable (unaffected by
Bluetooth radio activity, unlike ADC2), input-safe, and not
boot-strapping pins.
 
## Controls
 
| Action | Result |
|---|---|
| Push joystick UP | Next slide |
| Push joystick RIGHT | Next slide |
| Push joystick DOWN | Previous slide |
| Push joystick LEFT | Previous slide |
| Press joystick button | Start Slide Show (F5) |
 
Internally, "next" always sends the Right Arrow key and "previous" always
sends the Left Arrow key, regardless of which physical direction
triggered it - PowerPoint's native Up/Down arrow bindings in Slide Show
mode are reversed from what this project wants, so Left/Right is used
for reliability instead.
 
If UP and DOWN feel swapped on your specific joystick, swap the two
`yEvent` lines in `handleJoystick()` rather than rewiring anything.
 
## Power behavior
 
Power is managed by pairing state, not by idle time:
 
1. Pressing the board's physical **EN/RESET** button boots the ESP32,
   which advertises over Bluetooth for up to 2 minutes
   (`PAIRING_WINDOW_MS`).
2. If a host connects within that window, the device stays fully active
   for as long as it stays connected - no timeout while connected.
3. If nothing connects within 2 minutes, it deep-sleeps.
4. If it was connected and then disconnects, it waits up to 2 minutes for
   a reconnect before sleeping. Reconnecting in time cancels the timeout.
5. Once asleep, the **only** way back on is pressing the physical
   EN/RESET button (or a power cycle). No GPIO wake source is
   configured - a hardware reset already forces a reboot on its own.
The joystick's own push-button is unrelated to this state machine - it
only sends the Start Slide Show keystroke, same as always.
 
## Installation
 
1. Arduino IDE, Boards Manager -> install the `esp32` board package
   (Espressif Systems), latest 3.x.
2. Install the `ESP32 BLE Keyboard` library by T-vK (Library Manager, or
   add via .ZIP from its GitHub releases).
3. Install `NimBLE-Arduino` by h2zero (Library Manager).
4. Open the installed `ESP32-BLE-Keyboard` library's `BleKeyboard.h` and
   uncomment `#define USE_NIMBLE`. Required - the library's default
   (non-NimBLE) backend does not compile against current ESP32 core
   versions.
5. Open `optimized_presentation_remote.ino`.
6. Tools -> Board -> "ESP32 Dev Module".
7. Tools -> Upload Speed -> 921600 (drop to 115200 if uploads fail with a
   checksum error - a common symptom of a marginal USB cable/port at the
   higher speed).
8. Select the correct COM port and upload.
## Pairing
 
1. Power on (or press RESET). The device advertises as **"PPT Remote"**
   for 2 minutes.
2. Windows: Settings -> Bluetooth & devices -> Add device -> Bluetooth ->
   select "PPT Remote". No PIN required.
3. Open PowerPoint, press the joystick button to start the slide show
   (F5), and use the joystick to navigate.
## Testing
 
- **Serial Monitor** at 115200 baud shows connection state changes and
  every action fired ("Action: NEXT slide", "Powering off (...)", etc.) -
  useful for confirming behavior without needing PowerPoint open.
- Move the joystick to each extreme and back to center: expect exactly
  one action per push, not several.
- Disconnect Bluetooth on the PC and reconnect within 2 minutes to
  confirm the remote stays awake and reconnects.
- Leave it disconnected for over 2 minutes to confirm it deep-sleeps, and
  confirm RESET brings it back.
## Troubleshooting
 
**Doesn't appear in Windows' Bluetooth list** - confirm `USE_NIMBLE` is
enabled in `BleKeyboard.h` and the sketch uploaded without errors.
 
**Joystick fires randomly / phantom slide changes** - increase
`ADC_DEADZONE` and/or `ADC_TRIGGER_MARGIN` in the configuration section.
 
**Multiple slides skip on one push** - increase
`SLIDE_CHANGE_COOLDOWN_MS`.
 
**Compile errors mentioning `std::string`/`String` in `BleKeyboard.cpp`**
- NimBLE mode isn't enabled; see step 4 under Installation.
**Upload fails with "Bad data checksum"** - lower the upload speed to
115200, try a different USB cable/port, and close any program (like
Serial Monitor) holding the COM port open during upload.
 
**Device won't wake up** - only the physical EN/RESET button (or a power
cycle) wakes it from sleep, by design; nothing on the joystick will.
 
## Limitations
 
- Battery level reported over Bluetooth is hardcoded (`BLE_BATTERY_LEVEL`)
  and does not reflect a real measured charge.
- Only tested against Windows + PowerPoint; other Bluetooth HID hosts
  were not part of this project's scope.
## License
 
MIT.
 
