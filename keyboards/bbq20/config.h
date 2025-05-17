// Copyright 2022 Stefan Kerkmann
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once


#define DEBOUNCE 10                                 // Reduce debounce time to save power


#define I2C_DRIVER I2CD1
#define I2C1_SCL_PIN GP23
#define I2C1_SDA_PIN GP18
#define I2C1_CLOCK_SPEED 40000                     // Further reduced from 50000 to save more power
#define I2C_USE_FAST_MODE false                    // Disable fast mode to save power

#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_LED GP25
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 500U

#define BACKLIGHT_PWM_DRIVER PWMD4
#define BACKLIGHT_PWM_CHANNEL RP2040_PWM_CHANNEL_B

#define ADC_PIN GP26

// Power Management
#define NO_ACTION_ONESHOT
#define NO_ACTION_TAPPING

// Matrix scanning optimization
#define QMK_KEYS_PER_SCAN 4                       // Process fewer keys per scan cycle (more power saving)
#define MATRIX_SCAN_INTERVAL_MS 20                // Increased scan interval saves more power

// USB Power management
#define WAIT_FOR_USB                              // Enable proper USB suspend
#define USB_SUSPEND_WAKEUP_DELAY 0                // Minimal wake delay

// Enhanced USB suspend behavior
#define USB_SUSPEND_IDLE_DELAY 3000               // Enter low power sooner after 3 seconds of USB suspend

// Reduce polling rate when idle
#define POINTING_DEVICE_TASK_THROTTLE_MS 20       // Increase throttle time to save power
#define POINTING_DEVICE_ROTATION_90
#define POINTING_DEVICE_INVERT_X
#define POINTING_DEVICE_INVERT_Y
