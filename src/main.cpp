#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#endif

NimBLEScan* pBLEScan;
const int PIN_OUTPUT = 26;
const int RSSI_SOGLIA = -1000;

#ifndef TARGET_UUID
#define TARGET_UUID "YOUR_TARGET_UUID"
#endif

unsigned long triggerActiveUntil = 0;
bool isRelayActive = false;

unsigned long lastTriggerTime = 0;
const unsigned long COOLDOWN_DURATION = 120000;
bool isCooldown = false;

bool isClientConnected = false;

NimBLECharacteristic* pTxCharacteristic = nullptr;
#define UART_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// --- LOGGING INTELLIGENTE ---
void sysLog(const char* level, const char* module, const char* format, ...) {
    char msgBuffer[256];
    char logBuffer[300];
    
    va_list args;
    va_start(args, format);
    vsnprintf(msgBuffer, sizeof(msgBuffer), format, args);
    va_end(args);
    
    snprintf(logBuffer, sizeof(logBuffer), "[%08lu] [%-5s] [%-6s] %s", millis(), level, module, msgBuffer);
    
    Serial.println(logBuffer);
    
    // Invia via Bluetooth SOLO SE un client UART è connesso
    if (pTxCharacteristic && isClientConnected) {
        std::string strMsg(logBuffer);
        strMsg += "\n";
        pTxCharacteristic->setValue((uint8_t*)strMsg.c_str(), strMsg.length());
        pTxCharacteristic->notify();
    }
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        isClientConnected = true;
        Serial.printf("[INFO] Client connesso (MAC: %s).\n", connInfo.getAddress().toString().c_str());
        NimBLEDevice::startAdvertising();
    }
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        isClientConnected = false;
        sysLog("INFO", "BLE", "Client disconnesso. Motivo: %d", reason);
        NimBLEDevice::startAdvertising();
    }
} serverCallbacks;

// --- CALLBACK SCANSIONE ---
class ScanCallbacks: public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* device) override {
        if (isRelayActive) return; 

        if (device->haveManufacturerData()) {
            std::string strManufacturerData = device->getManufacturerData();
            
            if (strManufacturerData.length() >= 25 && strManufacturerData[0] == 0x4C && strManufacturerData[1] == 0x00) {
                NimBLEBeacon oBeacon = NimBLEBeacon();
                oBeacon.setData(reinterpret_cast<const uint8_t*>(strManufacturerData.data()), 25);
                
                std::string beaconUUID = oBeacon.getProximityUUID().toString();
                
                if (beaconUUID == TARGET_UUID) {
                    if (isCooldown) {
                        sysLog("WARN", "LOGIC", "Target UUID identificato, ma ignorato (COOLDOWN ATTIVO). RSSI: %d", device->getRSSI());
                    } else {
                        sysLog("INFO", "LOGIC", "Target UUID verificato | RSSI attuale: %d | Soglia: %d", device->getRSSI(), RSSI_SOGLIA);
                        
                        if (device->getRSSI() > RSSI_SOGLIA) {
                            digitalWrite(PIN_OUTPUT, HIGH);
                            isRelayActive = true;
                            triggerActiveUntil = millis() + 1000;
                            lastTriggerTime = millis();
                            isCooldown = true;
                            sysLog("ACT", "RELAY", "Attivazione RELE (PIN %d) | Avvio cooldown (%lu ms)", PIN_OUTPUT, COOLDOWN_DURATION);
                            Serial.flush();
                        } else {
                            sysLog("WARN", "LOGIC", "Segnale troppo debole. Avvicinare il dispositivo.");
                        }
                    }
                }
            }
        }
    }
} scanCallbacks;

void setup() {
    disableCore0WDT();
    disableLoopWDT();
    Serial.begin(115200);
    
    pinMode(PIN_OUTPUT, OUTPUT);
    digitalWrite(PIN_OUTPUT, LOW);
    delay(100); 

    // --- CONNESSIONE WIFI & ARDUINO OTA ---
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("[WIFI] Connessione al Wi-Fi: ");
    Serial.println(WIFI_SSID);
    
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connesso con successo!");
        Serial.print("[WIFI] Indirizzo IP ESP32: ");
        Serial.println(WiFi.localIP());
        
        ArduinoOTA.onStart([]() {
            Serial.println("[ArduinoOTA] Avvio aggiornamento Wi-Fi...");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\n[ArduinoOTA] Aggiornamento Wi-Fi completato!");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[ArduinoOTA] Avanzamento: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[ArduinoOTA] Errore [%u]\n", error);
        });
        ArduinoOTA.begin();
    } else {
        Serial.println("\n[WIFI] Impossibile connettersi al Wi-Fi (Timeout).");
    }

    sysLog("INFO", "SYS", "Avvio sistema AutoPass (iBeacon + Wi-Fi OTA)...");

    NimBLEDevice::init("AutoPass-Gate"); 
    
    // Server BLE per log UART
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
    
    NimBLEService* pUartService = pServer->createService(UART_SERVICE_UUID);
    pTxCharacteristic = pUartService->createCharacteristic(
        UART_CHAR_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );
    pUartService->start();
    
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->start(); 
    
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(&scanCallbacks, true); 
    pBLEScan->setActiveScan(false); 
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(70); 
    
    if(pBLEScan->start(0, false)) {
        sysLog("INFO", "SYS", "Scansione continua avviata. In attesa del Beacon...");
    } else {
        sysLog("ERR", "SYS", "Avvio scansione fallito!");
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }

    // Gestione temporizzazioni Relè
    if (isRelayActive && millis() > triggerActiveUntil) {
        digitalWrite(PIN_OUTPUT, LOW);
        isRelayActive = false;
        sysLog("ACT", "RELAY", "Temporizzazione conclusa. PIN disattivato.");
    }
    
    if (isCooldown && millis() - lastTriggerTime > COOLDOWN_DURATION) {
        isCooldown = false;
        sysLog("INFO", "LOGIC", "Cooldown terminato. Sistema pronto.");
    }

    // Pulizia periodica della memoria dello scanner
    static unsigned long lastClear = 0;
    if (millis() - lastClear > 5000) {
        NimBLEDevice::getScan()->clearResults();
        lastClear = millis();
    }
    
    // HEARTBEAT OGNI 10 SECONDI
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 10000) {
        sysLog("INFO", "SYS", "Sistema online e in ascolto.............-");
        lastHeartbeat = millis();
    }
}