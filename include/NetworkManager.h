/**
 * @file NetworkManager.h
 * @brief Orchestrates core IP networking, OTA updates, and auxiliary network services.
 * 
 * Manages the primary Wi-Fi connection lifecycle (including resilient reconnections),
 * initializes Over-The-Air (OTA) updates, provides a remote debugging interface via Telnet,
 * and runs a basic HTTP server for administrative tasks (e.g., remote reboot).
 */

#pragma once

#include <Arduino.h>

class NetworkManager {
public:
    /**
     * @typedef WifiStateCallback
     * @brief Signature for callbacks responding to changes in Wi-Fi connection status.
     * 
     * Useful for disabling radio-intensive tasks (like BLE scanning) during Wi-Fi reconnection attempts 
     * to prevent hardware contention.
     * 
     * @param connected true if Wi-Fi has successfully connected, false if disconnected.
     */
    typedef void (*WifiStateCallback)(bool connected);

    /**
     * @brief Initiates the primary Wi-Fi connection sequence.
     * 
     * Will block execution temporarily while attempting to acquire an initial IP address.
     */
    static void init();
    
    /**
     * @brief Main network processing task. 
     * 
     * Evaluates connection health, triggers periodic reconnection logic if needed, 
     * and handles incoming requests for OTA, HTTP, and Telnet.
     */
    static void handle();
    
    /**
     * @brief Checks the current Wi-Fi connection status.
     * @return true if an active IP connection exists, false otherwise.
     */
    static bool isConnected();

    /**
     * @brief Registers an external callback to react to Wi-Fi state transitions.
     * 
     * @param callback Function pointer to the transition handler.
     */
    static void setWifiStateCallback(WifiStateCallback callback);

private:
    static bool _isNetworkServicesStarted;       ///< Flag indicating if high-level IP services are initialized.
    static unsigned long _lastWifiRetry;         ///< Timestamp of the last Wi-Fi reconnection attempt.
    static bool _wasConnected;                   ///< Tracks the previous connection state to detect transitions.
    static WifiStateCallback _wifiStateCallback; ///< Injected handler for connection state changes.

    /**
     * @brief Bootstraps secondary network services (OTA, Telnet, Cloud) once an IP is acquired.
     */
    static void startNetworkServices();
};
