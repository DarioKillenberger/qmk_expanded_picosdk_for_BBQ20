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
    // Only perform power mode transition if needed
    backlight_toggle();
    wait_ms(123);
    backlight_toggle();
    wait_ms(123);
    backlight_toggle();
    wait_ms(123);
    backlight_toggle();
    backlight_level(10);
    if (in_low_power_mode) {
        in_low_power_mode = false;
        // Restore backlight if it was on before low power mode
        // Check the persisted EEPROM state for backlight level.
        // uint8_t persisted_level = eeconfig_read_backlight(); // Assuming this returns the level
        // if (get_backlight_level() == 0 && persisted_level > 0) { 
        //      backlight_level(persisted_level); // Restore to persisted level
        // } else if (get_backlight_level() > 0) {
            // If it's already on (e.g. user turned it on manually during low power), leave it.
            // Or, if persisted_level was 0 but it's on, respect current state.
            // Essentially, if backlight is off AND it should be on, turn it to persisted level.
        backlight_enable();
    }
}

// Check if we should enter low power mode
void power_management_task(void) {
    uint32_t elapsed_since_activity = timer_elapsed32(last_activity_time);

    // for (int i = 0; i < 9; i++) {
    //         backlight_toggle();
    //         wait_ms(150);
    //         backlight_toggle();
    //         wait_ms(150);
    //     }

    // Check for deep sleep first, as it's the deeper state and can be entered from any other state (active, light low power, or USB suspended)
    if (is_deep_sleep_enabled && elapsed_since_activity > DEEP_SLEEP_TIMEOUT) {
        // Entering deep sleep (dormant mode)
        backlight_toggle();
        wait_ms(1050);
        backlight_toggle();
        wait_ms(1050);
        backlight_toggle();
        wait_ms(1050);
        backlight_toggle();
        // trackpad_sleep(); // Ensure trackpad IC is powered down via its shutdown pin

        //  static const uint8_t wake_column_pins[] = MATRIX_COL_PINS;
        // static const uint8_t num_wake_columns = sizeof(wake_column_pins) / sizeof(wake_column_pins[0]);

        // // Configure all column pins to wake the device
        // // Temporarily disabled to test wake-up with only WAKEUP_PIN configured by sleep_goto_dormant_until_pin
        // for (uint8_t i = 0; i < num_wake_columns; ++i) {
        //     uint8_t pin = wake_column_pins[i];
        //     gpio_init(pin);
        //     gpio_set_dir(pin, GPIO_IN);
        //     gpio_pull_up(pin);
        //     // Change to wake on level low
        //     gpio_set_dormant_irq_enabled(pin, GPIO_IRQ_LEVEL_LOW, true);
        // }

        // Configure ROW pins to be low to allow columns to be pulled low for wake-up
        static const uint8_t row_pins_for_sleep[] = MATRIX_ROW_PINS;
        static const uint8_t num_row_pins_for_sleep = sizeof(row_pins_for_sleep) / sizeof(row_pins_for_sleep[0]);
        for (uint8_t i = 0; i < num_row_pins_for_sleep; ++i) {
            uint8_t pin = row_pins_for_sleep[i];
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_OUT);
            gpio_put(pin, 0); // Drive row pin low
        }

        // Prepare clocks for dormant mode (run from XOSC)
        sleep_run_from_xosc(); // Ensure XOSC is running and sys_clk is switched for dormant mode
        
        // Go dormant. Any of the configured column pins can now wake the device.
        // We pass WAKEUP_PIN (the first column pin) as the nominal pin to sleep_goto_dormant_until_pin.
        // This function handles the low-level sleep details.
        // CRITICAL ASSUMPTION: This function RESUMES execution here, does NOT reboot.
        // sleep_goto_dormant_until_pin(WAKEUP_PIN, true, false); // Original: true for edge, false for low (falling edge)
        // sleep_goto_dormant_until_pin(WAKEUP_PIN, false, false); // Changed: false for level, false for low (low level) - Did not wake
        sleep_goto_dormant_until_pin(WAKEUP_PIN, true, false); // Reverted to FALLING EDGE for WAKEUP_PIN only

        // reset_keyboard();

       

        // --- Restore pins after waking from sleep ---
        // It's crucial to disable the dormant IRQs and reconfigure pins for normal operation.
        // static const uint8_t wake_column_pins_v2[] = MATRIX_COL_PINS;
        // static const uint8_t num_wake_columns_v2 = sizeof(wake_column_pins_v2) / sizeof(wake_column_pins_v2[0]);
        static const uint8_t row_pins_for_sleep_v2[] = MATRIX_ROW_PINS;
        static const uint8_t num_row_pins_for_sleep_v2 = sizeof(row_pins_for_sleep_v2) / sizeof(row_pins_for_sleep_v2[0]);
        
        // Restore COLUMN pins to normal operation.
        // // Temporarily disabled as the setup loop is also disabled.
        // // Moreover, sleep_goto_dormant_until_pin handles disabling IRQ for WAKEUP_PIN,
        // // and reset_keyboard() will lead to QMK's matrix_init().
        // for (uint8_t i = 0; i < num_wake_columns_v2; ++i) {
        //     uint8_t pin = wake_column_pins_v2[i];
        //     // Disable the dormant IRQ that was enabled for wakeup.
        //     gpio_set_dormant_irq_enabled(pin, GPIO_IRQ_LEVEL_LOW, false);
        //     
        //     // Re-initialize the pin to the standard QMK state for columns.
        //     // QMK's matrix_init after reset_keyboard() should handle this.
        //     // gpio_init(pin);
        //     // gpio_set_dir(pin, GPIO_IN);
        //     // gpio_pull_up(pin); 
        // }

        // Restore ROW pins to normal operation.
        for (uint8_t i = 0; i < num_row_pins_for_sleep_v2; ++i) {
            uint8_t pin = row_pins_for_sleep_v2[i];
            
            // Reset the pin to its default GPIO state. 
            // QMK's matrix_init() or the matrix_scan() routine will then configure it 
            // as an output and set its level appropriately during the matrix scan.
            gpio_init(pin); // QMK's matrix scanning will set direction and state.
        }
        
        // Trigger activity to reset timers and potentially turn backlight on.
        // This should come after pins are restored to ensure proper hardware state.
        keyboard_activity_trigger();
        perform_full_usb_reset();
        // runtime_init_bootrom_reset();

        backlight_toggle();
        wait_ms(100);
        backlight_toggle();
        wait_ms(1000);
        backlight_toggle();
        wait_ms(100);
        backlight_toggle();
        
        // // ---- CODE REACHED ON RESUME FROM DORMANT SLEEP ----
        // for (int i = 0; i < 5; i++) {
        //     backlight_level(5);
        //     wait_ms(150);
        //     backlight_level(0);
        //     wait_ms(150);
        // }

        // set_sys_clock_khz(96000, true);

        // for (int i = 0; i < 4; i++) {
        //     backlight_level(5);
        //     wait_ms(150);
        //     backlight_level(0);
        //     wait_ms(150);
        // }
        // // 1. Re-initialize system clocks. USB LLD might depend on these.
        // //    set_sys_clock_khz also re-inits stdio_uart if the second param is true.
        // // restart_usb_driver(&USB_DRIVER);

    
        // for (int i = 0; i < 3; i++) {
        //     backlight_level(5);
        //     sleep_ms(100);
        //     backlight_level(0);
        //     sleep_ms(100);
        // }
        // // 2. Re-initialize USB driver
        // //    USB_DRIVER is defined in protocol/chibios/usb_main.h (usually USBD1)
        // //    restart_usb_driver handles stopping, reconfiguring, and restarting the USB stack.
        // // #if HAL_USE_USB
        // // dprintf("Resumed from dormant. Restarting USB driver.\n"); // For debugging if dprintf is set up
        
        // // #endif

        // // 3. Reset power management state and activity timer
        // keyboard_activity_trigger(); // This will also try to restore backlight based on persisted level
        //                              // if it thinks it was in low power mode.
        // in_low_power_mode = false;   // Explicitly ensure we are not in low power mode.

        // for (int i = 0; i < 2; i++) {
        //     backlight_level(5);
        //     sleep_ms(100);
        //     backlight_level(0);
        //     sleep_ms(100);
        // }
        // // 4. Explicitly restore backlight if keyboard_activity_trigger didn't (e.g. if persisted level was 0)
        // //    or if keyboard_activity_trigger's logic isn't sufficient for resume.
        // //    keyboard_activity_trigger turns backlight on if it was off AND persisted > 0
        // //    This is redundant if keyboard_activity_trigger already handled it, but ensures it.
        // uint8_t persisted_level = eeconfig_read_backlight();
        // if (get_backlight_level() == 0 && persisted_level > 0) {
        //     backlight_level(persisted_level);
        // } else if (get_backlight_level() > 0) {
        //     // If already on (perhaps keyboard_activity_trigger did it), leave it.
        // }
        // If persisted_level is 0, it remains off.

        // GPIOs used for dormant wake IRQ should be reset to their normal function
        // or re-initialized by their respective drivers if necessary.
        // For matrix pins, the QMK matrix scan will re-initialize them.
        // For other pins (like trackpad), ensure their drivers handle re-init if needed.
        // The RP2040 SDK's gpio_set_dormant_irq_enabled documentation isn't explicit on cleanup needs
        // upon resume vs reboot. Assuming for resume, pins might need manual reset if not handled by drivers.
        // However, QMK's matrix scanning should re-init column pins.

    } 
    // Else, if not going into deep sleep, consider light low power mode
    // This mode is only entered if USB is NOT suspended (as suspend_power_down_kb handles initial USB suspend state)
    // and if not already in a light low power mode.
    else if (!is_usb_suspended && !in_low_power_mode && elapsed_since_activity > LOW_POWER_TIMEOUT) {
        // Entering light low power mode (turn off backlight, trackpad IC sleep)
        in_low_power_mode = true;
        // if (get_backlight_level() > 0) {
        //     backlight_level(0);
        // }
        // trackpad_sleep(); // Use the existing trackpad_sleep which handles its shutdown pin
    }
    // Note: Waking from light low power mode (when in_low_power_mode is true and activity occurs)
    // is handled by keyboard_activity_trigger(), which sets in_low_power_mode = false and restores backlight.
}

