# Fadime AI Studio — ESP32-S3 test firmware

This branch adds a manual GitHub Actions firmware build for the user's known-working ESP32-S3 DevKitC-1 N16R8 configuration. It does **not** modify the running device or deploy to panel.haklio.com.

## Hardware and verified wiring

- ST7789 240x240 7-pin IPS screen: SCL/GPIO21, SDA/GPIO47, RES/GPIO45, DC/GPIO40, BLK/GPIO42, GND/GND, VCC/3V3. The module has no separate CS pin.
- INMP441: WS/GPIO4, SCK/GPIO5, SD/GPIO6, 3V3, GND, L/R to GND.
- MAX98357A: DIN/GPIO7, BCLK/GPIO15, LRC/GPIO16.
- Turkish language (tr-TR); wake word disabled for the baseline compilation.

## Build behavior

The manual workflow clones the upstream Xiaozhi source on each run, overrides the LCD board build variant from its default 240x320 screen to ST7789 240x240 7PIN, checks the pin defines and compiles via ESP-IDF in the cloud.

**First build is an unverified test**. Do not flash it to the already working device merely because GitHub Actions passes. It has not been hardware-tested and contains no custom panel.haklio.com client.

Build artifact is a merged image for an appropriate full flash process, **not** an OTA firmware image. The source version is upstream main at build time; pin it to a specific commit before production use. 

No credentials, database settings or API tokens belong in this repository.
