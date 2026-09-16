/**
 * @file BLEManager.cpp
 * @brief Implementation of the BLE scanning and UART server subsystem.
 */

#include "BLEManager.h"
#include "config.h"
#include "Logger.h"
#include "RelayController.h"
#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>

// Standard Nordic UART Service UUIDs
#define UART_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// The minimum acceptable Received Signal Strength Indicator to trigger an action
const int RSSI_SOGLIA = -1000;

static NimBLEScan* pBLEScan = nullptr;
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static bool isClientConnected = false;

/**
 * @class ServerCallbacks
 * @brief Handles connection events for the BLE Server.
 */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        isClientConnected = true;
        Logger::sysLog("INFO", "BLE", "Client connected (MAC: %s).", connInfo.getAddress().toString().c_str());
        // Resume advertising so other devices can discover the server if needed
        NimBLEDevice::startAdvertising();
    }
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        isClientConnected = false;
        Logger::sysLog("INFO", "BLE", "Client disconnected. Reason code: %d", reason);
        NimBLEDevice::startAdvertising();
    }
} serverCallbacks;

/**
 * @class RxCallbacks
 * @brief Handles incoming data written to the RX characteristic by a BLE client.
 */
class RxCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override {
        String rxValue = pCharacteristic->getValue().c_str();
        rxValue.trim();
        
        // Simple command parser for administrative tasks over BLE
        if (rxValue == "RESTART") {
            Logger::sysLog("INFO", "BLE", "RESTART command received via BLE. Rebooting...");
            delay(1000);
            ESP.restart();
        }
    }
} rxCallbacks;

/**
 * @class ScanCallbacks
 * @brief Evaluates discovered BLE devices in real-time during a scanning window.
 */
class ScanCallbacks: public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* device) override {
        // Halt processing immediately if the relay is currently active to save CPU cycles
        if (RelayController::isActive()) return; 

        // Check if the payload contains manufacturer-specific data typical of iBeacons (Apple = 0x004C)
        if (device->haveManufacturerData()) {
            std::string strManufacturerData = device->getManufacturerData();
            
            if (strManufacturerData.length() >= 25 && strManufacturerData[0] == 0x4C && strManufacturerData[1] == 0x00) {
                // Parse the raw bytes into a structured iBeacon object
                NimBLEBeacon oBeacon = NimBLEBeacon();
                oBeacon.setData(reinterpret_cast<const uint8_t*>(strManufacturerData.data()), 25);
                
                std::string beaconUUID = oBeacon.getProximityUUID().toString();
                
                if (beaconUUID == TARGET_UUID) {
                    // Target beacon detected. Evaluate operational state.
                    if (RelayController::isInCooldown()) {
                        // Throttled logging to prevent serial flooding during cooldown periods
                        static unsigned long lastCooldownLog = 0;
                        if (millis() - lastCooldownLog > 2000) {
                            Logger::sysLog("WARN", "LOGIC", "Target UUID verified, but ignored (COOLDOWN ACTIVE). RSSI: %d", device->getRSSI());
                            lastCooldownLog = millis();
                        }
                    } else {
                        Logger::sysLog("INFO", "LOGIC", "Target UUID verified | Current RSSI: %d | Threshold: %d", device->getRSSI(), RSSI_SOGLIA);
                        
                        // Evaluate proximity threshold
                        if (device->getRSSI() > RSSI_SOGLIA) {
                            RelayController::trigger("BLE");
                        } else {
                            static unsigned long lastWeakSignalLog = 0;
                            if (millis() - lastWeakSignalLog > 2000) {
                                Logger::sysLog("WARN", "LOGIC", "Signal too weak. Move device closer.");
                                lastWeakSignalLog = millis();
                            }
                        }
                    }
                }
            }
        }
    }
} scanCallbacks;


void BLEManager::init() {
    // Initialize the NimBLE stack with the visible device name
    NimBLEDevice::init("AutoPass-Gate"); 
    setupUARTServer();
    setupScanner();
}

void BLEManager::setupUARTServer() {
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
    
    NimBLEService* pUartService = pServer->createService(UART_SERVICE_UUID);
    
    // Configure the TX characteristic (Device -> Client) with Notification properties
    pTxCharacteristic = pUartService->createCharacteristic(
        UART_CHAR_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );
    
    // Configure the RX characteristic (Client -> Device) with Write properties
    NimBLECharacteristic* pRxCharacteristic = pUartService->createCharacteristic(
        UART_CHAR_RX_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    pRxCharacteristic->setCallbacks(&rxCallbacks);
    
    pUartService->start();
    
    // Begin broadcasting presence
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->start(); 
}

void BLEManager::setupScanner() {
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(&scanCallbacks, true); 
    
    // Use passive scanning to conserve energy as we only need manufacturer data
    pBLEScan->setActiveScan(false); 
    
    // Configure scanning duty cycle. 
    // Interval 250ms, window 30ms means the radio scans for 30ms every 250ms.
    // This low duty cycle ensures the Wi-Fi radio has sufficient airtime to operate concurrently.
    pBLEScan->setInterval(250); 
    pBLEScan->setWindow(30); 
    
    // Prevent internal memory accumulation of scanned devices (we process them on-the-fly)
    pBLEScan->setMaxResults(0); 
    
    startScanning();
}

void BLEManager::startScanning() {
    if (pBLEScan && !pBLEScan->isScanning()) {
        // Param 0 = scan continuously
        if(pBLEScan->start(0, false)) {
            Logger::sysLog("INFO", "SYS", "Continuous BLE scanning started. Waiting for Beacon...");
        } else {
            Logger::sysLog("ERR", "SYS", "Failed to start BLE scan!");
        }
    }
}

void BLEManager::stopScanning() {
    if (pBLEScan && pBLEScan->isScanning()) {
        pBLEScan->stop();
        Logger::sysLog("INFO", "SYS", "BLE scanning halted.");
    }
}

bool BLEManager::isScanning() {
    return pBLEScan && pBLEScan->isScanning();
}

void BLEManager::sendLog(const char* msg) {
    if (pTxCharacteristic && isClientConnected) {
        std::string strMsg(msg);
        strMsg += "\n";
        pTxCharacteristic->setValue((uint8_t*)strMsg.c_str(), strMsg.length());
        pTxCharacteristic->notify();
    }
}
