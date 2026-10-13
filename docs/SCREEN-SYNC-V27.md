# Fadime v2.7 experimental screen synchronization

The settings queue accepts the following **strict** screen-only payload:

```json
{"screen":{"theme":"dark","emotion":"happy","message":"Merhaba! Ben Fadime.","brightness":80}}
```

The existing ESP32 client polls `device-api.php?action=next_config` and acknowledges via `ack_config`. Screen settings are applied using Xiaozhi's built-in display API (light/dark theme, supported emotion, assistant message) and Backlight API. This does **not** reproduce the exact custom HTML/CSS face preview: custom RGB face/background colors and face drawing are not yet implemented on the embedded display. Sound and Wi-Fi remain untouched in the code.

Security: values are server-side validated, no arbitrary code, URLs, GPIO, or flash commands allowed. Older firmware won't process the new screen payload; jobs may stay pending until the new build is tested. A successful CI build is not a hardware test. The merged binary remains a FULL FLASH image, not OTA. No credentials are embedded.