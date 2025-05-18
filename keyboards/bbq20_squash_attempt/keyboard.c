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
#include "pico/sleep.h"
#include "hardware/clocks.h" // Likely made redundant by pico/stdlib.h
#include "hardware/gpio.h" // Already included by QMK wrappers, but good for clarity
#include "hardware/rosc.h" // For rosc_disable, used by sleep_run_from_xosc
#include "hardware/xosc.h" // For xosc_disable, used by sleep_run_from_rosc

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
    // Only perform power mode transition if needed
    if (in_low_power_mode) {
        in_low_power_mode = false;
        // Restore backlight if it was on before low power mode
        // Check the persisted EEPROM state for backlight level.
        uint8_t persisted_level = eeconfig_read_backlight(); // Assuming this returns the level
        if (get_backlight_level() == 0 && persisted_level > 0) { 
             backlight_level(persisted_level); // Restore to persisted level
        } else if (get_backlight_level() > 0) {
            // If it's already on (e.g. user turned it on manually during low power), leave it.
            // Or, if persisted_level was 0 but it's on, respect current state.
            // Essentially, if backlight is off AND it should be on, turn it to persisted level.
        }
    }
}

// Check if we should enter low power mode
void power_management_task(void) {
    uint32_t elapsed_since_activity = timer_elapsed32(last_activity_time);

    // Check for deep sleep first, as it's the deeper state and can be entered from any other state (active, light low power, or USB suspended)
    if (is_deep_sleep_enabled && elapsed_since_activity > DEEP_SLEEP_TIMEOUT) {
        // Entering deep sleep (dormant mode)
        if (get_backlight_level() > 0) { // Ensure backlight is off before sleep
            backlight_level(0);
        }
        trackpad_sleep(); // Ensure trackpad IC is powered down via its shutdown pin

        // Define column pins for wake-up based on MATRIX_COL_PINS from bbq20_pins.h
        // MATRIX_COL_PINS is an initializer list like { GP15, GP14, ... }
        // We need to create an actual array from it.
        static const uint8_t wake_column_pins[] = MATRIX_COL_PINS;
        static const uint8_t num_wake_columns = sizeof(wake_column_pins) / sizeof(wake_column_pins[0]);

        // Configure all column pins to wake the device on a falling edge
        for (uint8_t i = 0; i < num_wake_columns; ++i) {
            uint8_t pin = wake_column_pins[i];
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_IN);
            gpio_pull_up(pin);
            // Enable dormant IRQ on falling edge for this pin
            // GPIO_IRQ_EDGE_FALL is equivalent to (true, false) in sleep_goto_dormant_until_pin
            gpio_set_dormant_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true);
        }

        // Prepare clocks for dormant mode (run from XOSC)
        sleep_run_from_xosc(); 
        
        // Go dormant. Any of the configured column pins can now wake the device.
        // We pass WAKEUP_PIN (the first column pin) as the nominal pin to sleep_goto_dormant_until_pin.
        // This function handles the low-level sleep details and ensures a reboot on wake.
        sleep_goto_dormant_until_pin(WAKEUP_PIN, true, false); // true for edge, false for low (falling edge)
        
        // Code below this point in this function will not be reached if sleep_goto_dormant_until_pin reboots.
        // Upon reboot, GPIOs are reset, so explicit cleanup of dormant IRQs here is not strictly necessary.
    } 
    // Else, if not going into deep sleep, consider light low power mode
    // This mode is only entered if USB is NOT suspended (as suspend_power_down_kb handles initial USB suspend state)
    // and if not already in a light low power mode.
    else if (!is_usb_suspended && !in_low_power_mode && elapsed_since_activity > LOW_POWER_TIMEOUT) {
        // Entering light low power mode (turn off backlight, trackpad IC sleep)
        in_low_power_mode = true;
        if (get_backlight_level() > 0) {
            backlight_level(0);
        }
        trackpad_sleep(); // Use the existing trackpad_sleep which handles its shutdown pin
    }
    // Note: Waking from light low power mode (when in_low_power_mode is true and activity occurs)
    // is handled by keyboard_activity_trigger(), which sets in_low_power_mode = false and restores backlight.
}

void keyboard_post_init_user(void) {
    // If waking from dormant sleep, clocks need to be reinitialized.
    // The RP2040 SDK's clocks_init() is usually called very early in boot by crt0 or similar.
    // QMK's ChibiOS port for RP2040 likely handles this standard init.
    // Forcing set_sys_clock_khz re-configures, ensuring a known state.
    // Set system clock to a lower speed for power saving, e.g., 96 MHz.
    // USB requires PLL_USB to be active and providing 48MHz. SYS clock can be independent.
    set_sys_clock_khz(96000, true); // Set sys clock to 96MHz. Second param `true` means update UART too.

    // Restore backlight to its persisted state after waking from dormant sleep (which involves a reboot)
    uint8_t persisted_level = eeconfig_read_backlight(); // Reads the level from EEPROM
    backlight_level(persisted_level); // Set backlight to this level. If 0, it turns/keeps it off.
    // MODIFICATION: Do not automatically turn on backlight after dormant wake.
    // It will be restored by keyboard_activity_trigger() upon first actual key press or trackpad activity.
    // REVERTED: User wants backlight on after deep sleep wake for testing.

    keyboard_activity_trigger(); // Initialize activity timer and ensure power mode state is set to active.
                                 // `in_low_power_mode` will be false (due to global variable initialization on reboot).
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // Register any key activity
    if (record->event.pressed) {
        keyboard_activity_trigger();
    }
    return process_record_user(keycode, record);
}

void housekeeping_task_user(void) {
    // Check if we should enter low power mode
    power_management_task();
}

// Detect USB suspend state and adjust power accordingly
void suspend_power_down_kb(void) {
    is_usb_suspended = true;
    // trackpad.c's pointing_device_task checks is_usb_suspended and calls trackpad_sleep() if needed.
    // No need to call trackpad_sleep() directly here, to avoid potential race or double calls.
    if (!in_low_power_mode) { // Avoid redundant operations if already in low power from inactivity
        in_low_power_mode = true; // Set low power mode state
        if (get_backlight_level() > 0) {
            backlight_level(0);
        }
        // trackpad_sleep(); // Removed, handled by trackpad.c itself based on is_usb_suspended
    }
    suspend_power_down_user();
}

// Wake up from USB suspend
void suspend_wakeup_init_kb(void) {
    is_usb_suspended = false;
    // Clocks should be fine as USB suspend isn't as deep as dormant.
    // Re-initialize sys clock to our desired frequency, in case it was changed.
    set_sys_clock_khz(96000, true);
    keyboard_activity_trigger(); 
    suspend_wakeup_init_user();
} 