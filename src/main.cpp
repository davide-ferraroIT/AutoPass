/**
 * @file main.cpp
 * @brief Application entry point.
 * 
 * Orchestrates the initialization of hardware and software subsystems and
 * dispatches execution time to individual module handlers within the main loop.
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "Logger.h"
#include "RelayController.h"
#include "NetworkManager.h"
#include "CloudManager.h"
#include "BLEManager.h"

// Watchdog timeout in seconds
#define WDT_TIMEOUT 15

void setup() {
    Serial.begin(115200);

    // Initialize Hardware Watchdog Timer
    esp_task_wdt_init(WDT_TIMEOUT, true); // Panic and restart on timeout
    esp_task_wdt_add(NULL);               // Subscribe the main loop task

    // 1. Initialize the Logger and bind the BLE UART transmission callback.
    // This allows the Logger to remain decoupled from the BLE implementation.
    Logger::init(BLEManager::sendLog);
    
    // Initialize hardware pins and timing logic for the Relay.
    RelayController::init();
    
    // Bind the Cloud synchronization callback. 
    // Ensures the cloud dashboard correctly reflects automatic local state changes.
    RelayController::setCloudSyncCallback(CloudManager::syncState);
    
    // Initialize Network capabilities (Wi-Fi, OTA, WebServer).
    // Note: The first connection attempt blocks execution to prevent 
    // RF interference with early BLE operations.
    NetworkManager::init();
    
    // Handle RF contention: Suspend BLE scanning whenever the Wi-Fi stack 
    // needs to perform resource-intensive reconnection procedures.
    NetworkManager::setWifiStateCallback([](bool connected) {
        if (connected) {
            BLEManager::startScanning();
        } else {
            BLEManager::stopScanning();
        }
    });

    Logger::sysLog("INFO", "SYS", "AutoPass System Initialized (iBeacon + Wi-Fi OTA)");
    
    // Initialize Bluetooth Low Energy stack (Scanner & GATT Server).
    // Executed after Wi-Fi stabilization to guarantee a clean radio environment.
    BLEManager::init();
}

void loop() {
    // Feed the Hardware Watchdog Timer
    esp_task_wdt_reset();

    // Dispatch relay timing evaluation (trigger durations, cooldowns).
    RelayController::handle();
    
    // Dispatch network task evaluation (reconnections, OTA polling, Cloud keep-alive).
    NetworkManager::handle();
    
    // Generate a periodic heartbeat log for system health observability.
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 10000) {
        Logger::sysLog("INFO", "SYS", "System online | Free Heap: %u | Min Free Heap: %u", ESP.getFreeHeap(), ESP.getMinFreeHeap());
        lastHeartbeat = millis();
    }
}