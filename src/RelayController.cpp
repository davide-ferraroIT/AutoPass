/**
 * @file RelayController.cpp
 * @brief Implementation of the RelayController component.
 */

#include "RelayController.h"
#include "Logger.h"
#include <TelnetStream.h>

// Static member initialization
unsigned long RelayController::_triggerActiveUntil = 0;
bool RelayController::_isRelayActive = false;
unsigned long RelayController::_lastTriggerTime = 0;
bool RelayController::_isCooldown = false;
RelayController::CloudSyncCallback RelayController::_cloudSyncCallback = nullptr;

void RelayController::init() {
    pinMode(PIN_OUTPUT, OUTPUT);
    digitalWrite(PIN_OUTPUT, LOW);
    delay(100);
}

void RelayController::setCloudSyncCallback(CloudSyncCallback callback) {
    _cloudSyncCallback = callback;
}

bool RelayController::trigger(const char* source) {
    // Prevent re-triggering if already active
    if (_isRelayActive) return false;
    
    digitalWrite(PIN_OUTPUT, HIGH);
    _isRelayActive = true;
    _triggerActiveUntil = millis() + TRIGGER_DURATION;
    
    // Register trigger timestamp to enforce subsequent cooldown
    _lastTriggerTime = millis();
    _isCooldown = true;
    
    Logger::sysLog("ACT", "RELAY", "Activation via %s (PIN %d) | Starting cooldown (%lu ms)", source, PIN_OUTPUT, COOLDOWN_DURATION);
    
    // Force immediate flush for time-critical log delivery
    Serial.flush();
    TelnetStream.flush();
    
    return true;
}

void RelayController::triggerFromCloud() {
    // Cloud commands override standard cooldown checks, but we prevent overlapping active phases
    if (!_isRelayActive) {
        digitalWrite(PIN_OUTPUT, HIGH);
        _isRelayActive = true;
        _triggerActiveUntil = millis() + TRIGGER_DURATION;
        
        _lastTriggerTime = millis();
        _isCooldown = true; 
        
        Logger::sysLog("ACT", "RELAY", "Activation via SinricPro (Cloud)");
    }
}

void RelayController::handle() {
    // Process automatic deactivation once the trigger duration elapses
    if (_isRelayActive && millis() > _triggerActiveUntil) {
        digitalWrite(PIN_OUTPUT, LOW);
        _isRelayActive = false;
        Logger::sysLog("ACT", "RELAY", "Trigger duration completed. Pin deactivated.");
        
        // Notify external platforms to ensure UI state consistency
        if (_cloudSyncCallback) {
            _cloudSyncCallback(false);
        }
    }
    
    // Process cooldown expiration
    if (_isCooldown && (millis() - _lastTriggerTime > COOLDOWN_DURATION)) {
        _isCooldown = false;
        Logger::sysLog("INFO", "LOGIC", "Cooldown period expired. System is ready.");
    }
}

bool RelayController::isActive() {
    return _isRelayActive;
}

bool RelayController::isInCooldown() {
    return _isCooldown;
}
