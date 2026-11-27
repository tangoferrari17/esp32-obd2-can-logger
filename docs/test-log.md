# Test Log

One entry per bench or vehicle session. Record what was tried, what happened and what changed, including failures. A failed session with a clear explanation is worth more than a vague success.

## Template

```
### YYYY-MM-DD — <bench | vehicle: year make model>
**Goal:**
**Setup:** firmware commit, wiring changes, ignition state
**Result:**
**Evidence:** log file name, serial output excerpt, photo
**Next:**
```

---

### 2026-10 — Bench: SD card
**Goal:** Verify SD read/write on the ESP32 SPI bus.
**Setup:** SD breakout soldered and wired to the shared SPI bus.
**Result:** Read/write verified with a test sketch.
**Next:** MCP2515 loopback self-test on hardware, then first vehicle connection.

### 2026-10-09 — Bench: MCP2515 loopback + SD on shared SPI bus
**Goal:** Confirm the MCP2515 talks to the ESP32 over SPI and that SD logging still works with both devices on the bus.
**Setup:** MCP2515 on the shared SPI bus (SCK 18 / MISO 19 / MOSI 23), CS on GPIO 5, 8 MHz crystal, USB power. SD CS is on GPIO 15 (config.h previously assumed 13; changed to match the real wiring). CAN-H/CAN-L not connected.
**Result:** Loopback self-test passed. After the SD CS fix the card mounted and the logger opened `/log_001.csv`. Every PID row logged as `tx_error` or `timeout`, which is expected with no bus attached. The first run failed SD init (`Card Failed! cmd 0x00`) because the firmware was selecting the wrong CS pin.
**Not yet verified:** the CAN bitrate/crystal setting (loopback is internal and does not exercise it), and the contents of the CSV on a computer (no card reader yet).
**Evidence:** serial output of the passing loopback and `Logging to /log_001.csv`.
**Next:** Read the CSV on a laptop; first vehicle connection (parked, ignition on) with a real RPM response.

### 2026-10-13 — Bench: SSD1306 OLED on I2C
**Goal:** Wire the OLED and confirm the ESP32 can address it and draw text.
**Setup:** OLED VCC to 3V3, GND to GND, SDA to GPIO 33, SCL to GPIO 32 (moved off the default 21/22 for wiring layout). USB power. CAN-H/CAN-L not connected. Standalone Arduino sketches, not yet part of the logger firmware.
**Result:** An I2C scan found one device at `0x3C`. A second sketch initialised the display with Adafruit SSD1306 + GFX and rendered text on screen. The panel is two-color (yellow and blue bands); band boundaries are noted in the bench sketch for layout later.
**Not yet verified:** the OLED has not been driven by the logger firmware or shown live CAN data.
**Next:** Add a small display module to the logger (SD/CAN status, row count, then live PID values). Still open from the Oct 9 entry: CSV contents on a computer, then the first vehicle connection.

### 2026-10-15 — Bench: SD logging in the combined sketch (SdFat)
**Goal:** Log one row per second to the SD card from a combined bench sketch (MCP2515 loopback + OLED + SD) and read it back over Serial.
**Setup:** Standalone Arduino sketch `MCP2515-OLED-SD` (not part of the PlatformIO firmware tree). MCP2515 in loopback mode, OLED on GPIO 33/32, SD CS on GPIO 15, shared SPI bus, USB power. CAN-H/CAN-L not connected. SD library: SdFat, `SdSpiConfig(15, SHARED_SPI, SD_SCK_MHZ(16))`.
**Result:** Rows of `ms, rpm` written once per second and read back on the next boot with `dumpLog()`: timestamps 1000 ms apart, RPM 1726 on every row (the loopback test frame). The OLED showed the same RPM.
**What was learned about the card:** The 64 GB card ships exFAT. The ESP32 Arduino `SD` library failed to mount it even after a FAT32 format (`f_mount failed: (13) There is no valid FAT volume`), while SdFat's SdInfo reported FAT32 and SdFat mounted and wrote to it. The FAT32 format was done with SdFat's SdFormatter example, with the exFAT branch of `formatCard()` replaced by `fatFormatter` and `SPI_CLOCK` lowered from 50 MHz to 16 MHz (at 50 MHz the formatter failed with a `READ_TOKEN` error on the breadboard wiring).
**Not yet verified:** the PlatformIO firmware still uses the ESP32 `SD` library, so it has not been run against this card. The CAN bitrate on a real bus is still unverified.
**Next:** Move the firmware's SD logging to SdFat (or use a card of 32 GB or less); identify the OBD-II pigtail wires and connect CAN-H/CAN-L to the module; first vehicle connection.

### 2026-11-27 — Bench: OBD-II pigtail wired to MCP2515, car-mode sketch
**Goal:** Identify the OBD-II pigtail wires, connect ground and CAN-H/CAN-L to the MCP2515 module, and check the car-mode sketch (normal mode) on the bench.
**Setup:** Pigtail unplugged from any vehicle. Wires identified by continuity with a throwaway ESP32 tester sketch (GPIO 4 INPUT_PULLUP, jumper probes, 0 = connected) because the multimeter battery was dead. The seller's color chart matched: pin 4 Orange (chassis ground), pin 5 Yellow (signal ground), pin 6 Green (CAN-H), pin 14 Brown/White (CAN-L), pin 16 Green/White (+12V). The pin 4 wire is spliced to a male jumper, soldered, and plugged into the breadboard GND rail. CAN-H and CAN-L are in the module's screw terminals, 120 ohm termination jumper off. Standalone Arduino sketch `MCP2515-OLED-SD_car` (setNormalMode, request ID 0x7DF, request {02 01 0C}, rpm reset to -1 each cycle), not the PlatformIO firmware. USB power.
**Result:** Continuity checks all as expected. The ground wire connects to pin 4 and not to pin 16, and the soldered jumper reads connected to GND. Green reaches pin 6 and Brown/White reaches pin 14 (not swapped). H and L are not shorted to each other or to pin 16. The sketch prints "CAN Initialized!" in normal mode at 500 kbps. With no bus attached, sendMessage returns 4 (ERROR_FAILTX) and then 2 (ERROR_ALLTXBUSY) and replies time out, which is expected with no node to acknowledge.
**Not yet verified:** Real CAN traffic, the bitrate and crystal setting on a real bus, an ECU response. The pin 16 (+12V) wire and the unused pigtail wires are still uninsulated. The 1k/2k divider on MCP2515 SO to GPIO 19 is not installed (no resistors yet).
**Evidence:** Serial output of the no-bus run; tester sketch readings in the Serial Monitor.
**Next:** Fresh log filename and CSV header; buy resistors, electrical tape and heat shrink; install the SO divider; insulate pin 16 and the unused wires; first vehicle session (parked, ignition on).
