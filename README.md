# Real-Time Embedded Systems

Coursework for **Real-Time Embedded Systems** (Cyber-Physical Systems) at the University of Tehran, Faculty of Electrical and Computer Engineering (Spring 2026, Dr. Mahdi Kargahi & Dr. Mohsen Shokrisaz).

## Projects

| # | Project | What it does | Tech |
|---|---------|--------------|------|
| HW1 *(team)* | [Communication-protocol simulator](hw1-communication-protocols-simulator/) | Extends a C++ hardware simulator with bit-level implementations of three buses: **I2C** (SDA/SCL, start/stop bits, addressing, ACK) between an ESP8266 and a VL53L0X distance sensor, **USART/USB** to a simulated hard disk, and an **I2C multiplexer** that serves several sensors on one bus | C++20, Boost, CMake |
| HW2 | [Air Mouse](hw2-air-mouse/) | An Android app that turns the phone into a wireless mouse. It reads the **raw** gyroscope, accelerometer and magnetometer; calibrates them (gyro bias, 6-position accelerometer offset and scale, figure-8 magnetometer hard-iron correction); fuses them with a hand-written **Madgwick AHRS filter**; and streams cursor, click and scroll events over **UDP**, retransmitting click/scroll until acknowledged | Java (Android), Python |

## HW1 – build & run

```bash
cd hw1-communication-protocols-simulator
cmake -S . -B build        # needs Boost headers (e.g. `brew install boost`)
cmake --build build
./build/CPS4042
```

`src/main.cpp` runs the I2C-multiplexer scenario. `src/main12.cpp` (I2C sensor and USB disk) and `src/main3.cpp` are the stand-alone programs for the individual parts; swap one into `CMakeLists.txt` to run it. All three build and run, and every value the sensor measures is received intact at the other end of each bus.

## HW2 – Air Mouse

```
phone ── UDP ──▶ laptop
  M:<dx>,<dy>   cursor movement (may be dropped)
  S:<amount>    scroll          (resent until ACK)
  C:L | C:R     click           (resent until ACK)
```

- `android-app/` is the Android Studio project (Android 8.0+, API 26). Enter the laptop's IP in the app, run **Calibrate**, then **Start**.
- `laptop-receiver/main.py` listens on UDP port 8081 and acknowledges click and scroll packets. **Status:** the receiver currently only logs and acknowledges packets. Moving the cursor, clicking and scrolling with `pyautogui` on the laptop is not implemented yet.

## Team

HW1: Babak Hosseini Mohtasham, Hanita Niknasab, Narges Asadi Khansari and Alireza Karimi.
