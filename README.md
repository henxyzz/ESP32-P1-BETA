
# ESP32 Telegram Bot dengan Fitur MAC Changer dan WiFi Repeater

## Fitur
- Scan WiFi networks dengan opsi tombol untuk terhubung
- Ganti MAC address ESP32
- Ping host/website
- Connect ke WiFi melalui Telegram
- Fitur Forgot WiFi
- Mode Repeater WiFi - ESP32 bertindak sebagai WiFi repeater

## Pengaturan Awal
1. ESP32 akan terhubung ke SSID "DIRECT-NS-H1" dengan password "87654321"
2. Menggunakan proxy 192.168.49.1:8282 untuk terhubung ke internet
3. Semua konfigurasi dilakukan melalui bot Telegram

## Cara Penggunaan
1. Flash kode ke ESP32
2. ESP32 akan terhubung ke WiFi default
3. Mulai chat dengan bot Telegram
4. Gunakan tombol yang tersedia di bot untuk mengontrol ESP32

## Perintah Bot Telegram
- `/start` - Memulai bot dan menampilkan menu utama
- Tombol "Scan WiFi" - Memindai jaringan WiFi di sekitar dengan opsi untuk terhubung
- Tombol "Ganti MAC" - Mengubah MAC address ESP32
- Tombol "Ping" - Mengirim ping ke host tertentu
- Tombol "Connect WiFi" - Menghubungkan ESP32 ke jaringan WiFi baru
- Tombol "Start Repeater" - Mengaktifkan mode repeater WiFi
- Tombol "Stop Repeater" - Menghentikan mode repeater WiFi
- Tombol "Forgot WiFi" - Mereset pengaturan WiFi ke default

## Petunjuk Ganti MAC Address
Format MAC address: XX:XX:XX:XX:XX:XX (contoh: 12:34:56:78:9A:BC)

## Petunjuk Mode Repeater WiFi
Format konfigurasi: SSID_UPSTREAM,PASSWORD_UPSTREAM,AP_SSID,AP_PASSWORD

Contoh: MyWifi,mypass123,ESP32-AP,12345678

## Mode Operasi
- Mode Normal: ESP32 terhubung ke WiFi sebagai client
- Mode Repeater: ESP32 terhubung ke WiFi upstream dan membuat AP sendiri
