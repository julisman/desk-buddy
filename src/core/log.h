// log.h — logging to the serial monitor.
// Why not Serial.printf: on core 2.0.17, Serial (HWCDC) on the native USB port
// sometimes wrongly detects "not connected" (SOF detection fails, seen on macOS)
// and drops all output. log_printf writes straight to the console, always comes out.
#pragma once

#include <esp32-hal-log.h>

#define LOG(...) log_printf(__VA_ARGS__)
