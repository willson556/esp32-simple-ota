#include "ota.hpp"

#include <cstdint>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h>

#include <esp_https_ota.h>
#include <cJSON.h>

static const char *TAG = "ota";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

namespace {

template <TickType_t milliseconds>
void delay()
{
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
}

char* copy_response(esp_http_client_handle_t client, size_t content_length) {
    char* buffer = new char[content_length + 1];

    if (buffer) {
        int read_len = esp_http_client_read(client, buffer, content_length);
        buffer[read_len] = 0;
    }

    return buffer;
}

} // namespace

namespace HttpsOta {

esp_err_t ota_http_event_handler(esp_http_client_event_t *e) {
    return 0;
}

void Ota::thread_func() {
    while (thread_running) {
        constexpr size_t url_buffer_size{256};
        char url[url_buffer_size]{0};

        if (update_available(url, url_buffer_size)) {
            install_update(url);
        }

        delay<1 * 60 * 1000>(); // Check for update every 10 minutes
    }
}

bool Ota::update_available(char* url, size_t url_buffer_size) {
    bool update_found {false};

    esp_http_client_config_t config {0};
    config.url = feed_url;
    config.cert_pem = (char *)server_cert_pem_start;
    config.event_handler = ota_http_event_handler;

    esp_http_client_handle_t client {esp_http_client_init(&config)};
    const esp_err_t err {esp_http_client_perform(client)};

    if (err == ESP_OK) {
        const int status_code {esp_http_client_get_status_code(client)};
        const int content_length {esp_http_client_get_content_length(client)};

        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                status_code,
                content_length);

        if (status_code == 200 && content_length != 0) {
            char *response {copy_response(client, content_length)};

            if (response) {
                cJSON *feed {cJSON_Parse(response)};

                if (feed) {
                    const cJSON *feed_version {cJSON_GetObjectItem(feed, "version")};

                    if (cJSON_IsString(feed_version) && feed_version->valuestring != nullptr) {
                        ESP_LOGD(TAG, "Current Version: %s, Feed Version: %s", installed_version, feed_version->valuestring);

                        if (strcmp(feed_version->valuestring, installed_version)) {
                            ESP_LOGD(TAG, "Update available!");
                            const cJSON *feed_update_url {cJSON_GetObjectItem(feed, "update-url")};

                            if (cJSON_IsString(feed_update_url)
                                    && feed_update_url->valuestring != nullptr
                                    && strlen(feed_update_url->valuestring) < url_buffer_size)
                            {
                                strcpy(url, feed_update_url->valuestring);
                                ESP_LOGD(TAG, "Update URL: %s", url);
                                update_found = true;
                            }
                            else {
                                ESP_LOGE(TAG, "Update URL is too long or not found!");
                            }
                        } else {
                            ESP_LOGD(TAG, "Firmware is up to date!");
                        }
                    }

                    cJSON_Delete(feed);
                } else {
                    ESP_LOGE(TAG, "Failed to parse response:\n\n%s\n\n", response);
                }

                delete[] response;
            } else {
                ESP_LOGE(TAG, "Couldn't allocate space to copy response!");
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    return update_found;    
}

void Ota::install_update(const char *url) {
    esp_http_client_config_t config = {0};
    config.url = url;
    config.cert_pem = (char *)server_cert_pem_start;
    config.event_handler = ota_http_event_handler;

    if (preUpdateCallback) {
        preUpdateCallback();
    }

    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
}

}