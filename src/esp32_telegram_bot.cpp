
#include "esp32_telegram_bot.h"

// Konfigurasi Default WiFi
const char* default_ssid = "DIRECT-NS-H1";
const char* default_password = "87654321";
const char* proxy_address = "192.168.49.1";
const int proxy_port = 8282;

// Konfigurasi Telegram Bot
#define BOT_TOKEN "GANTI_DENGAN_TOKEN_BOT_ANDA"

// Alokasi EEPROM untuk menyimpan MAC address dan konfigurasi repeater
#define EEPROM_SIZE 512
#define MAC_ADDR_OFFSET 0 // Posisi awal untuk menyimpan MAC address
#define REPEATER_CONFIG_OFFSET 100 // Posisi awal untuk menyimpan konfigurasi repeater

// Mode repeater
bool repeaterMode = false;
String repeaterSSID = "";
String repeaterPassword = "";
String apSSID = "ESP32-Repeater";
String apPassword = "12345678";

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

// Struktur untuk menyimpan konfigurasi repeater
struct {
  char ssid[33];        // SSID maksimum 32 karakter + null terminator
  char password[65];    // Password maksimum 64 karakter + null terminator
  char ap_ssid[33];     // SSID AP maksimum 32 karakter + null terminator
  char ap_password[65]; // Password AP maksimum 64 karakter + null terminator
  bool active;          // Status aktif repeater
} repeater_config;

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

    // Note: Direct proxy setting is not supported in HTTPClient
    // Using proxy would require manual implementation
    Serial.println("Terhubung langsung tanpa proxy");
  } else {
    Serial.println("\nGagal terhubung ke WiFi!");
  }
}

// Fungsi untuk mengaktifkan mode repeater WiFi
bool startRepeaterMode(String ssid, String password, String ap_ssid, String ap_password) {
  // Disconnect dari WiFi terlebih dahulu
  WiFi.disconnect(true);
  delay(1000);
  
  // Simpan konfigurasi repeater ke EEPROM
  strncpy(repeater_config.ssid, ssid.c_str(), 32);
  strncpy(repeater_config.password, password.c_str(), 64);
  strncpy(repeater_config.ap_ssid, ap_ssid.c_str(), 32);
  strncpy(repeater_config.ap_password, ap_password.c_str(), 64);
  repeater_config.active = true;
  
  EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
  EEPROM.commit();
  
  // Konfigurasi mode AP+STA (repeater)
  WiFi.mode(WIFI_AP_STA);
  
  // Setup Access Point
  bool ap_success = WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  if (!ap_success) {
    Serial.println("AP Mode gagal diaktifkan");
    return false;
  }
  
  Serial.print("AP Mode aktif dengan SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // Hubungkan ke WiFi upstream
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nTerhubung ke WiFi upstream!");
    Serial.print("STA IP Address: ");
    Serial.println(WiFi.localIP());
    
    repeaterMode = true;
    repeaterSSID = ssid;
    repeaterPassword = password;
    apSSID = ap_ssid;
    apPassword = ap_password;
    
    return true;
  } else {
    Serial.println("\nGagal terhubung ke WiFi upstream!");
    // Nonaktifkan mode AP
    WiFi.softAPdisconnect(true);
    // Kembali ke mode STA
    WiFi.mode(WIFI_STA);
    // Hubungkan kembali ke WiFi default
    setupWiFi();
    
    repeater_config.active = false;
    EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
    EEPROM.commit();
    
    return false;
  }
}

