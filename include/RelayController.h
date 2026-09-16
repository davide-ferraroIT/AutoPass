/**
 * @file RelayController.h
 * @brief Manages the hardware relay operations and timing.
 * 
 * Handles the physical activation of the relay, execution duration, 
 * and enforces cooldown periods to prevent rapid consecutive triggers.
 */

#pragma once

#include <Arduino.h>

class RelayController {
public:
    /**
     * @typedef CloudSyncCallback
     * @brief Callback signature to synchronize relay state with external cloud platforms.
     * 
     * @param state The boolean state of the relay (true = ON, false = OFF).
     */
    typedef void (*CloudSyncCallback)(bool state);

    /**
     * @brief Initializes the relay GPIO pin and default states.
     */
    static void init();
    
    /**
     * @brief Main processing task. Must be called repeatedly within the main loop().
     * 
     * Evaluates active timers and triggers deactivation and cooldown completion events.
     */
    static void handle(); 
    
    /**
     * @brief Attempts to activate the relay.
     * 
     * Will fail safely if the relay is already active or currently in a cooldown state.
     * 
     * @param source String identifier of the trigger source (e.g., "BLE", "API") used for logging.
     * @return true if the relay was successfully activated, false otherwise.
     */
    static bool trigger(const char* source); 
    
    /**
     * @brief Unconditionally activates the relay, typically invoked by authoritative remote platforms.
     * 
     * This bypasses standard cooldown restrictions.
     */
    static void triggerFromCloud();

    /**
     * @brief Checks if the relay is currently energized.
     * @return true if active, false otherwise.
     */
    static bool isActive();

    /**
     * @brief Checks if the system is currently enforcing a cooldown period.
     * @return true if in cooldown, false otherwise.
     */
    static bool isInCooldown();

    /**
     * @brief Registers a callback function for state synchronization.
     * 
     * @param callback Function pointer to the synchronization routine.
     */
    static void setCloudSyncCallback(CloudSyncCallback callback);

private:
    static const int PIN_OUTPUT = 26;                      ///< GPIO pin designated for the relay.
    static const unsigned long TRIGGER_DURATION = 1000;    ///< Duration (in ms) the relay remains active.
    static const unsigned long COOLDOWN_DURATION = 120000; ///< Minimum duration (in ms) between triggers.
    
    static unsigned long _triggerActiveUntil;              ///< Timestamp denoting when the relay should deactivate.
    static bool _isRelayActive;                            ///< Internal state tracking for relay activation.
    
    static unsigned long _lastTriggerTime;                 ///< Timestamp of the last activation event.
    static bool _isCooldown;                               ///< Internal state tracking for the cooldown phase.

    static CloudSyncCallback _cloudSyncCallback;           ///< Injected callback for cloud state synchronization.
};
