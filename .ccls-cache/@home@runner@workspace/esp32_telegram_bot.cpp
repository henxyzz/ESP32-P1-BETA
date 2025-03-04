
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <Ping.h>

// Konfigurasi Default WiFi
const char* default_ssid = "DIRECT-NS-H1";
const char* default_password = "87654321";
const char* proxy_address = "192.168.49.1";
const int proxy_port = 8282;

// Konfigurasi Telegram Bot
#define BOT_TOKEN "GANTI_DENGAN_TOKEN_BOT_ANDA"

// Alokasi EEPROM untuk menyimpan MAC address
#define EEPROM_SIZE 512
#define MAC_ADDR_OFFSET 0 // Posisi awal untuk menyimpan MAC address

// Variabel global
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime = 0;
const unsigned long BOT_MTBS = 1000; // waktu pengecekan pesan baru (1 detik)

// Struktur untuk menyimpan MAC address
struct {
  uint8_t mac[6];
  bool valid;
} saved_mac;

// Fungsi untuk menginisialisasi koneksi WiFi dengan proxy
void setupWiFi() {
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(default_ssid, default_password);
  
  // Tunggu koneksi
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nTerhubung ke WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Setup proxy
    HTTPClient::setProxyIP(proxy_address);
    HTTPClient::setProxyPort(proxy_port);
    Serial.println("Proxy diaktifkan");
  } else {
    Serial.println("\nGagal terhubung ke WiFi!");
  }
}

// Fungsi untuk mengubah MAC address
bool changeMACAddress(const uint8_t* newMac) {
  esp_err_t result;
  
  // Matikan WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Atur MAC address baru
  result = esp_wifi_set_mac(WIFI_IF_STA, newMac);
  if (result != ESP_OK) {
    Serial.println("Gagal mengubah MAC address");
    return false;
  }
  
  // Simpan MAC address ke EEPROM
  EEPROM.put(MAC_ADDR_OFFSET, newMac);
  EEPROM.commit();
  
  // Aktifkan WiFi kembali
  WiFi.mode(WIFI_STA);
  setupWiFi();
  
  return true;
}

// Fungsi untuk mengirim ping ke host tertentu
bool pingHost(const char* host) {
  bool success = Ping.ping(host, 3);
  return success;
}

// Fungsi untuk membuat keyboard inline di Telegram
String createKeyboard() {
  String keyboard = "[";
  keyboard += "[{\"text\":\"Scan WiFi\", \"callback_data\":\"scan\"}],";
  keyboard += "[{\"text\":\"Ganti MAC\", \"callback_data\":\"macchange\"}],";
  keyboard += "[{\"text\":\"Ping\", \"callback_data\":\"ping\"}],";
  keyboard += "[{\"text\":\"Connect WiFi\", \"callback_data\":\"connect\"}],";
  keyboard += "[{\"text\":\"Forgot WiFi\", \"callback_data\":\"forgot\"}]";
  keyboard += "]";
  return keyboard;
}

