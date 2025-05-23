#include QMK_KEYBOARD_H
#include "pico_sdk_config_qmk.h" // QMK-specific Pico SDK assertion config
#include <stdint.h>
#include "pico/sleep.h"
#include "hardware/gpio.h"
#include "hardware/resets.h"
// Add ChibiOS USB main header for restart_usb_driver and USB_DRIVER
#include "protocol/chibios/usb_main.h"

void perform_full_usb_reset(void);

// Last activity timestamp for power management
static uint32_t last_activity_time = 0;
static bool in_low_power_mode = false;

// Defined power management timeouts
#define LOW_POWER_TIMEOUT 10000 // TODO: FIX LOW POWER MODE
#define DEEP_SLEEP_TIMEOUT 60000
static bool is_deep_sleep_enabled = true; // Enable deep sleep by default

// Pin for waking from deep sleep (Trackpad Motion/Button Pin) - This will be the pin passed to sleep_goto_dormant_until_pin
#define WAKEUP_PIN GP9

// Call this function whenever there's keyboard activity
void keyboard_activity_trigger(void) {
    last_activity_time = timer_read32();
    if (in_low_power_mode) {
        in_low_power_mode = false;
        backlight_enable();
    }
}

// Called by housekeeping_task_kb() each matrix scan cycle, to see if we should enter power saving mode
void power_management_task(void) {
    uint32_t elapsed_since_activity = timer_elapsed32(last_activity_time);

    // Check if deep sleep conditions are met
    if (is_deep_sleep_enabled && elapsed_since_activity > DEEP_SLEEP_TIMEOUT) {
        backlight_toggle();
        wait_ms(800);
        backlight_toggle();
        wait_ms(800);
        backlight_toggle();
        wait_ms(800);
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
        // Go into dormant mode until WAKEUP_PIN is pulled low
        sleep_goto_dormant_until_pin(WAKEUP_PIN, true, false);
        // Restore clocks etc. NOTE: THIS IS CRUCIAL, OTHERWISE THE KEYBOARD WILL NOT WORK CORRECTLY AFTER SLEEPING.
        sleep_power_up();

        // Restore pins after waking from sleep
        static const uint8_t row_pins_for_sleep_v2[] = MATRIX_ROW_PINS;
        static const uint8_t num_row_pins_for_sleep_v2 = sizeof(row_pins_for_sleep_v2) / sizeof(row_pins_for_sleep_v2[0]);
        
        // Restore ROW pins to normal operation
        for (uint8_t i = 0; i < num_row_pins_for_sleep_v2; ++i) {
            uint8_t pin = row_pins_for_sleep_v2[i];
            gpio_init(pin);
        }
       
        // Reset USB connection to re-initialize device, otherwise host might not realize it's back online
        perform_full_usb_reset();
        // Trigger keyboard activity to re-enable backlight
        keyboard_activity_trigger();
    } 
    // If no deep sleep yet, but 6 seconds have passed since last activity, turn off backlight
    else if (!in_low_power_mode && elapsed_since_activity > LOW_POWER_TIMEOUT) {
        backlight_disable();
        in_low_power_mode = true;
    }
}

// Called by QMK after all initialization is complete, but before the first matrix scan
void keyboard_post_init_kb(void) {
    backlight_level(10);
}

// Called by QMK each time a key is pressed or released, to handle any custom logic
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // Register any key activity
    if (record->event.pressed) {
        keyboard_activity_trigger();
    }
    return process_record_user(keycode, record);
}

// Called by QMK after every matrix scan to handle housekeeping tasks
void housekeeping_task_kb(void) {
    power_management_task();
}

// Called by power_management_task() to perform a full USB reset
void perform_full_usb_reset(void) {
    // Step 1: Perform hardware reset of the USB controller
    reset_block(1u << RESET_USBCTRL);
    unreset_block_wait(1u << RESET_USBCTRL);

    // Short delay after hardware reset and before software re-init
    wait_ms(50);

    // Step 2: Re-initialize the ChibiOS/QMK USB driver software stack
    restart_usb_driver(&USB_DRIVER);
}