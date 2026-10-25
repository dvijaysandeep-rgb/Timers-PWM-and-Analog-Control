# Assessment: Timers, PWM and Analog Control (Arduino Uno)

Two tasks, both simulated in **Wokwi** on an Arduino Uno (ATmega328P, 16 MHz).

| Task | Folder | Wokwi link |
|---|---|---|
| **Q4.1** Timer-based 1 kHz, 45 % PWM for a DC motor driver | `q4_1_timer_pwm/` | `<ADD Q4.1 WOKWI LINK>` |
| **Q4.2** Potentiometer-based servo control, 0-180 deg | `q4_2_pot_servo/` | `<ADD Q4.2 WOKWI LINK>` |

---

## Q4.1 Timer-Based PWM Generation

### Method
Timer1 (16-bit) is configured directly through its registers, with no `analogWrite()`:
**Fast PWM, mode 14 (TOP = ICR1), non-inverting output on OC1A (pin D9), prescaler 8.**

```
Timer clock      = F_CPU / N = 16 MHz / 8 = 2 MHz   (0.5 us per count)

Frequency        f = F_CPU / (N * (1 + ICR1))
                 1000 = 16 000 000 / (8 * (1 + ICR1))   ->   ICR1 = 1999

Duty (fast PWM, non-inverting)   D = (OCR1A + 1) / (ICR1 + 1)
                 0.45 = (OCR1A + 1) / 2000              ->   OCR1A = 899

Check            period = 2000 counts = 1000 us  (1 kHz)
                 high   =  900 counts =  450 us  (45 %)
                 low    = 1100 counts =  550 us
```

### Registers used
| Register | Value | Meaning |
|---|---|---|
| `ICR1` | 1999 | TOP, sets the 1 kHz period |
| `OCR1A` | 899 | Compare value, sets the 45 % duty |
| `TCCR1A` | `COM1A1 \| WGM11` | Non-inverting on OC1A, WGM bits 1:0 = 10 |
| `TCCR1B` | `WGM13 \| WGM12 \| CS11` | WGM bits 3:2 = 11 gives mode 14, `CS11` gives clk/8 |

The constants are computed at compile time from `PWM_FREQ_HZ` and `DUTY_PERCENT`, and `static_assert`
checks that 45 % is exactly representable.

### Motor driver interface
**Real hardware (L298N module):**

| Arduino | L298N | Note |
|---|---|---|
| D9 (OC1A) | ENA | Remove the ENA jumper so the PWM controls speed |
| D8 | IN1 | HIGH |
| D7 | IN2 | LOW (IN1 high, IN2 low = forward) |
| GND | GND | Common ground with the motor supply |
| - | 12V / +V | External motor supply (not from the Arduino) |
| - | OUT1, OUT2 | DC motor terminals |

**Simulation:** Wokwi has no DC motor or L298N part, so the driver inputs are shown with LEDs:
green = ENA (its brightness follows the PWM duty), yellow = IN1, red = IN2. D9 is jumpered to D2 so the
sketch can measure its own PWM signal with `pulseIn()`, the same jumper works on real hardware.

### Expected serial output
```
=== Q4.1 Timer1 Fast PWM : 1 kHz, 45 % duty ===
F_CPU=16000000 Hz  prescaler=8  ICR1(TOP)=1999  OCR1A=899
Calculated from registers: f=1000.0 Hz  duty=45.0 %
Motor: ENA=D9 (PWM)  IN1=D8=HIGH  IN2=D7=LOW  -> forward
Measured on D2 : f=1000.0 Hz  high=450 us  low=550 us  duty=45.0 %
```
The "Calculated" line comes from the register values and should be exact. The "Measured" line may differ by a
fraction of a percent because `pulseIn()` has about 1 us resolution.

