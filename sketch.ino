/*
 * Q4.2 - Potentiometer-Based Servo Control (Arduino Uno)
 *
 * Continuous sense -> scale -> actuate loop:
 *   potentiometer (A0, 10-bit ADC 0..1023)
 *     -> exponential moving-average filter (removes ADC noise, gives smooth motion)
 *     -> map to 0..180 degrees
 *     -> Servo.write()  (the servo's own position feedback closes the loop)
 *
 * Wiring: pot VCC=5V, GND=GND, SIG=A0      servo V+=5V, GND=GND, PWM=D9
 */
#include <Arduino.h>
#include <Servo.h>

constexpr uint8_t  POT_PIN         = A0;
constexpr uint8_t  SERVO_PIN       = 9;
constexpr float    FILTER_ALPHA    = 0.2f;   // 0..1, lower = smoother but slower
constexpr uint16_t LOOP_DELAY_MS   = 15;     // servo update period (~66 Hz)

// Servo library maps 0..180 deg to this pulse range by default
constexpr int      PULSE_MIN_US    = 544;
constexpr int      PULSE_MAX_US    = 2400;

Servo servo;
float filtered   = 0.0f;
int   lastAngle  = -1;

void setup() {
  Serial.begin(115200);
  servo.attach(SERVO_PIN);                 // default 544..2400 us
  filtered = analogRead(POT_PIN);          // start from the real position (no jump at boot)

  Serial.println(F("=== Q4.2 Potentiometer -> Servo (0-180 deg) ==="));
  Serial.println(F("Turn the potentiometer; a line is printed whenever the angle changes."));
}

void loop() {
  int raw = analogRead(POT_PIN);                          // 0..1023
  filtered += FILTER_ALPHA * (raw - filtered);            // low-pass filter

  int angle = map((int)(filtered + 0.5f), 0, 1023, 0, 180);
  angle = constrain(angle, 0, 180);

  if (angle != lastAngle) {                               // only touch the servo when needed
    servo.write(angle);
    lastAngle = angle;

    Serial.print(F("ADC="));    Serial.print(raw);
    Serial.print(F("  Angle=")); Serial.print(angle);
    Serial.print(F(" deg  Pulse="));
    Serial.print(map(angle, 0, 180, PULSE_MIN_US, PULSE_MAX_US));
    Serial.println(F(" us"));
  }
  delay(LOOP_DELAY_MS);
}
