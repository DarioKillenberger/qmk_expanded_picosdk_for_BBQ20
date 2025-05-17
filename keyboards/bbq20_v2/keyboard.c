#include QMK_KEYBOARD_H
#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/sio.h"
#include "stdlib.h"
// #include "sleep.h"
// #include "hardware/vreg.h"

// Include trackpad header from keymap
#include "keymaps/default/trackpad.h"

// Last activity timestamp for power management
static uint32_t last_activity_time = 0;
static bool in_low_power_mode = false;
bool is_usb_suspended = false;  // Made global so trackpad code can access it
bool is_deep_sleep_enabled = true; // Can be disabled via keycode if needed

// Defined power management timeouts (ms)
#define LOW_POWER_TIMEOUT 120000  // 2 minutes of inactivity before trackpad shutdown
#define DEEP_SLEEP_TIMEOUT 300000 // 5 minutes of inactivity before deep sleep

// Call this function whenever there's keyboard activity
void keyboard_activity_trigger(void) {
    last_activity_time = timer_read32();
    
    // Only perform power mode transition if needed
    if (in_low_power_mode) {
        in_low_power_mode = false;
        // Wake up trackpad if we're in low power mode
        trackpad_wake();
    }
}

// Check if we should enter low power mode
void power_management_task(void) {
    if (is_usb_suspended) {
        return; // Already in lowest power state during USB suspend
    }
    
    uint32_t elapsed = timer_elapsed32(last_activity_time);
    
    // Enter dormant mode after extended inactivity
    if (is_deep_sleep_enabled && elapsed > DEEP_SLEEP_TIMEOUT) {
        // Ensure trackpad is powered down
        trackpad_sleep();
        
        // Set up wake-up pin (trackpad button)
        // Note: We use pin_TP_MOTION (GP22) as the wake pin - this is the physical click, not movement
        gpio_init(GP22);
        gpio_set_dir(GP22, GPIO_IN);
        gpio_pull_up(GP22);
        
        // Enter dormant mode until trackpad button is pressed (falling edge)
        sleep_run_from_xosc();
        sleep_goto_dormant_until_edge_low(GP22);
        
        // Re-initialize system
        clocks_init();
        
        // Reset activity timer and wake up systems
        in_low_power_mode = false;
        last_activity_time = timer_read32();
        
        // Will wake the trackpad when a button is pressed
    } 
    // Just power down trackpad if inactive
    else if (!in_low_power_mode && elapsed > LOW_POWER_TIMEOUT) {
        in_low_power_mode = true;
        trackpad_sleep();
    }
}

void keyboard_post_init_user(void) {
    // Set system clock to 96 MHz for power saving
    set_sys_clock_khz(96000, true);
    // Initialize with normal power settings
    keyboard_activity_trigger();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // Register any key activity
    if (record->event.pressed) {
        keyboard_activity_trigger();
    }
    
    // Handle custom keycodes if needed
    switch (keycode) {
        case QK_KB_0: // Custom keycode to toggle deep sleep
            if (record->event.pressed) {
                is_deep_sleep_enabled = !is_deep_sleep_enabled;
            }
            return false;
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
    suspend_power_down_user();
}

// Wake up from USB suspend
void suspend_wakeup_init_kb(void) {
    is_usb_suspended = false;
    // Do NOT wake the trackpad here; it will wake only on button press
    keyboard_activity_trigger();
    suspend_wakeup_init_user();
} 