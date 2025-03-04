
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Melakukan ping ke host tertentu
 *
 * @param host_ip Alamat IP atau nama host
 * @param count Jumlah ping yang akan dikirim
 * @param interval_ms Interval antara masing-masing ping (dalam ms)
 * @param timeout_ms Waktu tunggu untuk respon
 * @param received Pointer ke variabel yang akan menyimpan jumlah ping yang diterima
 * @return esp_err_t ESP_OK jika berhasil
 */
esp_err_t esp_ping_simple(const char *host_ip, uint32_t count, uint32_t interval_ms, 
                         uint32_t timeout_ms, uint32_t *received);

#ifdef __cplusplus
}
#endif
