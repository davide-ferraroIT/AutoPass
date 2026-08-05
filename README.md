# AutoPass 🚗⚡

**AutoPass** è un sistema IoT basato su **ESP32** progettato per l'apertura automatica di cancelli o garage tramite rilevamento di vicinanza di dispositivi iBeacon (smartphone, tag BLE o smartwatch) con supporto per **aggiornamenti del firmware wireless via Wi-Fi (ArduinoOTA)**.

---

## 🌟 Caratteristiche Principali

- 📡 **Rilevamento iBeacon Continuo**: Scansione Bluetooth ad alte prestazioni in background tramite libreria **NimBLE**.
- ⏱️ **Protezione Anti-Rimbalzo (Cooldown)**: Timer configurabile per prevenire attivazioni ripetute del relè mentre ci si trova nel raggio del cancello.
- 📶 **Wi-Fi OTA (ArduinoOTA)**: Aggiornamento del firmware wireless ad alta velocità tramite rete locale senza dover collegare cavi USB.
- 🔒 **Gestione Sicura delle Credenziali**: Credenziali di rete esternalizzate tramite file di configurazione per prevenire il caricamento involontario di informazioni sensibili su GitHub.

---

## 🛠️ Tecnologie Utilizzate

- **Firmware**: C++ su framework Arduino per ESP32 (PlatformIO).
- **Bluetooth Stack**: [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) (stack BLE leggero ed efficiente).
- **OTA Library**: [ArduinoOTA](https://github.com/espressif/arduino-esp32/tree/master/libraries/ArduinoOTA).

---

## 📁 Struttura del Progetto

```text
AutoPass/
├── include/
│   └── config.h.example     # Template per le credenziali Wi-Fi
├── src/
│   └── main.cpp             # Logica principale ESP32 (Scansione iBeacon, Relè, ArduinoOTA)
├── platformio.ini           # Configurazione PlatformIO e dipendenze
└── README.md                # Documentazione di progetto
```

---

## 🚀 Configurazione e Installazione

### 1. Configurazione del Firmware
Rinominare `include/config.h.example` in `include/config.h` (o crearne uno nella cartella `src/`) inserendo le credenziali della propria rete Wi-Fi:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID "NOME_RETE_WIFI"
#define WIFI_PASS "PASSWORD_WIFI"

#endif
```

### 2. Compilazione e Flashing iniziale via USB
```bash
pio run -t upload
```

---

## 📶 Aggiornamento Firmware via Wi-Fi (ArduinoOTA)

Una volta caricato il primo firmware via USB, è possibile aggiornare l'ESP32 via Wi-Fi:

1. Modificare il file `platformio.ini` decommentando ed inserendo l'IP dell'ESP32:
   ```ini
   upload_protocol = espota
   upload_port = YOUR_ESP32_IP
   ```
2. Eseguire l'upload wireless:
   ```bash
   pio run -t upload
   ```

---

## 🛡️ Licenza
Questo progetto è distribuito sotto licenza MIT.