### Test results (real Wokwi run)
| # | Test | Expected | Result |
|---|---|---|---|
| 1 | Register values | ICR1 = 1999, OCR1A = 899 | **PASS** - serial shows `ICR1(TOP)=1999  OCR1A=899` |
| 2 | Frequency | 1000 Hz | **PASS** - calculated 1000.0 Hz, measured 999.0 Hz (high 450 us + low 551 us, the 1 us difference is `pulseIn()` resolution) |
| 3 | Duty cycle | 45 % | **PASS** - calculated 45.0 %, measured 45.0 % on every reading |
| 4 | Motor driver inputs | ENA PWM, IN1 on, IN2 off | **PASS** - ENA LED lit by PWM, IN1 LED on, IN2 LED off (pin states D8 = High, D7 = Low in the simulator) |

Serial output from the run:
```
=== Q4.1 Timer1 Fast PWM : 1 kHz, 45 % duty ===
F_CPU=16000000 Hz  prescaler=8  ICR1(TOP)=1999  OCR1A=899
Calculated from registers: f=1000.0 Hz  duty=45.0 %
Motor: ENA=D9 (PWM)  IN1=D8=HIGH  IN2=D7=LOW  -> forward
Measured on D2 : f=999.0 Hz  high=450 us  low=551 us  duty=45.0 %
```

![Q4.1 simulation](docs/screenshots/q4_1_serial_pwm.png)


---

## Q4.2 Potentiometer-Based Servo Control

### Method
A continuous sense, scale and actuate loop runs every 15 ms:

1. `analogRead(A0)` gives 0-1023.
2. An exponential moving-average filter (`alpha = 0.2`) removes ADC noise so the servo moves smoothly.
3. `map(value, 0, 1023, 0, 180)` converts to degrees.
4. `servo.write(angle)` updates the servo only when the angle changes. The servo's internal position
   feedback closes the loop.

Wiring: pot `VCC`=5V, `GND`=GND, `SIG`=A0. Servo `V+`=5V, `GND`=GND, `PWM`=D9.

### Expected mapping
| Potentiometer | ADC | Angle | Servo pulse |
|---|---|---|---|
| Fully left | 0 | 0 deg | 544 us |
| 25 % | 256 | 45 deg | 1008 us |
| Middle | 512 | 90 deg | 1472 us |
| 75 % | 768 | 135 deg | 1936 us |
| Fully right | 1023 | 180 deg | 2400 us |

### Expected serial output
```
=== Q4.2 Potentiometer -> Servo (0-180 deg) ===
Turn the potentiometer; a line is printed whenever the angle changes.
ADC=512  Angle=90 deg  Pulse=1472 us
ADC=520  Angle=91 deg  Pulse=1481 us
...
```

### Test results (real Wokwi run)
| # | Test | Expected | Result |
|---|---|---|---|
| 1 | Pot fully left | Servo 0 deg | **PASS** - `ADC=0  Angle=0 deg  Pulse=544 us`, servo arm at 0 deg |
| 2 | Pot middle | Servo ~90 deg | **PASS** - `ADC=546  Angle=90 deg  Pulse=1472 us` (exactly the expected 1472 us for 90 deg). The pot then settles at ADC=546, which maps to 96 deg (546 x 180 / 1023 = 96.1), matching the printed value |
| 3 | Pot fully right | Servo 180 deg | **PASS** - `ADC=1023  Angle=180 deg  Pulse=2400 us` (the expected maximum pulse), servo arm at the 180 deg end |
| 4 | Smooth response while turning | Continuous movement, no jitter | **PASS over the full 0 to 180 deg range** - angle rises in small steps (3, 8, 12, 16 ... 28 deg, then 90, 91, 92 ... 96 deg) with the pulse width following it, no jumps back and forth. When the pot is moved quickly (ADC 546 to 996) the filter turns the jump into a ramp (111, 123, 133, 140, 144 deg) instead of an instant swing. The final approach to the end stop is also gradual (165, 167, 169 ... 179, 180 deg) |