// Fungsi untuk menghentikan mode repeater WiFi
void stopRepeaterMode() {
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  delay(1000);
  
  repeaterMode = false;
  repeater_config.active = false;
  EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
  EEPROM.commit();
  
  // Kembali ke mode STA
  WiFi.mode(WIFI_STA);
  // Hubungkan kembali ke WiFi default
  setupWiFi();
  
  Serial.println("Mode repeater WiFi dihentikan");
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
  String keyboard = "[[{\"text\":\"Scan WiFi\", \"callback_data\":\"scan\"}],";
  keyboard += "[{\"text\":\"Ganti MAC\", \"callback_data\":\"macchange\"}],";
  keyboard += "[{\"text\":\"Ping\", \"callback_data\":\"ping\"}],";
  keyboard += "[{\"text\":\"Connect WiFi\", \"callback_data\":\"connect\"}],";
  keyboard += "[{\"text\":\"Start Repeater\", \"callback_data\":\"repeater\"}],";
  
  if (repeaterMode) {
    keyboard += "[{\"text\":\"Stop Repeater\", \"callback_data\":\"stoprepeater\"}],";
  }
  
  keyboard += "[{\"text\":\"Forgot WiFi\", \"callback_data\":\"forgot\"}]]";
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
    
    if (text == "/scan" || (bot.messages[i].type == "callback_query" && bot.messages[i].text == "scan")) {
      String scanResponse = "Memulai pemindaian WiFi...\n\n";
      int networksFound = WiFi.scanNetworks();
      
      if (networksFound == 0) {
        scanResponse += "Tidak ada jaringan yang ditemukan.";
      } else {
        scanResponse += "Ditemukan " + String(networksFound) + " jaringan:\n\n";
        
        // Membuat keyboard untuk memilih WiFi
        String keyboard = "[";
        for (int j = 0; j < networksFound; j++) {
          String ssid = WiFi.SSID(j);
          int rssi = WiFi.RSSI(j);
          bool encrypted = (WiFi.encryptionType(j) != WIFI_AUTH_OPEN);
          
          scanResponse += (j + 1) + ": " + ssid + " (";
          scanResponse += rssi + "dBm) ";
          scanResponse += encrypted ? "terenkripsi" : "terbuka";
          scanResponse += "\n";
          
          // Tambahkan tombol untuk menghubungkan ke WiFi
          if (j < 5) { // Batasi hingga 5 jaringan untuk keyboard
            keyboard += "[{\"text\":\"" + ssid + "\", \"callback_data\":\"wifi:" + ssid + "\"}],";
          }
        }
        keyboard += "[{\"text\":\"Kembali\", \"callback_data\":\"back\"}]";
        keyboard += "]";
        
        bot.sendMessage(chat_id, scanResponse, "");
        bot.sendMessageWithInlineKeyboard(chat_id, "Pilih WiFi untuk terhubung:", "", keyboard);
        return;
      }
      
      bot.sendMessage(chat_id, scanResponse, "");
      bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
    }
    
    // Handle callback untuk memilih WiFi
    if (bot.messages[i].type == "callback_query" && bot.messages[i].text.startsWith("wifi:")) {
      String selected_ssid = bot.messages[i].text.substring(5);
      bot.sendMessage(chat_id, "Anda memilih: " + selected_ssid + "\nMasukkan password (kirim 'none' jika tidak ada password):", "");
      // Simpan SSID sementara di EEPROM
      strncpy(repeater_config.ssid, selected_ssid.c_str(), 32);
      EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
      EEPROM.commit();
    }
    
    if (text == "/macchange" || (bot.messages[i].type == "callback_query" && bot.messages[i].text == "macchange")) {
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
    
    if (text == "/ping" || (bot.messages[i].type == "callback_query" && bot.messages[i].text == "ping")) {
      bot.sendMessage(chat_id, "Masukkan host yang ingin di-ping (contoh: google.com):", "");
      // Bot akan menunggu pesan berikutnya dengan host
    } else if (text.indexOf(".") != -1 && !text.startsWith("/")) {
      // Cek apakah ini respon untuk ping atau password WiFi
      EEPROM.get(REPEATER_CONFIG_OFFSET, repeater_config);
      if (strlen(repeater_config.ssid) > 0 && !text.equals("none")) {
        // Ini adalah password untuk WiFi yang dipilih sebelumnya
        String selected_ssid = String(repeater_config.ssid);
        String password = text;
        
        bot.sendMessage(chat_id, "Mencoba menghubungkan ke " + selected_ssid + "...", "");
        
        WiFi.disconnect();
        WiFi.begin(selected_ssid.c_str(), password.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500);
          attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
          bot.sendMessage(chat_id, "Berhasil terhubung ke " + selected_ssid + "!\nIP: " + WiFi.localIP().toString(), "");
          // Simpan kredensial
          strncpy(repeater_config.password, password.c_str(), 64);
          EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
          EEPROM.commit();
        } else {
          bot.sendMessage(chat_id, "Gagal terhubung ke " + selected_ssid, "");
          setupWiFi(); // Hubungkan kembali ke WiFi default
        }
        
        // Reset SSID sementara
        repeater_config.ssid[0] = '\0';
        EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
        EEPROM.commit();
        
        bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
      } else if (text.equals("none") && strlen(repeater_config.ssid) > 0) {
        // Connect tanpa password
        String selected_ssid = String(repeater_config.ssid);
        
        bot.sendMessage(chat_id, "Mencoba menghubungkan ke " + selected_ssid + " tanpa password...", "");
        
        WiFi.disconnect();
        WiFi.begin(selected_ssid.c_str(), "");
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500);
          attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
          bot.sendMessage(chat_id, "Berhasil terhubung ke " + selected_ssid + "!\nIP: " + WiFi.localIP().toString(), "");
          // Simpan kredensial
          repeater_config.password[0] = '\0';
          EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
          EEPROM.commit();
        } else {
          bot.sendMessage(chat_id, "Gagal terhubung ke " + selected_ssid, "");
          setupWiFi(); // Hubungkan kembali ke WiFi default
        }
        
        // Reset SSID sementara
        repeater_config.ssid[0] = '\0';
        EEPROM.put(REPEATER_CONFIG_OFFSET, repeater_config);
        EEPROM.commit();
        
        bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
      } else {
        // Ini adalah host untuk ping
        if (pingHost(text.c_str())) {
          bot.sendMessage(chat_id, "Ping ke " + text + " berhasil!", "");
        } else {
          bot.sendMessage(chat_id, "Ping ke " + text + " gagal.", "");
        }
        bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
      }
    }
    
    if (text == "/connect" || (bot.messages[i].type == "callback_query" && bot.messages[i].text == "connect")) {
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
    
    if (text == "/repeater" || (bot.messages[i].type == "callback_query" && bot.messages[i].text == "repeater")) {
      bot.sendMessage(chat_id, "Untuk mengaktifkan mode repeater WiFi, masukkan konfigurasi dalam format:\nSSID_UPSTREAM,PASSWORD_UPSTREAM,AP_SSID,AP_PASSWORD\n\nContoh: MyWifi,mypass123,ESP32-AP,12345678", "");
      // Bot akan menunggu pesan berikutnya dengan konfigurasi repeater
    } else if (text.indexOf(",") != -1 && text.indexOf(",", text.indexOf(",") + 1) != -1 && !text.startsWith("/")) {
      // Format valid untuk konfigurasi repeater (SSID_UPSTREAM,PASSWORD_UPSTREAM,AP_SSID,AP_PASSWORD)
      int firstComma = text.indexOf(",");
      int secondComma = text.indexOf(",", firstComma + 1);
      int thirdComma = text.indexOf(",", secondComma + 1);
      
      if (secondComma != -1 && thirdComma != -1) {
        String ssid = text.substring(0, firstComma);
        String password = text.substring(firstComma + 1, secondComma);
        String ap_ssid = text.substring(secondComma + 1, thirdComma);
        String ap_password = text.substring(thirdComma + 1);
        
        bot.sendMessage(chat_id, "Mengaktifkan mode repeater WiFi...", "");
        
        if (startRepeaterMode(ssid, password, ap_ssid, ap_password)) {
          String status = "Mode repeater WiFi aktif!\n";
          status += "Terhubung ke: " + ssid + "\n";
          status += "IP STA: " + WiFi.localIP().toString() + "\n";
          status += "SSID AP: " + ap_ssid + "\n";
          status += "IP AP: " + WiFi.softAPIP().toString();
          
          bot.sendMessage(chat_id, status, "");
        } else {
          bot.sendMessage(chat_id, "Gagal mengaktifkan mode repeater WiFi!", "");
        }
        bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
      }
    }
    
    if (bot.messages[i].type == "callback_query" && bot.messages[i].text == "stoprepeater") {
      if (repeaterMode) {
        bot.sendMessage(chat_id, "Menghentikan mode repeater WiFi...", "");
        stopRepeaterMode();
        bot.sendMessage(chat_id, "Mode repeater WiFi dihentikan dan kembali ke koneksi default.", "");
      } else {
        bot.sendMessage(chat_id, "Mode repeater WiFi tidak aktif.", "");
      }
      bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
    }
    
    if (text == "/forgot" || (bot.messages[i].type == "callback_query" && bot.messages[i].text == "forgot")) {
      WiFi.disconnect(true);
      delay(1000);
      setupWiFi(); // Hubungkan kembali ke WiFi default
      bot.sendMessage(chat_id, "Pengaturan WiFi direset. Terhubung kembali ke SSID default.", "");
      bot.sendMessageWithInlineKeyboard(chat_id, "Pilih opsi lain:", "", createKeyboard());
    }
    
    if (bot.messages[i].type == "callback_query" && bot.messages[i].text == "back") {
      bot.sendMessageWithInlineKeyboard(chat_id, "Kembali ke menu utama:", "", createKeyboard());
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
  
  // Cek apakah mode repeater aktif
  EEPROM.get(REPEATER_CONFIG_OFFSET, repeater_config);
  if (repeater_config.active) {
    Serial.println("Memulai mode repeater dari konfigurasi tersimpan...");
    String ssid = String(repeater_config.ssid);
    String password = String(repeater_config.password);
    String ap_ssid = String(repeater_config.ap_ssid);
    String ap_password = String(repeater_config.ap_password);
    
    if (startRepeaterMode(ssid, password, ap_ssid, ap_password)) {
      Serial.println("Mode repeater berhasil diaktifkan kembali");
    } else {
      Serial.println("Gagal mengaktifkan kembali mode repeater");
    }
  } else {
    // Koneksi ke WiFi
    setupWiFi();
  }
  
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
