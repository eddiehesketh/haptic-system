# haptic-system

Bare-metal firmware for an STM32C031C6 (NUCLEO-C031C6) haptic feedback system. An MPU6050 accelerometer gives roll, pitch and tilt angles, and three servos push back on the user as the angles grow. An autopilot / manual mode is toggled with a button and shown on two LEDs.

## How it works

- **TIM3** runs at 50 Hz. Each update event applies the latest servo pulses and tells the main loop to sample the sensor.
- **Main loop** reads the MPU6050 over I2C, computes roll, pitch and tilt, and runs one FSM per axis.
- **Axis FSM** (`Src/logic/fsm.c`) moves between three zones, with a 5 degree hysteresis band on the way back down:

  | Axis | Free | Compliant (servo follows) | Clamped (servo held) |
  | --- | --- | --- | --- |
  | Roll, pitch | below 30 degrees | 30 to 50 degrees, 1:1 | above 50 degrees |
  | Tilt | below 20 degrees | 20 to 70 degrees, 2 x tilt | above 70 degrees |

- **Mode FSM** switches to manual when the button is pressed and roll and pitch are both free. It returns to autopilot when the button is pressed again, or when roll and pitch are both clamped.

## Pin allocations (STM32C031C6)

| Hardware | Signal | Pin | Description |
| --- | --- | --- | --- |
| Haptic servos | Roll | PB4 | PWM (50 Hz), TIM3 CH1 |
| | Pitch | PB5 | PWM (50 Hz), TIM3 CH2 |
| | Tilt | PB0 | PWM (50 Hz), TIM3 CH3 |
| Indicator LEDs | Autopilot | PA2 | Output |
| | Manual | PB11 | Output |
| MPU6050 IMU | SCL | PB8 | I2C serial clock |
| | SDA | PB9 | I2C serial data |
| Autopilot button | Autopilot Btn | PA12 | Input (pull-down) |

## Layout

| Path | Contents |
| --- | --- |
| `Src/main.c` | Main loop, TIM3 and EXTI interrupt handlers |
| `Src/logic/` | `init.c` (system setup), `fsm.c` (axis and mode FSMs), `angles.c` (angle calculation, MPU6050 read) |
| `Src/wrappers/` | Peripheral wrappers: `gpio.c`, `pwm.c`, `timer.c`, `interrupt.c`, `i2c.c`, `reset_and_control_clock.c` |
| `Src/syscall.c`, `sysmem.c`, `startup_stm32c031xx.S` | Newlib stubs and startup code |
| `Inc/drivers/logic`, `Inc/drivers/wrappers` | Headers for `Src/logic` and `Src/wrappers` |
| `tests/` | Host-side unit tests with the hardware faked |
| `diagram.json`, `wokwi.toml` | Wokwi simulation |

## Build

Requires the Arm GNU toolchain (`arm-none-eabi-gcc`), CMake and Ninja. The project is set up for the STM32Cube extension for VS Code, but it also builds from the command line:

```sh
cmake --preset Release
cmake --build build/Release
```

This produces `build/Release/haptic-system.elf` along with `.hex` and `.bin` files. Use the `Debug` preset for a debug build.

## Run in Wokwi

Build the `Release` preset, then start the simulation from the project root. `wokwi.toml` points at `build/Release/haptic-system.hex` and `.elf`.

## Tests

The FSMs and angle maths run on the host with the hardware faked. From the repository root:

```sh
sh tests/run_tests.sh
```
