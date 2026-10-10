# Fadime device integration — experimental

This branch adds an **experimental**, opt-in HTTPS heartbeat implementation for the Bread Compact Wi-Fi LCD board. It will be compiled in a *test* artifact only after merging and running GitHub Actions; no ESP32 has been flashed or connected by this change.

## What it does
- Checks Wi-Fi station connectivity, then once per minute reads NVS namespace `fadime` keys `device_id` and `device_token`.
- If either key is absent, it does **nothing** and does not report online.
- Sends HTTPS POST `https://panel.haklio.com/device-api.php?action=heartbeat` with the existing `X-Device-ID` and `X-Device-Token` headers.
- Requires TLS certificate bundle. Never logs the token.

## Not implemented yet
- Secure **device enrollment/provisioning** of those NVS values.
- Configuration polling, acknowledgements, OTA, and screen editing.
- Hardware runtime testing, security review, or successful CI compilation (must be verified).

**Never** put tokens in this repository, GitHub Actions variables, or build-time firmware constants. Token values should only be provisioned per-device through an authenticated local enrollment flow in a later iteration.

The firmware artifact remains a **merged full-flash test image**, not OTA. Keep the currently working ESP32 image in place until tested.
