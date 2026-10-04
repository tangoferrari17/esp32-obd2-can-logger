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
