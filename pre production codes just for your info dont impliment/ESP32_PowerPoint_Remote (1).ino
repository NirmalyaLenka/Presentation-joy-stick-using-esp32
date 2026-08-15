/*
 * ESP32 PowerPoint Remote
 * ------------------------
 * Turns an ESP32-WROOM-32 + 2-axis analog joystick into a Bluetooth LE
 * HID keyboard that drives PowerPoint slide navigation on Windows.
 *
 * No companion PC software required - the ESP32 pairs as a normal
 * Bluetooth keyboard.
 *
 * Library used: T-vK/ESP32-BLE-Keyboard, running in NimBLE mode.
 * (See README.md for why NimBLE mode is required and how to enable it.)
 *
 * Board: ESP32-WROOM-32 / ESP32 DevKit V1
 */

#include <BleKeyboard.h>

// =====================================================================
// CONFIGURATION - edit these to match your wiring / preferences
// =====================================================================

// ---- Pins ----
// GPIO34/35 are input-only ADC1 pins with no strapping function - safe
// choices for the joystick axes. GPIO32 is a general-purpose ADC1 pin
// with an internal pull-up, used here as a plain digital input for the
// joystick's push-button switch.
#define PIN_JOY_VRX   34   // Joystick X axis (ADC1_CH6)
#define PIN_JOY_VRY   35   // Joystick Y axis (ADC1_CH7)
#define PIN_JOY_SW    32   // Joystick push-button (active LOW)

// ---- Bluetooth device identity ----
#define BLE_DEVICE_NAME   "PPT Remote"     // max 15 characters
#define BLE_MANUFACTURER  "DIY Electronics"
#define BLE_BATTERY_LEVEL 100              // reported level; not read from real battery

// ---- Joystick calibration ----
// 12-bit ADC: readings range 0-4095. A centered, undriven joystick
// typically rests near 2048 but drifts between units, so the deadzone
// is defined as a band around the observed center rather than a fixed
// pair of absolute thresholds.
#define ADC_MAX            4095
#define ADC_CENTER         2048
#define ADC_DEADZONE        700   // +/- counts around center treated as "neutral"
#define ADC_TRIGGER_MARGIN  350   // must clear the deadzone edge by this much to fire
#define ADC_SAMPLES            6   // samples averaged per reading (noise smoothing)

// Derived thresholds:
//   OUTER edge  -> fires a slide-change action
//   INNER edge  -> must return inside this to re-arm (hysteresis)
#define THRESHOLD_HIGH_FIRE   (ADC_CENTER + ADC_DEADZONE + ADC_TRIGGER_MARGIN)
#define THRESHOLD_LOW_FIRE    (ADC_CENTER - ADC_DEADZONE - ADC_TRIGGER_MARGIN)
#define THRESHOLD_HIGH_REARM  (ADC_CENTER + ADC_DEADZONE)
#define THRESHOLD_LOW_REARM   (ADC_CENTER - ADC_DEADZONE)

// ---- Timing ----
#define SAMPLE_INTERVAL_MS      15   // how often the joystick is polled
#define SLIDE_CHANGE_COOLDOWN_MS 350  // minimum gap between two slide changes
#define BUTTON_DEBOUNCE_MS       50   // button debounce window

// ---- Button action ----
// KEY_F5      -> Start Slide Show from the beginning
// Uncomment the SHIFT_F5 block below instead if you'd rather resume
// the slide show from the current slide.
#define BUTTON_ACTION_KEY  KEY_F5
// #define BUTTON_USE_SHIFT_F5

// =====================================================================
// END CONFIGURATION
// =====================================================================

BleKeyboard bleKeyboard(BLE_DEVICE_NAME, BLE_MANUFACTURER, BLE_BATTERY_LEVEL);

enum AxisState { AXIS_NEUTRAL, AXIS_HIGH_LATCHED, AXIS_LOW_LATCHED };

AxisState xState = AXIS_NEUTRAL;
AxisState yState = AXIS_NEUTRAL;

unsigned long lastSampleTime = 0;
unsigned long lastSlideChangeTime = 0;

bool buttonLastReading = HIGH;   // idle state with INPUT_PULLUP
bool buttonStableState = HIGH;
unsigned long buttonLastChangeTime = 0;

// ---------------------------------------------------------------------
// Reads and averages several ADC samples on a pin to reduce noise.
// ---------------------------------------------------------------------
int readAxisAveraged(uint8_t pin) {
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
  }
  return (int)(sum / ADC_SAMPLES);
}

// ---------------------------------------------------------------------
// Sends a "next slide" or "previous slide" keystroke.
//
// PowerPoint's native Slide Show key bindings are:
//   Right Arrow / Down Arrow / Page Down / Space / Enter -> next slide
//   Left Arrow  / Up Arrow   / Page Up   / Backspace      -> previous slide
//
// The project spec asks for joystick UP and RIGHT to mean "next" and
// DOWN and LEFT to mean "previous". If UP/DOWN keys were forwarded
// literally, UP would trigger PowerPoint's native "previous" binding -
// the opposite of what's wanted. To avoid that mismatch, every "next"
// action below sends KEY_RIGHT_ARROW and every "previous" action sends
// KEY_LEFT_ARROW, since those two keys are the most consistently
// supported "next/previous" pair across PowerPoint versions.
// ---------------------------------------------------------------------
void sendNextSlide() {
  if (!bleKeyboard.isConnected()) return;
  bleKeyboard.write(KEY_RIGHT_ARROW);
  Serial.println("Action: NEXT slide (Right Arrow)");
}

