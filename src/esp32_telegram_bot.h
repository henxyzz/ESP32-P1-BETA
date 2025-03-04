
#ifndef ESP32_TELEGRAM_BOT_H
#define ESP32_TELEGRAM_BOT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32Ping.h>

// Function declarations
void setupWiFi();
bool startRepeaterMode(String ssid, String password, String ap_ssid, String ap_password);
void stopRepeaterMode();
bool changeMACAddress(const uint8_t* newMac);
bool pingHost(const char* host);
String createKeyboard();
void handleNewMessages(int numNewMessages);
void setup();
void loop();

#endif // ESP32_TELEGRAM_BOT_H
