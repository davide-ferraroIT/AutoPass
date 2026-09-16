/**
 * @file Logger.h
 * @brief Centralized logging system.
 * 
 * Provides a modular logging interface that routes messages to Serial,
 * Telnet, and an optional custom output (e.g., Bluetooth Low Energy) 
 * via dependency injection (callback).
 */

#pragma once

#include <Arduino.h>

/**
 * @typedef BleLogCallback
 * @brief Defines the signature for the external logging callback.
 * 
 * @param msg The formatted log message to be transmitted.
 */
typedef void (*BleLogCallback)(const char* msg);

class Logger {
public:
    /**
     * @brief Initializes the logging subsystem.
     * 
     * @param bleCallback Optional callback function pointer to route logs to an external service (default: nullptr).
     */
    static void init(BleLogCallback bleCallback = nullptr);
    
    /**
     * @brief Outputs a formatted log message across all registered streams.
     * 
     * @param level The severity level of the log (e.g., "INFO", "WARN", "ERR").
     * @param module The subsystem or module generating the log (e.g., "SYS", "BLE").
     * @param format A standard printf-like format string.
     * @param ... Additional arguments matching the format string.
     */
    static void sysLog(const char* level, const char* module, const char* format, ...);

private:
    static BleLogCallback _bleCallback; ///< Holds the injected callback for external logging.
};
