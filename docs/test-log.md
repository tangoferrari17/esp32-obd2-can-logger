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
