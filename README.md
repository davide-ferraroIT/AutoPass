# GateKeeper-ESP 📡🚪

Firmware unificato e all-in-one per **ESP32** progettato per l'automazione intelligente e il controllo remoto di cancelli elettrici o porte automatizzate. 

Il sistema combina la comodità dell'apertura automatica tramite tracciamento di prossimità **BLE (Bluetooth Low Energy)** e la flessibilità di un'interfaccia di controllo remoto basata su **WebSerial**, eliminando la necessità di cavi fisici dopo la prima installazione grazie agli aggiornamenti **OTA (Over-The-Air)**.

---

## 💡 Caratteristiche Principali

* **Apertura Automatica via BLE**: Monitora costantemente l'ambiente circostante. Quando rileva lo specifico indirizzo MAC del beacon autorizzato, valuta la potenza del segnale (RSSI) e attiva il relè solo a distanza ravvicinata, evitando aperture accidentali.
* **Controllo Remoto (WebSerial)**: Interfaccia web interattiva accessibile da browser (all'indirizzo IP dell'ESP32) per monitorare i log di scansione in tempo reale e inviare comandi manuali di apertura (es. digitando `APRI`).
* **Aggiornamenti Wireless (ArduinoOTA)**: Consente di modificare e caricare il firmware sulla scheda direttamente tramite Wi-Fi da VSCode/PlatformIO, senza dover scollegare il dispositivo dal cancello.
* **Architettura Non-Bloccante**: A differenza dei classici firmware basati su `delay()`, la gestione dei tempi di attesa e del cooldown del Bluetooth è sviluppata tramite `millis()`. Questo garantisce che il server web e l'ascolto OTA rimangano sempre attivi e reattivi.
* **Prevenzione Loop (Cooldown)**: Include una pausa di sicurezza temporizzata (3 minuti di default) dopo ogni apertura riuscita per evitare che il cancello continui a ricevere impulsi finché rimani fisicamente nel raggio d'azione.

---

## 🛠️ Requisiti Hardware

* Scheda di sviluppo **ESP32** (es. ESP32 DevKit v1).
* Modulo Relè (collegato di default al pin `GPIO 26`).
* Un Beacon BLE (portachiavi smart, smartphone con app di emulazione o tracker).
* Rete Wi-Fi locale a 2.4 GHz.

---

## ⚙️ Struttura e Configurazione del Codice

Prima di caricare il firmware, personalizza le definizioni all'inizio del file `src/main.cpp`:

```cpp
// --- CONFIGURAZIONE RETE ---
const char* ssid = "IL_TUO_SSID_WIFI";
const char* password = "LA_TUA_PASSWORD_WIFI";

// --- CONFIGURAZIONE BLE E TRIGGER ---
#define PIN_OUTPUT 26                  // GPIO associato al comando del relè
#define TARGET_MAC "aa:bb:cc:dd:ee:ff"  // Indirizzo MAC del tuo beacon BLE
#define RSSI_SOGLIA -90                 // Soglia di prossimità (più vicino a 0 = più vicino fisicamente)