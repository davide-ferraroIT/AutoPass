/**
 * @file NetworkManager.cpp
 * @brief Implementation of the network management subsystem.
 */

#include "NetworkManager.h"
#include "config.h"
#include "Logger.h"
#include "CloudManager.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>
#include <WebServer.h>

static WebServer server(80);

// Static member initialization
bool NetworkManager::_isNetworkServicesStarted = false;
unsigned long NetworkManager::_lastWifiRetry = 0;
bool NetworkManager::_wasConnected = false;
NetworkManager::WifiStateCallback NetworkManager::_wifiStateCallback = nullptr;

void NetworkManager::setWifiStateCallback(WifiStateCallback callback) {
    _wifiStateCallback = callback;
}

void NetworkManager::init() {
    // Configure ESP32 as a standard Wi-Fi station
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    // Explicitly set TX power to balance range and thermal/power performance
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    Serial.print("[WIFI] Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);
    
    // Block execution for a maximum of 15 seconds waiting for the initial connection.
    // This is crucial as initializing BLE simultaneously degrades Wi-Fi radio performance.
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        _wasConnected = true;
    }
}

void NetworkManager::startNetworkServices() {
    Serial.println("\n[WIFI] Network IP acquired!");
    
    // --- Configure Over-The-Air (OTA) Updates ---
    ArduinoOTA.onStart([]() { 
        Serial.println("[ArduinoOTA] Starting update..."); 
        TelnetStream.println("[ArduinoOTA] Starting update..."); 
    });
    ArduinoOTA.onEnd([]() { 
        Serial.println("\n[ArduinoOTA] Update completed successfully!"); 
        TelnetStream.println("\n[ArduinoOTA] Update completed successfully!"); 
    });
    ArduinoOTA.onError([](ota_error_t error) { 
        Serial.printf("[ArduinoOTA] Error encountered [%u]\n", error); 
        TelnetStream.printf("[ArduinoOTA] Error encountered [%u]\n", error); 
    });
    ArduinoOTA.begin();
    
    // Initialize Telnet stream for remote debug logging
    TelnetStream.begin();
    
    // Cloud initialization requires an active network connection
    CloudManager::init(); 
    
    // --- Configure Administrative Web Server ---
    server.on("/restart", HTTP_GET, []() {
        server.send(200, "text/plain", "Restart command received. Rebooting ESP32...");
        delay(1000);
        ESP.restart();
    });
    server.begin();

    _isNetworkServicesStarted = true;
    Logger::sysLog("INFO", "SYS", "Network services (OTA + SinricPro + WebServer) started successfully.");
}

void NetworkManager::handle() {
    bool currentConnected = (WiFi.status() == WL_CONNECTED);
    
    // --- Disconnected State Handling ---
    if (!currentConnected) {
        // Detect transition from Connected -> Disconnected
        if (_wasConnected) {
            _wasConnected = false;
            // Notify external modules (e.g., to suspend BLE scanning)
            if (_wifiStateCallback) _wifiStateCallback(false);
        }

        // Implement an exponential or fixed backoff for reconnection attempts (currently 60s)
        if (_lastWifiRetry == 0 || millis() - _lastWifiRetry > 60000) {
            Logger::sysLog("WARN", "WIFI", "Connection lost. Attempting to reconnect...");
            
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            
            // Block temporarily to allow the radio stack to negotiate
            unsigned long retryStart = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - retryStart < 10000) {
                delay(500);
                Serial.print(".");
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Logger::sysLog("INFO", "WIFI", "Successfully reconnected!");
                _wasConnected = true;
                if (_wifiStateCallback) _wifiStateCallback(true);
            } else {
                Logger::sysLog("ERR", "WIFI", "Reconnection attempt failed. Retrying in 60 seconds.");
            }
            
            _lastWifiRetry = millis();
        }
    } 
    // --- Connected State Handling ---
    else {
        // Detect transition from Disconnected -> Connected
        if (!_wasConnected) {
            _wasConnected = true;
            if (_wifiStateCallback) _wifiStateCallback(true);
        }

        // Lazy initialization of network-dependent services
        if (!_isNetworkServicesStarted) {
            startNetworkServices();
        }
        
        // Service external requests
        ArduinoOTA.handle();
        CloudManager::handle();
        server.handleClient();
    }
}

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
