// Copyright 2022 Stefan Kerkmann
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once


#define DEBOUNCE 15                                  //debounce time set to 15ms


#define I2C_DRIVER I2CD1
#define I2C1_SCL_PIN GP23
#define I2C1_SDA_PIN GP18
#define I2C1_CLOCK_SPEED 100000

#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_LED GP25
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 500U

#define BACKLIGHT_PWM_DRIVER PWMD4
#define BACKLIGHT_PWM_CHANNEL RP2040_PWM_CHANNEL_B

// Battery level ADC pin
#define ADC_PIN GP26

// Enable USB suspend support
#define USB_SUSPEND_WAKEUP_DELAY 200
#define NO_USB_STARTUP_CHECK     // Don't wait for USB to be ready

// Auto-shutdown configuration (timeout values defined in keyboard.c)
// Note: Only trackpad button presses will wake the device, not movement

// Optimize RP2040 power consumption during active use
#define RP2040_FLASH_GD25Q64CS // Use correct flash chip
#define USB_POLLING_INTERVAL_MS 10 // Reduce USB polling rate to save power

// Optimize matrix scanning for power saving
#define MATRIX_IO_DELAY 15 // Increase delay between scans
#define QMK_KEYS_PER_SCAN 4 // Process fewer keys per scan to reduce power spikes

// Disable unused features to reduce power consumption
// #define NO_ACTION_MACRO   // Removed: no longer valid in QMK
// #define NO_ACTION_FUNCTION // Removed: no longer valid in QMK
#define NO_ACTION_TAPPING       // Disable tap dance and other tapping features
#define NO_ACTION_ONESHOT       // Disable one-shot keys
#define DISABLE_LEADER          // Disable leader key

#define USB_MAX_POWER_CONSUMPTION 100
