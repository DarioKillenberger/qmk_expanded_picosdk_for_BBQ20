#include QMK_KEYBOARD_H
#include "trackpad.h"

#define Trackpad_ADDR 0x3B

#define REG_PID        0x00
#define REG_REV        0x01
#define REG_MOTION     0x02
#define REG_DELTA_X    0x03
#define REG_DELTA_Y    0x04
#define REG_DELTA_XY_H 0x05
#define REG_CONFIG     0x11
#define REG_OBSERV     0x2E
#define REG_MBURST     0x42
#define REG_POWER      0x0F

#define pin_TP_SHUTDOWN GP24
#define pin_TP_MOTION GP22
#define pin_TP_RESET GP16
#define pin_BLK_KBD GP25
#define pin_BLK_TP GP0

#define BIT_MOTION_MOT (1 << 7)
#define BIT_MOTION_OVF (1 << 4)

#define trackpad_timeout 100
#define SCROLL_SPEED_DIVIDER   6
#define Boost_SPEED_Multiplyer 2
#define Solw_SPEED_DIVIDER     2
#define TRACKPAD_SLEEP_TIMEOUT 5000      // 5 seconds of inactivity before sleep
#define TRACKPAD_SUSPEND_POLL_DELAY 50   // Poll less frequently during USB suspend
#define SELF_SUSPEND_TIMEOUT 10000 // 10 seconds of inactivity for self-suspend

bool isScrollMode = false;
bool isBoostMode  = false;
bool isSlowMode   = false;
bool trackpad_enabled = false;
uint32_t last_activity = 0;
static uint32_t last_motion_check = 0;

// This variable is declared in keyboard.c
extern bool is_usb_suspended;

// Declare this function which is defined in keyboard.c
extern void keyboard_activity_trigger(void);

static bool is_self_suspended = false;

// Function to check if trackpad button is pressed
bool is_trackpad_button_pressed(void) {
    return !readPin(pin_TP_MOTION);  // Assuming active low
}

void trackpad_sleep(void) {
    if (trackpad_enabled) {
        writePinLow(pin_TP_SHUTDOWN);
        trackpad_enabled = false;
    }
}

void trackpad_wake(void) {
    if (!trackpad_enabled) {
        writePinHigh(pin_TP_SHUTDOWN);
        wait_ms(10);
        writePinLow(pin_TP_RESET);
        wait_ms(100);
        writePinHigh(pin_TP_RESET);
        trackpad_enabled = true;
        last_activity = timer_read32();
        
        // Notify the power management system
        keyboard_activity_trigger();
    }
}

void pointing_device_init(void) {
    i2c_init();
    wait_ms(10);

    setPinOutput(pin_TP_SHUTDOWN);
    setPinInputHigh(pin_TP_MOTION);
    setPinOutput(pin_TP_RESET);
    
    // Start with trackpad in sleep mode
    trackpad_sleep();
}

// Read a single byte from register
uint8_t read_register8(uint8_t reg) {
    if (!trackpad_enabled) return 0;

    uint8_t val = 0;
    i2c_read_register(Trackpad_ADDR, reg, &val, 1, trackpad_timeout);
    return val;
}

// Optimization: Read multiple bytes at once to reduce I2C overhead
bool read_multiple_registers(uint8_t start_reg, uint8_t *buffer, uint8_t length) {
    if (!trackpad_enabled) return false;
    
    return i2c_read_register(Trackpad_ADDR, start_reg, buffer, length, trackpad_timeout);
}

bool pointing_device_task(void) {
    // Defensive suspend: block all activity if either USB or self-suspend is active
    if (is_usb_suspended || is_self_suspended) {
        return false;
    }
    
    uint32_t now = timer_read32();
    uint32_t poll_threshold = 10;
    
    // Optimization: Early return if we're throttling polling
    if (timer_elapsed32(last_motion_check) < poll_threshold) {
        return false;
    }
    last_motion_check = now;
    
    // Check for trackpad button press to wake up
    if (!trackpad_enabled && is_trackpad_button_pressed()) {
        trackpad_wake();
        is_self_suspended = false; // Wake from self-suspend
        return false;  // Skip this cycle to allow trackpad to initialize
    }
    
    // Check if we should put trackpad to sleep (shorter timeout for hardware power saving)
    if (trackpad_enabled && (timer_elapsed32(last_activity) > TRACKPAD_SLEEP_TIMEOUT)) {
        trackpad_sleep();
        return false;
    }
    
    // Defensive self-suspend: if no activity for timeout, enter self-suspend
    if (!is_self_suspended && timer_elapsed32(last_activity) > SELF_SUSPEND_TIMEOUT) {
        is_self_suspended = true;
        trackpad_sleep();
        return false;
    }
    
    if (!trackpad_enabled) return false;
    
    // Optimization: Use pin input to check motion before I2C communication
    if (readPin(pin_TP_MOTION)) {
        // No motion detected by pin, skip I2C reading
        return false;
    }
    
    // Read motion register and delta values in a single I2C transaction when possible
    uint8_t data_buffer[3] = {0}; // Buffer for motion, delta_x, delta_y
    
    // First check if motion is detected
    const uint8_t ifmotion = read_register8(REG_MOTION);
    
    if (ifmotion & BIT_MOTION_MOT) {
        last_activity = now;
        if (is_self_suspended) {
            is_self_suspended = false;
            trackpad_wake();
        }
        // Notify the power management system
        keyboard_activity_trigger();
        
        // Optimization: Read both X and Y deltas in a single I2C transaction
        if (read_multiple_registers(REG_DELTA_X, &data_buffer[0], 2)) {
            int8_t x = data_buffer[0]; // REG_DELTA_X
            int8_t y = data_buffer[1]; // REG_DELTA_Y
            
            report_mouse_t currentReport = pointing_device_get_report();
            
            // Optimization: Combine calculation logic for different modes
            float scaleFactor = 1.5;
            if (isBoostMode) {
                scaleFactor *= Boost_SPEED_Multiplyer;
            } else if (isSlowMode) {
                scaleFactor /= Solw_SPEED_DIVIDER;
            }
            
            if (isScrollMode) {
                currentReport.h = 1 * x/SCROLL_SPEED_DIVIDER;
                currentReport.v = -1 * y/SCROLL_SPEED_DIVIDER;
            } else {
                x = ((x < 127) ? x : (x - 256)) * scaleFactor;
                y = ((y < 127) ? y : (y - 256)) * scaleFactor;
                currentReport.x = x;
                currentReport.y = y;
            }
            
            // Only send if there is actual movement or button press
            if (currentReport.x != 0 || currentReport.y != 0 || currentReport.h != 0 || currentReport.v != 0 || currentReport.buttons != 0) {
                pointing_device_set_report(currentReport);
                return pointing_device_send();
            }
        }
    }
    // No movement, do not send any report
    return false;
} 