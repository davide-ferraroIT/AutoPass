#include <NimBLEDevice.h>

// --- CONFIGURAZIONE ---
#define PIN_OUTPUT 26 //inserisci qui il pin d'uscita
#define TARGET_MAC "aa:bb:cc:dd:ee:ff" // Il MAC del tuo beacon
#define RSSI_SOGLIA -90 // SOGLIA: Più il numero è vicino a 0, più devi essere vicino (es. -90 = lontano, -50 = vicinissimo)

NimBLEScan* pBLEScan;

void setup() {
    Serial.begin(115200);
    
    // Inizializza il pin di output
    pinMode(PIN_OUTPUT, OUTPUT);
    digitalWrite(PIN_OUTPUT, LOW);

    Serial.println("Inizializzazione Sistema di Accesso BLE...");

    // Inizializza il modulo Bluetooth
    NimBLEDevice::init(""); 
    
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setActiveScan(true); 
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void loop() {
    bool autorizzaApertura = false;
    Serial.println("\n--- Inizio scansione (5 secondi) ---");
    
    // Scansione bloccante di 5000 millisecondi (5 secondi)
    NimBLEScanResults foundDevices = pBLEScan->getResults(5000, false); //modifica qui per cambiare il tempo della scansione
    
    // controllo se tra i dispositivi trovati c'è il beacon
    for (int i = 0; i < foundDevices.getCount(); i++) {
        const NimBLEAdvertisedDevice* device = foundDevices.getDevice(i);
        
        // Se il MAC corrisponde al becon
        if (device->getAddress().toString() == TARGET_MAC) {
            Serial.printf("BERSAGLIO INDIVIDUATO! Segnale (RSSI): %d \n", device->getRSSI());
            
            // controllo se la potenza del segnale supera la soglia minima
            if (device->getRSSI() > RSSI_SOGLIA) {
                autorizzaApertura = true;
            } else {
                Serial.println("Beacon trovato, ma troppo lontano");
            }
            
            break;
        }
    }

    // Pulisce la memoria per la prossima scansione
    pBLEScan->clearResults(); 

    // --- LOGICA DI ATTIVAZIONE ---
    if (autorizzaApertura) {
        Serial.println("AZIONE: Beacon vicino! (Attivazione PIN)...");
        digitalWrite(PIN_OUTPUT, HIGH);
        
        // Mantiene il pin attivo per 1 secondo
        delay(1000);  //modifica qui per cambiare il tempo in cui il pin d'uscita rimane HIGH
        
        Serial.println("Chiusura contatto. Spegnimento PIN.");
        digitalWrite(PIN_OUTPUT, LOW);
        
        // Pausa di sicurezza di 3 minuti per evitare ripetizioni (es: cancello in apertura)
        delay(180000); //modifica qui per cambiare il tempo tra un ricerca del ebcon e l'altra
    } else {
        Serial.println("In attesa del beacon...");
        digitalWrite(PIN_OUTPUT, LOW); // Assicurazione extra che il pin sia spento
    }
}