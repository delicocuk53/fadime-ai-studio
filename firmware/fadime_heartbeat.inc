// Fadime AI Studio optional heartbeat. No credentials embedded in binary.
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <cstdio>

static constexpr const char* FDM_URL =
    "https://panel.haklio.com/device-api.php?action=heartbeat";

static bool FadimeGetCredentials(std::string& id, std::string& token) {
    nvs_handle_t h;
    if (nvs_open("fadime", NVS_READONLY, &h) != ESP_OK) return false;
    auto get = [&](const char* key, std::string& value) {
        size_t n = 0;
        if (nvs_get_str(h, key, nullptr, &n) != ESP_OK || n < 2 || n > 256) return false;
        value.resize(n);
        if (nvs_get_str(h, key, value.data(), &n) != ESP_OK) return false;
        value.resize(n - 1);
        return true;
    };
    bool ok = get("device_id", id) && get("device_token", token);
    nvs_close(h);
    return ok;
}

static void FadimeHeartbeatTask(void*) {
    while (true) {
        wifi_ap_record_t ap = {};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            std::string id, token;
            if (FadimeGetCredentials(id, token)) {
                esp_http_client_config_t cfg = {};
                cfg.url = FDM_URL;
                cfg.method = HTTP_METHOD_POST;
                cfg.timeout_ms = 8000;
                cfg.crt_bundle_attach = esp_crt_bundle_attach;
                esp_http_client_handle_t client = esp_http_client_init(&cfg);
                if (client) {
                    esp_http_client_set_header(client, "Content-Type", "application/json");
                    esp_http_client_set_header(client, "X-Device-ID", id.c_str());
                    esp_http_client_set_header(client, "X-Device-Token", token.c_str());
                    static const char payload[] = "{\"firmware\":\"fadime-test\"}";
                    esp_http_client_set_post_field(client, payload, sizeof(payload) - 1);
                    esp_err_t result = esp_http_client_perform(client);
                    if (result == ESP_OK) {
                        ESP_LOGI("FadimePanel", "Heartbeat HTTP %d",
                                 esp_http_client_get_status_code(client));
                    } else {
                        ESP_LOGW("FadimePanel", "Heartbeat failed (%s)", esp_err_to_name(result));
                    }
                    esp_http_client_cleanup(client);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

static void FadimeStartHeartbeat() {
    xTaskCreate(FadimeHeartbeatTask, "fadime_panel", 6144, nullptr, 2, nullptr);
}
