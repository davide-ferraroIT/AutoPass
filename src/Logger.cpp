/**
 * @file Logger.cpp
 * @brief Implementation of the centralized logging system.
 */

#include "Logger.h"
#include <TelnetStream.h>

BleLogCallback Logger::_bleCallback = nullptr;

void Logger::init(BleLogCallback bleCallback) {
    _bleCallback = bleCallback;
}

void Logger::sysLog(const char* level, const char* module, const char* format, ...) {
    char msgBuffer[256];
    char logBuffer[300];
    
    // Format the incoming variadic arguments into the message buffer
    va_list args;
    va_start(args, format);
    vsnprintf(msgBuffer, sizeof(msgBuffer), format, args);
    va_end(args);
    
    // Construct the final log payload with timestamp, level, and module
    snprintf(logBuffer, sizeof(logBuffer), "[%08lu] [%-5s] [%-6s] %s", millis(), level, module, msgBuffer);
    
    // Dispatch to standard hardware serial
    Serial.println(logBuffer);
    
    // Dispatch to remote network serial (Telnet)
    TelnetStream.println(logBuffer);
    
    // Dispatch to the injected external callback (e.g., BLE UART) if registered
    if (_bleCallback) {
        _bleCallback(logBuffer);
    }
}
