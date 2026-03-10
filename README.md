# STM32 Ultrasonic Distance Measurement

This project implements an ultrasonic distance measurement system using an STM32 NUCLEO F401RE and an HC-SR04 ultrasonic sensor.  
The distance is measured by sending an ultrasonic pulse and measuring the duration of the returning echo signal using a hardware timer.

The implementation uses **timer input capture interrupts and a non-blocking state machine** to measure the echo pulse width with microsecond resolution.

---

## Features

- Microsecond-precision timing using **TIM2**
- **Interrupt-based echo detection** using timer input capture
- **Non-blocking state machine** for measurement control
- UART output to transmit distance measurements to the PC

---

# Hardware

Example hardware setup:

- STM32 NUCLEO F401RE
- HC-SR04 ultrasonic sensor

### Pin connections

| Sensor  | STM32 Pin | Description           |
|---------|-----------|-----------------------|
| VCC     | 5V        | Sensor power supply   |
| GND     | GND       | Ground                |
| TRIG    | PA0       | Trigger output signal |
| ECHO    | PA1       | Echo input (TIM2 CH2) |
| UART TX | USART2 TX | Serial output to PC   |

The measured distance is transmitted via UART using USART2 (TX pin).
On the development board, USART2 is connected to the onboard ST-Link
virtual COM port. When the board is connected to the PC via USB, the
distance measurements can be viewed through a serial terminal.

---

# Measurement Principle

The ultrasonic sensor measures distance by emitting a short ultrasonic
pulse and measuring the time until the reflected signal returns.

To start a measurement, the **TRIG** pin is set HIGH for **10 µs**.
This causes the sensor to transmit a short burst of ultrasonic sound
waves (typically around 40 kHz).

After sending the pulse, the sensor sets the **ECHO** pin HIGH.
The ECHO pin remains HIGH until the reflected sound wave is detected.
When the echo is received, the pin is set LOW again.

The duration of the HIGH signal on the ECHO pin therefore represents
the **round-trip travel time** of the sound wave between the sensor
and the object.

Since the sound travels to the object and back, the measured time
corresponds to twice the distance. The distance can be estimated by:

```
distance_cm = echo_time_us / 58
```

This approximation assumes a speed of sound of approximately **343 m/s**
in air at room temperature. Effects such as temperature, humidity, and
air pressure are not considered in this simple model and may slightly
affect measurement accuracy.

---

# Software Architecture

The measurement logic is implemented as a **state machine** executed in the main loop.

States:

| State           | Description                                             |
|-----------------|---------------------------------------------------------|
| `WAIT_INTERVAL` | Waits a fixed time before starting the next measurement |
| `TRIGGER`       | Sends a 10 µs trigger pulse to the ultrasonic sensor    |
| `WAIT_RISING`   | Waits for the rising edge of the echo signal            |
| `WAIT_FALLING`  | Waits for the falling edge of the echo signal           |
| `DONE`          | Calculates echo duration and converts it to distance    |
| `TIMEOUT`       | Handles the case where no echo is received              |

### Timer configuration

- Timer: **TIM2**
- Prescaler: **83**
- Timer clock: **1 MHz**
- Resolution: **1 µs**

TIM2 runs as a free-running counter. With a system clock of 84 MHz and
a prescaler of 83, the timer increments once every microsecond.

The echo duration is calculated using:

```
duration = ic_end - ic_start
```

The subtraction is performed using unsigned arithmetic, which ensures
correct operation even if the timer counter overflows between the two
capture events.

---

# Interrupt Handling

Echo signal edges are detected using the **TIM2 input capture interrupt**.

- Rising edge → start timestamp captured (`ic_start`)
- Falling edge → end timestamp captured (`ic_end`)

The interrupt handler also switches the capture polarity between rising
and falling edges and advances the state machine. The main loop later
processes the captured timestamps to compute the distance.

---

## Build

The project was generated using **STM32CubeMX** and is built using a
CMake-based toolchain in **Visual Studio Code**.

Typical workflow:

1. Open the project folder in **VS Code**.
2. Configure the project using the CMake extension.
3. Build the firmware using the CMake build command.
4. Flash the firmware to the STM32 board using the debugger
   (e.g. ST-Link).

---

## Example Output

Distance measurements are transmitted via **USART2** using the onboard
ST-Link Virtual COM Port.

When the board is connected to a PC via USB, a virtual serial interface
appears. The output can be viewed using a serial terminal with the following settings:

Example output:
```
Distance: 24 cm
Distance: 25 cm
Distance: 23 cm
Distance: 24 cm
```

If no echo signal is detected within the configured timeout period,
the system prints:
```
out of range
```


---

# Learning Goals

This project demonstrates several important embedded systems concepts:

- Hardware timers
- Timer input capture
- Interrupt-driven programming
- Non-blocking state machines
- Microsecond timing
- UART communication

---

# License

This project is licensed under the MIT License.