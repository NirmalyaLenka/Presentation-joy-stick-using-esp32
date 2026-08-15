/*
 * Joystick Raw Test
 * ------------------
 * Standalone diagnostic sketch - no BLE, no debounce logic.
 * Just prints raw ADC readings and button state so you can confirm
 * the HW-504 joystick and the ESP32 pins are working correctly
 * before wiring anything else in.
 *
 * Wiring (3.3V, not 5V - see project README for why):
 *   VRx -> GPIO34
 *   VRy -> GPIO35
 *   SW  -> GPIO32
 *   +5V -> 3V3
 *   GND -> GND
 *
 * Open Tools -> Serial Monitor at 115200 baud after uploading.
 */

#define PIN_VRX 34
#define PIN_VRY 35
#define PIN_SW  32

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_SW, INPUT_PULLUP);
  Serial.println("Joystick raw test starting.");
  Serial.println("Move the stick through its full range and press the button.");
  Serial.println("Expect: X/Y roughly 0-4095, resting near ~2048. Button reads 1 (idle) / 0 (pressed).");
  Serial.println();
}

void loop() {
  int x = analogRead(PIN_VRX);
  int y = analogRead(PIN_VRY);
  int sw = digitalRead(PIN_SW);

  Serial.print("X: ");
  Serial.print(x);
  Serial.print("\tY: ");
  Serial.print(y);
  Serial.print("\tButton: ");
  Serial.println(sw == LOW ? "PRESSED" : "idle");

  delay(200);
}