// Fungsi untuk menangani pesan baru Telegram
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    if (text == "/start") {
      String welcome = "Selamat datang, " + from_name + "!\n";
      welcome += "Gunakan tombol berikut untuk mengontrol ESP32:\n";
      
      bot.sendMessageWithInlineKeyboard(chat_id, welcome, "", createKeyboard());
    }
    
    if (text == "/scan" || bot.messages[i].callback_query_data == "scan") {
      String scanResponse = "Memulai pemindaian WiFi...\n\n";
      int networksFound = WiFi.scanNetworks();
      
      if (networksFound == 0) {
        scanResponse += "Tidak ada jaringan yang ditemukan.";
      } else {
        scanResponse += "Ditemukan " + String(networksFound) + " jaringan:\n\n";
        for (int j = 0; j < networksFound; j++) {
          scanResponse += (j + 1) + ": " + WiFi.SSID(j) + " (";
          scanResponse += WiFi.RSSI(j) + "dBm) ";
          scanResponse += (WiFi.encryptionType(j) == WIFI_AUTH_OPEN) ? "terbuka" : "terenkripsi";
          scanResponse += "\n";
        }
      }
      
      bot.sendMessage(chat_id, scanResponse, "");
      bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
    }
    
    if (text == "/macchange" || bot.messages[i].callback_query_data == "macchange") {
      bot.sendMessage(chat_id, "Masukkan MAC address baru dalam format XX:XX:XX:XX:XX:XX", "");
      // Bot akan menunggu pesan berikutnya dengan MAC address
    } else if (text.length() == 17 && text.indexOf(":") != -1) {
      // Format MAC address valid (XX:XX:XX:XX:XX:XX)
      uint8_t newMac[6];
      int values[6];
      
      if (sscanf(text.c_str(), "%x:%x:%x:%x:%x:%x", 
                &values[0], &values[1], &values[2], 
                &values[3], &values[4], &values[5]) == 6) {
        
        for (int j = 0; j < 6; j++) {
          newMac[j] = (uint8_t)values[j];
        }
        
        if (changeMACAddress(newMac)) {
          String currentMac = WiFi.macAddress();
          bot.sendMessage(chat_id, "MAC address berhasil diubah menjadi: " + currentMac, "");
        } else {
          bot.sendMessage(chat_id, "Gagal mengubah MAC address!", "");
        }
        bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
      }
    }
    
    if (text == "/ping" || bot.messages[i].callback_query_data == "ping") {
      bot.sendMessage(chat_id, "Masukkan host yang ingin di-ping (contoh: google.com):", "");
      // Bot akan menunggu pesan berikutnya dengan host
    } else if (text.indexOf(".") != -1 && !text.startsWith("/")) {
      if (pingHost(text.c_str())) {
        bot.sendMessage(chat_id, "Ping ke " + text + " berhasil!", "");
      } else {
        bot.sendMessage(chat_id, "Ping ke " + text + " gagal.", "");
      }
      bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
    }
    
    if (text == "/connect" || bot.messages[i].callback_query_data == "connect") {
      bot.sendMessage(chat_id, "Masukkan SSID dan password WiFi dalam format SSID,PASSWORD", "");
      // Bot akan menunggu pesan berikutnya dengan SSID dan password
    } else if (text.indexOf(",") != -1 && !text.startsWith("/")) {
      String ssid = text.substring(0, text.indexOf(","));
      String password = text.substring(text.indexOf(",") + 1);
      
      bot.sendMessage(chat_id, "Mencoba menghubungkan ke " + ssid + "...", "");
      
      WiFi.disconnect();
      WiFi.begin(ssid.c_str(), password.c_str());
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        bot.sendMessage(chat_id, "Berhasil terhubung ke " + ssid + "!\nIP: " + WiFi.localIP().toString(), "");
      } else {
        bot.sendMessage(chat_id, "Gagal terhubung ke " + ssid, "");
        setupWiFi(); // Hubungkan kembali ke WiFi default
      }
      bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
    }
    
    if (text == "/forgot" || bot.messages[i].callback_query_data == "forgot") {
      WiFi.disconnect(true);
      delay(1000);
      setupWiFi(); // Hubungkan kembali ke WiFi default
      bot.sendMessage(chat_id, "Pengaturan WiFi direset. Terhubung kembali ke SSID default.", "");
      bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
    }
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  
  // Inisialisasi WiFi
  WiFi.mode(WIFI_STA);
  
  // Cek apakah ada MAC address tersimpan di EEPROM
  EEPROM.get(MAC_ADDR_OFFSET, saved_mac);
  if (saved_mac.valid) {
    esp_wifi_set_mac(WIFI_IF_STA, saved_mac.mac);
    Serial.println("Menggunakan MAC address tersimpan");
  }
  
  // Koneksi ke WiFi
  setupWiFi();
  
  // Konfigurasi klien SSL
  secured_client.setCACert(nullptr); // Nonaktifkan verifikasi SSL untuk sementara
  
  Serial.println("ESP32 Bot Telegram Siap!");
}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    
    if (numNewMessages) {
      handleNewMessages(numNewMessages);
    }
    
    bot_lasttime = millis();
  }
}
