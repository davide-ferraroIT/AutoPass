/**
 * @file CloudManager.cpp
 * @brief Implementation of the Cloud integration layer.
 */

#include "CloudManager.h"
#include "config.h"
#include "Logger.h"
#include "RelayController.h"

// Enables debugging output within the SinricPro library
#define ENABLE_DEBUG
#include <SinricPro.h>
#include <SinricProSwitch.h>

/**
 * @brief Callback invoked by the SinricPro library when a remote state change is requested.
 * 
 * @param deviceId The unique identifier of the target device.
 * @param state The requested power state (passed by reference).
 * @return true indicating the request was processed successfully.
 */
static bool onPowerState(const String &deviceId, bool &state) {
    Logger::sysLog("INFO", "SINRIC", "Command received. Requested state: %s", state ? "ON" : "OFF");
    
    if (state) {
        RelayController::triggerFromCloud();
        // Immediately revert the reported state to OFF, emulating a push-button behavior 
        // in the SinricPro user interface.
        state = false;
    }
    return true; 
}

void CloudManager::init() {
    // Bind the local switch instance to the configured SinricPro device ID
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
    
    // Register the handler for incoming power commands
    mySwitch.onPowerState(onPowerState);
    
    // Register connection status handlers for observability
    SinricPro.onConnected([](){ Logger::sysLog("INFO", "SINRIC", "Successfully connected to SinricPro Cloud!"); });
    SinricPro.onDisconnected([](){ Logger::sysLog("WARN", "SINRIC", "Disconnected from SinricPro Cloud."); });
    
    // Initiate the connection using credentials defined in config.h
    SinricPro.begin(APP_KEY, APP_SECRET);
}

void CloudManager::handle() {
    // Process internal SinricPro tasks (keep-alive, incoming messages)
    SinricPro.handle();
}

void CloudManager::syncState(bool state) {
    // Broadcast the new state to the cloud to update dashboards and apps
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
    mySwitch.sendPowerStateEvent(state);
}
