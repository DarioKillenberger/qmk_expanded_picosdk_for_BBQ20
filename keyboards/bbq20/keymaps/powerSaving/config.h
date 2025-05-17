#pragma once

// Power Management
#ifndef QMK_KEYS_PER_SCAN
#define QMK_KEYS_PER_SCAN 4        // Process fewer keys per scan cycle
#endif

#ifndef POINTING_DEVICE_TASK_THROTTLE_MS
#define POINTING_DEVICE_TASK_THROTTLE_MS 10
#endif

// USB Power management - matches root values
#ifndef USB_SUSPEND_WAKEUP_DELAY
#define USB_SUSPEND_WAKEUP_DELAY 0
#endif

#ifndef USB_SUSPEND_IDLE_DELAY
#define USB_SUSPEND_IDLE_DELAY 5000
#endif

// Matrix scanning optimization
#ifndef MATRIX_SCAN_INTERVAL_MS
#define MATRIX_SCAN_INTERVAL_MS 15  // Increase scan interval to save power when active
#endif

// Disable breathing to save power
// #define BACKLIGHT_BREATHING

// Reduce backlight level when idle
#ifndef BACKLIGHT_LIMIT_VAL
#define BACKLIGHT_LIMIT_VAL 50
#endif

#ifndef BACKLIGHT_BREATHING_PERIOD
#define BACKLIGHT_BREATHING_PERIOD 5
#endif
