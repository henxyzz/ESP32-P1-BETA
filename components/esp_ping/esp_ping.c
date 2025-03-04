
#include "esp_ping.h"
#include "ping/ping_sock.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "esp_log.h"

static const char *TAG = "ESP_PING";

typedef struct {
    uint32_t received;
    uint32_t transmitted;
    bool done;
} ping_result_t;

static void ping_end(esp_ping_handle_t hdl, void *args)
{
    ping_result_t *result = (ping_result_t *)args;
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));

    ESP_LOGI(TAG, "Ping statistics: %d packets transmitted, %d received, time %dms",
             transmitted, received, total_time_ms);

    result->received = received;
    result->transmitted = transmitted;
    result->done = true;
}

esp_err_t esp_ping_simple(const char *host_ip, uint32_t count, uint32_t interval_ms, 
                         uint32_t timeout_ms, uint32_t *received)
{
    ip_addr_t target_addr;
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));

    // Resolve hostname
    int err = getaddrinfo(host_ip, NULL, &hint, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        if (res) {
            freeaddrinfo(res);
        }
        return ESP_FAIL;
    }

    // Convert IP address
    if (res->ai_family == AF_INET) {
        struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
        inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    } else {
        ESP_LOGE(TAG, "Unsupported address family: %d", res->ai_family);
        freeaddrinfo(res);
        return ESP_FAIL;
    }
    freeaddrinfo(res);

    // Set up ping configuration
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.count = count;
    ping_config.interval_ms = interval_ms;
    ping_config.timeout_ms = timeout_ms;

    // Set up ping callbacks
    ping_result_t result = {0};
    esp_ping_callbacks_t cbs = {
        .on_ping_success = NULL,
        .on_ping_timeout = NULL,
        .on_ping_end = ping_end,
        .cb_args = &result
    };

    // Start ping session
    esp_ping_handle_t ping;
    esp_err_t ret = esp_ping_new_session(&ping_config, &cbs, &ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ping session");
        return ret;
    }

    // Start pinging
    ret = esp_ping_start(ping);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ping");
        esp_ping_delete_session(ping);
        return ret;
    }

    // Wait for ping to complete
    while (!result.done) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Clean up
    esp_ping_delete_session(ping);

    // Return results
    if (received) {
        *received = result.received;
    }

    return ESP_OK;
}
