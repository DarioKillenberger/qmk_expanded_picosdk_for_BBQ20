#include QMK_KEYBOARD_H
#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "trackpad.h"
#include "bbq20_pins.h"

// Last activity timestamp for power management
static uint32_t last_activity_time = 0;
static bool in_low_power_mode = false;
bool is_usb_suspended = false;  // Made global so trackpad.c can access it

// Defined power management timeouts
#define LOW_POWER_TIMEOUT 10000  // 10 seconds

// Track backlight state
static bool backlight_was_on = true;

// Helper to check if backlight is on (assume high = on)
bool is_backlight_on(void) {
    // This is a stub; on RP2040, you may need to track this in software
    return backlight_was_on;
}

// Allow keymap to update backlight state
void set_backlight_state(bool on) {
    backlight_was_on = on;
}

// Call this function whenever there's keyboard activity
void keyboard_activity_trigger(void) {
    last_activity_time = timer_read32();
    // Only perform power mode transition if needed
    if (in_low_power_mode) {
        in_low_power_mode = false;
        // Restore backlight only if it was previously on
        if (backlight_was_on) {
            writePinHigh(pin_BLK_KBD);
        }
    }
}

// Check if we should enter low power mode
void power_management_task(void) {
    // Only check timing if we're not already in low power mode
    if (!in_low_power_mode && timer_elapsed32(last_activity_time) > LOW_POWER_TIMEOUT) {
        in_low_power_mode = true;
        // Track and turn off backlight when entering low power mode
        backlight_was_on = is_backlight_on();
        writePinLow(pin_BLK_KBD);
    }
}

void keyboard_post_init_user(void) {
    keyboard_activity_trigger();
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
    trackpad_sleep(); // Power down trackpad during USB suspend
    in_low_power_mode = true;
    // Track and turn off backlight on USB suspend
    backlight_was_on = is_backlight_on();
    writePinLow(pin_BLK_KBD);
    suspend_power_down_user();
}

// Wake up from USB suspend
void suspend_wakeup_init_kb(void) {
    is_usb_suspended = false;
    // Do NOT wake the trackpad here; it will wake only on button press
    keyboard_activity_trigger();
    // Restore backlight only if it was previously on
    if (backlight_was_on) {
        writePinHigh(pin_BLK_KBD);
    }
    suspend_wakeup_init_user();
} 