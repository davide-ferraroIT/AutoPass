#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <ArduinoOTA.h>
#include <NimBLEDevice.h>

// --- CONFIGURAZIONE RETE ---
const char* ssid = "SSID";
const char* password = "PASSWORD";

AsyncWebServer server(80);

// --- CONFIGURAZIONE BLE E TRIGGER ---
#define PIN_OUTPUT 26
#define TARGET_MAC "aa:bb:ccc:dd:ee:ff" // Inserisci il MAC del tuo beacon
#define RSSI_SOGLIA -90

NimBLEScan* pBLEScan;

// Variabili per gestire il "cooldown" (pausa) in modo non bloccante
unsigned long ultimoTrigger = 0;
bool inPausa = false;

void setup() {
  Serial.begin(115200);
  
  // --- Inizializzazione PIN ---
  pinMode(PIN_OUTPUT, OUTPUT);
  digitalWrite(PIN_OUTPUT, LOW);

  // --- Connessione WiFi ---
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connessione WiFi in corso");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnesso! Indirizzo IP: " + WiFi.localIP().toString());

  // --- Configurazione WebSerial (Interfaccia Remota) ---
  WebSerial.begin(&server);
  WebSerial.onMessage([](uint8_t *data, size_t len) {
    String comando = "";
    for (size_t i = 0; i < len; i++) {
      comando += (char)data[i];
    }
    comando.trim(); // Rimuove eventuali spazi bianchi o ritorni a capo
    
  });
  server.begin();

  // --- Configurazione OTA ---
  ArduinoOTA.setHostname("ESP32-GateKeeper");
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Inizio aggiornamento OTA " + type);
  });
  ArduinoOTA.begin();

  // --- Inizializzazione BLE ---
  Serial.println("Inizializzazione Sistema di Accesso BLE...");
  NimBLEDevice::init(""); 
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setActiveScan(true); 
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  
  // Stampa l'indirizzo a cui collegarsi
  Serial.println("Sistema pronto! Vai su http://" + WiFi.localIP().toString() + "/webserial per controllare il dispositivo.");
}

void loop() {
  // 1. Fondamentale: mantieni in vita la ricezione di eventuali aggiornamenti OTA
  ArduinoOTA.handle();

  // 2. Gestione della pausa di sicurezza in modo non bloccante (Cooldown)
  if (inPausa) {
    // Controlla se sono passati i 180.000 ms (3 minuti)
    if (millis() - ultimoTrigger >= 180000) {
      inPausa = false;
      WebSerial.println("[SISTEMA] Cooldown terminato. Riprendo la ricerca del beacon...");
    } else {
      // Se siamo ancora in pausa, esce dal loop (così permette all'OTA di girare in background)
      return; 
    }
  }

  // 3. Scansione BLE e Logica di Prossimità
  bool autorizzaApertura = false;
  
  Serial.println("Inizio scan");
  WebSerial.println("Inizio scan"); // Invia il log anche all'interfaccia remota

  // Scansione di 5000 millisecondi
  NimBLEScanResults foundDevices = pBLEScan->getResults(5000, false); 
  
  for (int i = 0; i < foundDevices.getCount(); i++) {
    const NimBLEAdvertisedDevice* device = foundDevices.getDevice(i);
    
    if (device->getAddress().toString() == TARGET_MAC) {
      String msgRssi = "[BLE] BERSAGLIO INDIVIDUATO! RSSI: " + String(device->getRSSI());
      Serial.println(msgRssi);
      WebSerial.println(msgRssi); // Invia il log anche all'interfaccia remota
      
      if (device->getRSSI() > RSSI_SOGLIA) {
        autorizzaApertura = true;
      } else {
        WebSerial.println("[BLE] Beacon trovato, ma troppo lontano.");
      }
      break;
    }
  }

  Serial.println("Fine scan");
  WebSerial.println("Fine scan"); // Invia il log anche all'interfaccia remota

  pBLEScan->clearResults(); 

  // 4. Attivazione del Relè via BLE
  if (autorizzaApertura) {
    WebSerial.println("[AZIONE] Beacon vicino! Attivazione automatica in corso...");
    
    digitalWrite(PIN_OUTPUT, HIGH);
    delay(1000);  // Mantiene il pin attivo per 1 secondo
    digitalWrite(PIN_OUTPUT, LOW);
    
    WebSerial.println("[AZIONE] Contatto chiuso. Inizio pausa di sicurezza di 3 minuti.");
    
    // Attiva il timer del cooldown al posto del delay()
    ultimoTrigger = millis();
    inPausa = true;
  }
}