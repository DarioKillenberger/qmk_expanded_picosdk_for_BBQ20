#include QMK_KEYBOARD_H
#include "pico_sdk_config_qmk.h" // QMK-specific Pico SDK assertion config
#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "trackpad.h"
#include "bbq20_pins.h"
#include "backlight.h" // For QMK backlight functions

// Pico SDK includes for deep sleep and clock control
#include "pico/stdlib.h" // Provides set_sys_clock_khz and other std utilities
#include "pico/stdio.h"  // For stdio_init_all()
#include "pico/sleep.h"
#include "hardware/clocks.h" // Likely made redundant by pico/stdlib.h
#include "hardware/gpio.h" // Already included by QMK wrappers, but good for clarity
#include "hardware/rosc.h" // For rosc_disable, used by sleep_run_from_xosc
#include "hardware/xosc.h" // For xosc_disable, used by sleep_run_from_rosc
#include "hardware/resets.h" // For reset_block and unreset_block_wait_blocking

// Add ChibiOS USB main header for restart_usb_driver and USB_DRIVER
#include "protocol/chibios/usb_main.h"

// Forward declaration for USB reset function
void perform_full_usb_reset(void);

// Last activity timestamp for power management
static uint32_t last_activity_time = 0;
static bool in_low_power_mode = false;
bool is_usb_suspended = false;  // Made global so trackpad.c can access it

// Defined power management timeouts
#define LOW_POWER_TIMEOUT 10000  // 10 seconds for regular low power (e.g. backlight off)
#define DEEP_SLEEP_TIMEOUT 60000 // 60 seconds for MCU deep sleep
static bool is_deep_sleep_enabled = true; // Enable deep sleep by default

// Pin for waking from deep sleep (Trackpad Motion/Button Pin) - This will be the pin passed to sleep_goto_dormant_until_pin,
// but all column pins will be configured to trigger wake-up.
// #define WAKEUP_PIN GP22 // This is pin_TP_MOTION from trackpad.c
// #define WAKEUP_PIN GP8 // Changed to a keyboard matrix column pin
#define WAKEUP_PIN GP9 // Corrected to the actual COL2 (GP9) based on schematic

// Track backlight state - No longer needed, QMK handles this
// static bool backlight_was_on = true; 

// Helper to check if backlight is on (assume high = on) - No longer needed, use QMK's backlight_is_on() or backlight_get_level()
// bool is_backlight_on(void) {
//     return backlight_was_on;
// }

// Allow keymap to update backlight state - No longer needed
// void set_backlight_state(bool on) {
//     backlight_was_on = on;
// }

// Call this function whenever there's keyboard activity
void keyboard_activity_trigger(void) {
    last_activity_time = timer_read32();
    backlight_toggle();
    wait_ms(200);
    backlight_toggle();
    backlight_level(10);
    if (in_low_power_mode) {
        in_low_power_mode = false;
        backlight_enable();
    }
}

// Check if we should enter low power mode
void power_management_task(void) {
    uint32_t elapsed_since_activity = timer_elapsed32(last_activity_time);

    // Check for deep sleep first, as it's the deeper state and can be entered from any other state
    if (is_deep_sleep_enabled && elapsed_since_activity > DEEP_SLEEP_TIMEOUT) {
        // Entering deep sleep (dormant mode)
        backlight_toggle();
        wait_ms(1050);
        backlight_toggle();
        wait_ms(1050);
        backlight_toggle();
        wait_ms(1050);
        backlight_toggle();

        // Configure ROW pins to be low to allow columns to be pulled low for wake-up
        static const uint8_t row_pins_for_sleep[] = MATRIX_ROW_PINS;
        static const uint8_t num_row_pins_for_sleep = sizeof(row_pins_for_sleep) / sizeof(row_pins_for_sleep[0]);
        for (uint8_t i = 0; i < num_row_pins_for_sleep; ++i) {
            uint8_t pin = row_pins_for_sleep[i];
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_OUT);
            gpio_put(pin, 0);
        }

        // Prepare clocks for dormant mode (run from XOSC)
        sleep_run_from_xosc();
        
        sleep_goto_dormant_until_pin(WAKEUP_PIN, true, false);

        sleep_power_up();

        // --- Restore pins after waking from sleep ---
        static const uint8_t row_pins_for_sleep_v2[] = MATRIX_ROW_PINS;
        static const uint8_t num_row_pins_for_sleep_v2 = sizeof(row_pins_for_sleep_v2) / sizeof(row_pins_for_sleep_v2[0]);
        
        // Restore ROW pins to normal operation
        for (uint8_t i = 0; i < num_row_pins_for_sleep_v2; ++i) {
            uint8_t pin = row_pins_for_sleep_v2[i];
            gpio_init(pin);
        }
        
        keyboard_activity_trigger();
        backlight_toggle();
        wait_ms(1000);
        backlight_toggle();
        wait_ms(1000);
        backlight_toggle();
        wait_ms(1000);
        backlight_toggle();
        perform_full_usb_reset();
        
        wait_ms(1000);
        backlight_toggle();
        wait_ms(1000);
        backlight_toggle();
        wait_ms(1000);
        backlight_toggle();
        wait_ms(100);
        backlight_toggle();
    } 
    // Else, if not going into deep sleep, consider light low power mode
    else if (!is_usb_suspended && !in_low_power_mode && elapsed_since_activity > LOW_POWER_TIMEOUT) {
        // Entering light low power mode (turn off backlight, trackpad IC sleep)
        in_low_power_mode = true;
    }
}

void keyboard_post_init_kb(void) {
    backlight_toggle();
    wait_ms(100);
    backlight_toggle();
    
    perform_full_usb_reset();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // Register any key activity
    if (record->event.pressed) {
        keyboard_activity_trigger();
    }
    return process_record_user(keycode, record);
}

void housekeeping_task_kb(void) {
    power_management_task();
}

// Detect USB suspend state and adjust power accordingly
// void suspend_power_down_kb(void) {
//     is_usb_suspended = true;
//     // trackpad.c's pointing_device_task checks is_usb_suspended and calls trackpad_sleep() if needed.
//     // No need to call trackpad_sleep() directly here, to avoid potential race or double calls.
//     if (!in_low_power_mode) { // Avoid redundant operations if already in low power from inactivity
//         in_low_power_mode = true; // Set low power mode state
//         if (get_backlight_level() > 0) {
//             backlight_level(0);
//         }
//         // trackpad_sleep(); // Removed, handled by trackpad.c itself based on is_usb_suspended
//     }
//     suspend_power_down_user();
// }

// Wake up from USB suspend
// void suspend_wakeup_init_kb(void) {
//     is_usb_suspended = false;
//     // Clocks should be fine as USB suspend isn't as deep as dormant.
//     // Re-initialize sys clock to our desired frequency, in case it was changed.
//     set_sys_clock_khz(96000, true);
//     keyboard_activity_trigger(); 
//     suspend_wakeup_init_user();
// } 

void perform_full_usb_reset(void) {
    // Step 1: Perform hardware reset of the USB controller
    reset_block(1u << RESET_USBCTRL);
    unreset_block_wait(1u << RESET_USBCTRL);

    // Optional: A short delay after hardware reset and before software re-init
    wait_ms(50);

    // Step 2: Re-initialize the ChibiOS/QMK USB driver software stack
    restart_usb_driver(&USB_DRIVER);
} 