void sendPreviousSlide() {
  if (!bleKeyboard.isConnected()) return;
  bleKeyboard.write(KEY_LEFT_ARROW);
  Serial.println("Action: PREVIOUS slide (Left Arrow)");
}

// ---------------------------------------------------------------------
// Evaluates one axis against the fire/rearm thresholds and returns
// which direction fired this cycle, if any allowed by the cooldown.
// direction: +1 = high side fired, -1 = low side fired, 0 = nothing
// ---------------------------------------------------------------------
int evaluateAxis(int reading, AxisState &state) {
  unsigned long now = millis();
  bool cooldownOk = (now - lastSlideChangeTime) >= SLIDE_CHANGE_COOLDOWN_MS;

  switch (state) {
    case AXIS_NEUTRAL:
      if (reading >= THRESHOLD_HIGH_FIRE && cooldownOk) {
        state = AXIS_HIGH_LATCHED;
        lastSlideChangeTime = now;
        return +1;
      }
      if (reading <= THRESHOLD_LOW_FIRE && cooldownOk) {
        state = AXIS_LOW_LATCHED;
        lastSlideChangeTime = now;
        return -1;
      }
      break;

    case AXIS_HIGH_LATCHED:
      // Wait for the stick to come back inside the inner band before
      // it can fire again - this is what prevents repeated triggers
      // while the joystick is held over or bouncing at the edge.
      if (reading < THRESHOLD_HIGH_REARM && reading > THRESHOLD_LOW_REARM) {
        state = AXIS_NEUTRAL;
      }
      break;

    case AXIS_LOW_LATCHED:
      if (reading < THRESHOLD_HIGH_REARM && reading > THRESHOLD_LOW_REARM) {
        state = AXIS_NEUTRAL;
      }
      break;
  }
  return 0;
}

// ---------------------------------------------------------------------
// Polls the joystick axes and issues slide-change actions.
// ---------------------------------------------------------------------
void handleJoystick() {
  int xReading = readAxisAveraged(PIN_JOY_VRX);
  int yReading = readAxisAveraged(PIN_JOY_VRY);

  int xEvent = evaluateAxis(xReading, xState);
  int yEvent = evaluateAxis(yReading, yState);

  // RIGHT (+X) -> next, LEFT (-X) -> previous
  if (xEvent == +1) sendNextSlide();
  else if (xEvent == -1) sendPreviousSlide();

  // UP (+Y) -> next, DOWN (-Y) -> previous
  // NOTE: whether "up" reads as a high or low ADC value depends on how
  // your joystick module is wired/oriented. If UP and DOWN are
  // reversed on your hardware, swap the two lines below rather than
  // rewiring anything.
  if (yEvent == +1) sendNextSlide();
  else if (yEvent == -1) sendPreviousSlide();
}

// ---------------------------------------------------------------------
// Debounced button read. Fires once per press (on the falling edge).
// ---------------------------------------------------------------------
void handleButton() {
  bool reading = digitalRead(PIN_JOY_SW);
  unsigned long now = millis();

  if (reading != buttonLastReading) {
    buttonLastChangeTime = now;
  }

  if ((now - buttonLastChangeTime) > BUTTON_DEBOUNCE_MS) {
    if (reading != buttonStableState) {
      buttonStableState = reading;
      if (buttonStableState == LOW) {   // press detected
        if (bleKeyboard.isConnected()) {
#ifdef BUTTON_USE_SHIFT_F5
          bleKeyboard.press(KEY_LEFT_SHIFT);
          bleKeyboard.press(KEY_F5);
          delay(10);
          bleKeyboard.releaseAll();
          Serial.println("Action: Resume Slide Show (Shift+F5)");
#else
          bleKeyboard.write(BUTTON_ACTION_KEY);
          Serial.println("Action: Start Slide Show (F5)");
#endif
        }
      }
    }
  }

  buttonLastReading = reading;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32 PowerPoint Remote starting...");

  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  // PIN_JOY_VRX / PIN_JOY_VRY need no pinMode() call - analogRead()
  // configures ADC pins automatically on the ESP32 Arduino core.

  bleKeyboard.begin();
  Serial.println("BLE advertising started. Pair with your PC as a Bluetooth keyboard.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;
    if (bleKeyboard.isConnected()) {
      handleJoystick();
    }
  }

  // Button is polled every loop iteration (not gated by SAMPLE_INTERVAL_MS)
  // since debouncing already provides its own timing control.
  handleButton();

  // No manual reconnect logic is required here: as of v0.3.0, the
  // T-vK/ESP32-BLE-Keyboard library automatically restarts BLE
  // advertising internally when the host disconnects, so the device
  // will show back up and reconnect on its own. isConnected() above
  // simply gates key-sending until a host is actually present.
}
