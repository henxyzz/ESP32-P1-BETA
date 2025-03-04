
# ESP32 Telegram Bot dengan Fitur MAC Changer

## Fitur
- Scan WiFi networks
- Ganti MAC address ESP32
- Ping host/website
- Connect ke WiFi melalui Telegram
- Fitur Forgot WiFi

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
- Tombol "Scan WiFi" - Memindai jaringan WiFi di sekitar
- Tombol "Ganti MAC" - Mengubah MAC address ESP32
- Tombol "Ping" - Mengirim ping ke host tertentu
- Tombol "Connect WiFi" - Menghubungkan ESP32 ke jaringan WiFi baru
- Tombol "Forgot WiFi" - Mereset pengaturan WiFi ke default

## Petunjuk Ganti MAC Address
Format MAC address: XX:XX:XX:XX:XX:XX (contoh: 12:34:56:78:9A:BC)