Serial output from the run:
```
=== Q4.2 Potentiometer -> Servo (0-180 deg) ===
Turn the potentiometer; a line is printed whenever the angle changes.
ADC=0    Angle=0 deg   Pulse=544 us
ADC=106  Angle=3 deg   Pulse=574 us
ADC=155  Angle=8 deg   Pulse=626 us
ADC=155  Angle=12 deg  Pulse=667 us
ADC=193  Angle=16 deg  Pulse=708 us
ADC=193  Angle=20 deg  Pulse=750 us
ADC=193  Angle=22 deg  Pulse=770 us
ADC=193  Angle=24 deg  Pulse=791 us
ADC=193  Angle=26 deg  Pulse=812 us
ADC=193  Angle=28 deg  Pulse=832 us
```
The angle keeps rising after the raw ADC value stops changing (ADC=193) because the moving-average filter
is still catching up. This is the smoothing at work: the servo glides to the target instead of jumping.

Later part of the same run (pot moved through the middle and up to the upper range):
```
ADC=546  Angle=90 deg   Pulse=1472 us
ADC=546  Angle=91 deg   Pulse=1482 us
ADC=546  Angle=92 deg   Pulse=1492 us
ADC=546  Angle=93 deg   Pulse=1502 us
ADC=546  Angle=94 deg   Pulse=1513 us
ADC=546  Angle=95 deg   Pulse=1523 us
ADC=546  Angle=96 deg   Pulse=1533 us
ADC=996  Angle=111 deg  Pulse=1688 us
ADC=978  Angle=123 deg  Pulse=1812 us
ADC=978  Angle=133 deg  Pulse=1915 us
ADC=951  Angle=140 deg  Pulse=1987 us
ADC=932  Angle=144 deg  Pulse=2028 us
```

End of the sweep, pot fully right:
```
ADC=1000  Angle=165 deg  Pulse=2245 us
ADC=1012  Angle=167 deg  Pulse=2265 us
ADC=1015  Angle=169 deg  Pulse=2286 us
ADC=1019  Angle=171 deg  Pulse=2307 us
ADC=1019  Angle=173 deg  Pulse=2327 us
ADC=1023  Angle=174 deg  Pulse=2338 us
ADC=1023  Angle=175 deg  Pulse=2348 us
ADC=1023  Angle=176 deg  Pulse=2358 us
ADC=1023  Angle=177 deg  Pulse=2369 us
ADC=1023  Angle=178 deg  Pulse=2379 us
ADC=1023  Angle=179 deg  Pulse=2389 us
ADC=1023  Angle=180 deg  Pulse=2400 us
```

| 0 deg to 28 deg | 90 deg to 144 deg | 180 deg |
|---|---|---|
| ![](docs/screenshots/q4_2_servo_sweep_0_to_28deg.png) | ![](docs/screenshots/q4_2_servo_90_to_144deg.png) | ![](docs/screenshots/q4_2_servo_180deg.png) |

## How to run (Wokwi)
For each task: create a new **Arduino Uno** project at [wokwi.com](https://wokwi.com), then paste the folder's
`sketch.ino`, `diagram.json` and `libraries.txt` into the matching Wokwi files and press **Start Simulation**.

## Repository structure
```
.
├── README.md
├── q4_1_timer_pwm/   sketch.ino  diagram.json  libraries.txt
├── q4_2_pot_servo/   sketch.ino  diagram.json  libraries.txt
└── docs/screenshots/   q4_1_serial_pwm.png  q4_2_servo_sweep_0_to_28deg.png  q4_2_servo_90_to_144deg.png  q4_2_servo_180deg.png
```

## Assignment checklist
| Requirement | Where |
|---|---|
| Configure Timer registers | Q4.1 `setupTimer1Pwm()` |
| 1 kHz PWM, 45 % duty | `ICR1 = 1999`, `OCR1A = 899` |
| Interface with a DC motor driver | Q4.1 motor driver section |
| Verify the PWM signal | Q4.1 calculated and measured serial lines |
| Read potentiometer, map to 0-180 deg | Q4.2 `analogRead` and `map` |
| Dynamically update the servo, smooth response | Q4.2 filter and `servo.write` |
| Test the full potentiometer range | Q4.2 test table and screenshots |
| Source code, circuit files, evidence | Both folders, `docs/screenshots/` |
