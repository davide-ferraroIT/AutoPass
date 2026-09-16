/**
 * @file CloudManager.h
 * @brief Interfaces with cloud-based IoT platforms (e.g., SinricPro).
 * 
 * Manages cloud connectivity, processes remote incoming commands, 
 * and synchronizes local hardware state back to the cloud UI.
 */

#pragma once

#include <Arduino.h>

class CloudManager {
public:
    /**
     * @brief Initializes the connection to the Cloud provider.
     * 
     * Registers callbacks for state changes and handles authentication.
     */
    static void init();
    
    /**
     * @brief Main processing task for cloud communication. 
     * 
     * Must be called repeatedly within the main loop() to keep the connection alive
     * and process incoming messages.
     */
    static void handle();
    
    /**
     * @brief Synchronizes the local physical state with the remote Cloud representation.
     * 
     * Typically used to reset a cloud UI button to the "OFF" state after the 
     * local relay deactivates automatically.
     * 
     * @param state The current boolean state to broadcast (true = ON, false = OFF).
     */
    static void syncState(bool state);
};