void keyboard_post_init_kb(void) {
//     // If waking from dormant sleep, clocks need to be reinitialized.
//     // The RP2040 SDK's clocks_init() is usually called very early in boot by crt0 or similar.
//     // QMK's ChibiOS port for RP2040 likely handles this standard init.
//     // Forcing set_sys_clock_khz re-configures, ensuring a known state.
//     // Set system clock to a lower speed for power saving, e.g., 96 MHz.
//     // USB requires PLL_USB to be active and providing 48MHz. SYS clock can be independent.
//     set_sys_clock_khz(96000, true); // Set sys clock to 96MHz. Second param `true` means update UART too.

//     // Restore backlight to its persisted state after waking from dormant sleep (which involves a reboot)
//     uint8_t persisted_level = eeconfig_read_backlight(); // Reads the level from EEPROM
//     backlight_level(persisted_level); // Set backlight to this level. If 0, it turns/keeps it off.
//     // MODIFICATION: Do not automatically turn on backlight after dormant wake.
//     // It will be restored by keyboard_activity_trigger() upon first actual key press or trackpad activity.
//     // REVERTED: User wants backlight on after deep sleep wake for testing.

//     keyboard_activity_trigger(); // Initialize activity timer and ensure power mode state is set to active.
//      
    backlight_toggle();
    wait_ms(100);
    backlight_toggle();
    

    perform_full_usb_reset();
                                    // `in_low_power_mode` will be false (due to global variable initialization on reboot).
    // for (int i = 0; i < 8; i++) {
    //     backlight_toggle();
    //     wait_ms(150);
    //     backlight_toggle();
    //     wait_ms(150);
    // }
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // Register any key activity
    if (record->event.pressed) {
        keyboard_activity_trigger();
    }
    return process_record_user(keycode, record);
}

void housekeeping_task_kb(void) {
    // wait_ms(1050);
    // backlight_toggle();
    // wait_ms(150);
    // backlight_toggle();
    // wait_ms(150);
    // backlight_toggle();
    // wait_ms(150);
    // backlight_toggle();
    // wait_ms(150);
    // backlight_toggle();
    // wait_ms(150);
    // backlight_toggle();
    // Check if we should enter low power mode
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
    // Use the SDK1XX compatibility alias if available, which takes a bitmask
    unreset_block_wait(1u << RESET_USBCTRL);

    // Optional: A short delay after hardware reset and before software re-init
    wait_ms(50);

    // Step 2: Re-initialize the ChibiOS/QMK USB driver software stack
    // USB_DRIVER is a common QMK macro for the ChibiOS USBDriver instance (e.g. &USBD1)
    // restart_usb_driver is provided by "protocol/chibios/usb_main.h"
    restart_usb_driver(&USB_DRIVER);
} 