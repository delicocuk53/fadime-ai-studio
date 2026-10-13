// Fadime AI Studio v2.3 - conservative configuration polling.
// Only brightness-only payloads can be acknowledged "applied".
// All other settings are explicitly rejected as "failed".
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_wifi.h>
#include <cJSON.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <cstring>
#include "display/lvgl_theme.h"

static bool FadimeConfigCredentials(std::string& id, std::string& token) {
    nvs_handle_t h;
    if (nvs_open("fadime", NVS_READONLY, &h) != ESP_OK) return false;
    auto read = [&](const char* key, std::string& value) -> bool {
        size_t n = 0;
        if (nvs_get_str(h, key, nullptr, &n) != ESP_OK || n < 2 || n > 256) return false;
        value.resize(n);
        if (nvs_get_str(h, key, value.data(), &n) != ESP_OK) return false;
        value.resize(n - 1);
        return true;
    };
    const bool ok = read("device_id", id) && read("device_token", token);
    nvs_close(h);
    return ok;
}
struct FadimeResponse {std::string body; bool overflow = false;};
static esp_err_t FadimeOnData(esp_http_client_event_t* event) {
    if(event->event_id == HTTP_EVENT_ON_DATA && event->data && event->data_len > 0) {
        auto* out = static_cast<FadimeResponse*>(event->user_data);
        if(out->body.size() + static_cast<size_t>(event->data_len) > 8192) {
            out->overflow = true;
            return ESP_FAIL;
        }
        out->body.append(static_cast<const char*>(event->data), event->data_len);
    }
    return ESP_OK;
}
static int FadimeRequest(const std::string& id, const std::string& token,
                          const char* endpoint, const char* payload,
                          FadimeResponse& out) {
    esp_http_client_config_t cfg = {};
    cfg.url = endpoint;
    cfg.method = payload ? HTTP_METHOD_POST : HTTP_METHOD_GET;
    cfg.timeout_ms = 8000;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.event_handler = FadimeOnData;
    cfg.user_data = &out;
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) return 0;
    esp_http_client_set_header(c, "X-Device-ID", id.c_str());
    esp_http_client_set_header(c, "X-Device-Token", token.c_str());
    if (payload) {
        esp_http_client_set_header(c, "Content-Type", "application/json");
        esp_http_client_set_post_field(c, payload, static_cast<int>(strlen(payload)));
    }
    const esp_err_t err = esp_http_client_perform(c);
    const int status = (err == ESP_OK && !out.overflow) ? esp_http_client_get_status_code(c) : 0;
    esp_http_client_cleanup(c);
    return status;
}
static void FadimeConfigTask(void*) {
    const char* next = "https://panel.haklio.com/device-api.php?action=next_config";
    const char* ack = "https://panel.haklio.com/device-api.php?action=ack_config";
    for (;;) {
        wifi_ap_record_t ap = {};
        std::string id, token;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK && FadimeConfigCredentials(id, token)) {
            FadimeResponse response;
            if (FadimeRequest(id, token, next, nullptr, response) == 200) {
                cJSON* root = cJSON_Parse(response.body.c_str());
                cJSON* config = root ? cJSON_GetObjectItemCaseSensitive(root, "config") : nullptr;
                if (cJSON_IsObject(config)) {
                    const cJSON* number = cJSON_GetObjectItemCaseSensitive(config, "id");
                    const cJSON* settings = cJSON_GetObjectItemCaseSensitive(config, "settings");
                    if (cJSON_IsNumber(number) && number->valuedouble >= 1 &&
                        number->valuedouble <= 2147483647 && cJSON_IsObject(settings)) {
                        bool applied = false;
                        const cJSON* screen = cJSON_GetObjectItemCaseSensitive(settings, "screen");
                        const cJSON* brightness = cJSON_GetObjectItemCaseSensitive(settings, "brightness");
                        if (cJSON_GetArraySize(settings) == 1 && cJSON_IsNumber(brightness) &&
                            brightness->valuedouble >= 0 && brightness->valuedouble <= 100 &&
                            brightness->valuedouble == brightness->valueint) {
                            Backlight* b = Board::GetInstance().GetBacklight();
                            if (b) { b->SetBrightness(static_cast<uint8_t>(brightness->valueint)); applied = true; }
                        } else if (cJSON_GetArraySize(settings) == 1 && cJSON_IsObject(screen) &&
                                   cJSON_GetArraySize(screen) == 4) {
                            const cJSON* theme = cJSON_GetObjectItemCaseSensitive(screen,"theme");
                            const cJSON* emotion = cJSON_GetObjectItemCaseSensitive(screen,"emotion");
                            const cJSON* message = cJSON_GetObjectItemCaseSensitive(screen,"message");
                            const cJSON* level = cJSON_GetObjectItemCaseSensitive(screen,"brightness");
                            if (cJSON_IsString(theme) && cJSON_IsString(emotion) &&
                                cJSON_IsString(message) && cJSON_IsNumber(level) &&
                                level->valuedouble >= 1 && level->valuedouble <= 100 &&
                                level->valuedouble == level->valueint &&
                                strlen(message->valuestring) <= 72 &&
                                (strcmp(theme->valuestring,"light")==0 || strcmp(theme->valuestring,"dark")==0) &&
                                (strcmp(emotion->valuestring,"happy")==0 || strcmp(emotion->valuestring,"neutral")==0 ||
                                 strcmp(emotion->valuestring,"thinking")==0 || strcmp(emotion->valuestring,"surprised")==0)) {
                                auto* display = Board::GetInstance().GetDisplay();
                                auto* backlight = Board::GetInstance().GetBacklight();
                                auto* t = LvglThemeManager::GetInstance().GetTheme(theme->valuestring);
                                if (display && backlight && t) {
                                    display->SetTheme(t);
                                    display->SetEmotion(emotion->valuestring);
                                    display->SetChatMessage("assistant",message->valuestring);
                                    backlight->SetBrightness(static_cast<uint8_t>(level->valueint));
                                    applied = true;
                                }
                            }
                        }
                        char body[100];
                        snprintf(body, sizeof(body), "{\"id\":%d,\"status\":\"%s\"}",
                                 number->valueint, applied ? "applied" : "failed");
                        FadimeResponse ignored;
                        FadimeRequest(id, token, ack, body, ignored);
                    }
                }
                cJSON_Delete(root);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
static void FadimeStartConfigPolling() {
    xTaskCreate(FadimeConfigTask, "fadime_config", 7168, nullptr, 2, nullptr);
}
