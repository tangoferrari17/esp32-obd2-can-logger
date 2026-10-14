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
