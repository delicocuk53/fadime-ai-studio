# Fadime config sync v2.3 (experimental)

The ESP32-S3 polls `device-api.php?action=next_config` over HTTPS every 30 seconds. Requests authenticate using already-provisioned NVS device credentials. The existing PHP API marks a fetched item `delivered`; the device then posts `ack_config` with either `applied` or `failed`.

**Supported now:** a JSON settings object containing **only** `{"brightness": 0..100}` with an integral number; applied through the existing Xiaozhi Backlight API, then acknowledged as applied. If any additional or unsupported setting exists, the whole job is reported **failed**, not falsely marked successful. No permanent brightness persistence is requested.

**Not supported:** theme, facial expression, custom animation, personality, TTS/voice, VAD, volume, wake word, OTA or remote commands. The current panel's general “Ayarları kuyruğa al” button creates a *multi-field* payload which will be marked failed; use a dedicated brightness-only UI in a later panel update after the firmware is verified.

No token embedded in code. USB pair remains unchanged. The firmware is a full merged flash image, **not OTA**. GitHub Actions passing does not establish runtime behavior. Test on a spare ESP32, or after a deliberate recovery plan on the existing device.