// Fadime USB enrollment experiment. Requires USB console stdin routing.
#include <nvs.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdio>
#include <cstring>
#include <cctype>

static bool FadimeValidHex(const char* s, size_t len) {
    if (strlen(s) != len) return false;
    for (size_t i=0;i<len;i++)
        if (!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f'))) return false;
    return true;
}
static bool FadimeValidId(const char* s) {
    if (strlen(s) != 19 || strncmp(s,"fadime-",7) != 0) return false;
    return FadimeValidHex(s+7,12);
}
static void FadimeSerialPairTask(void*) {
    char line[160];
    for (;;) {
        if (!fgets(line, sizeof(line), stdin)) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        char id[40] = {}, key[80] = {}, extra[2] = {};
        if (sscanf(line, "FADIME_PAIR %39s %79s %1s",id,key,extra) != 2
            || !FadimeValidId(id) || !FadimeValidHex(key,64)) {
            if (strncmp(line,"FADIME_PAIR",11)==0) puts("FADIME_PAIR_ERROR format");
            continue;
        }
        nvs_handle_t h;
        esp_err_t status = nvs_open("fadime",NVS_READWRITE,&h);
        if (status == ESP_OK) {
            status = nvs_set_str(h,"device_id",id);
            if (status == ESP_OK) status = nvs_set_str(h,"device_token",key);
            if (status == ESP_OK) status = nvs_commit(h);
            nvs_close(h);
        }
        // Never echo credential or log it.
        puts(status == ESP_OK ? "FADIME_PAIR_OK" : "FADIME_PAIR_ERROR nvs");
        fflush(stdout);
        memset(key,0,sizeof(key));
        memset(line,0,sizeof(line));
    }
}
static void FadimeStartUsbPairing() {
    xTaskCreate(FadimeSerialPairTask, "fadime_usb_pair", 4096, nullptr, 2, nullptr);
}
