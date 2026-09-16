/**
 * @file BLEManager.h
 * @brief Manages Bluetooth Low Energy (BLE) scanning and server operations.
 * 
 * Configures the ESP32 to act as a BLE Scanner looking for specific iBeacon signatures.
 * Additionally, hosts a BLE GATT Server with UART characteristics to transmit real-time logs
 * and receive administrative commands (like reboot) via Bluetooth.
 */

#pragma once

#include <Arduino.h>

class BLEManager {
public:
    /**
     * @brief Initializes the NimBLE stack, configures the UART server, and starts scanning.
     */
    static void init();
    
    /**
     * @brief Resumes the continuous BLE scanning process.
     * 
     * Typically called after the Wi-Fi stack has completed a reconnection cycle to avoid
     * radio hardware conflicts.
     */
    static void startScanning();

    /**
     * @brief Pauses the BLE scanning process.
     */
    static void stopScanning();

    /**
     * @brief Checks the current state of the BLE scanner.
     * @return true if actively scanning, false otherwise.
     */
    static bool isScanning();

    /**
     * @brief Transmits a text message to connected BLE UART clients.
     * 
     * Designed to be injected into the Logger module as a callback.
     * 
     * @param msg The null-terminated string to transmit.
     */
    static void sendLog(const char* msg);

private:
    /**
     * @brief Configures the BLE GATT Server, Services, and Characteristics for UART emulation.
     */
    static void setupUARTServer();

    /**
     * @brief Configures scanning parameters (intervals, windows) and registers callback handlers.
     */
    static void setupScanner();
};
