#include "trackpad.h"


#define Trackpad_ADDR 0x3B


#define REG_PID    0x00
#define REG_REV    0x01
#define REG_MOTION 0x02

#define REG_DELTA_X 0x03
#define REG_DELTA_Y 0x04

#define pin_TP_SHUTDOWN GP24
#define pin_TP_MOTION GP22
#define pin_TP_RESET GP16
#define pin_BLK_KBD GP25
#define pin_BLK_TP GP0

#define BIT_MOTION_MOT (1 << 7)

#define trackpad_timeout 100

// Tracking variables for power management
bool trackpad_enabled = true;
uint32_t last_activity = 0;
static uint32_t last_motion_check = 0;

// External function from keyboard.c
extern void keyboard_activity_trigger(void);

// Function to check if trackpad button is pressed (active low)
bool is_trackpad_button_pressed(void) {
    return !readPin(pin_TP_MOTION);
}

// Power down the trackpad
void trackpad_sleep(void) {
    if (trackpad_enabled) {
        writePinLow(pin_TP_SHUTDOWN);
        trackpad_enabled = false;
    }
}

// Power up the trackpad and reset it
void trackpad_wake(void) {
    if (!trackpad_enabled) {
        writePinHigh(pin_TP_SHUTDOWN);
        wait_ms(10);
        writePinLow(pin_TP_RESET);
        wait_ms(100);
        writePinHigh(pin_TP_RESET);
        trackpad_enabled = true;
        last_activity = timer_read32();
    }
}

void pointing_device_init(void) {
    i2c_init();
    wait_ms(10);

    setPinOutput(pin_TP_SHUTDOWN);
    writePinHigh(pin_TP_SHUTDOWN); // Power on the trackpad initially

    setPinInputHigh(pin_TP_MOTION);

    setPinOutput(pin_TP_RESET);
    writePinLow(pin_TP_RESET);
    wait_ms(100);
    writePinHigh(pin_TP_RESET);
    
    trackpad_enabled = true;
    last_activity = timer_read32();
}


uint8_t read_register8(uint8_t reg) {
    if (!trackpad_enabled) return 0;
    uint8_t val = 0;
    i2c_read_register(Trackpad_ADDR, reg, &val, 1, trackpad_timeout);
    return val;
}

bool pointing_device_task(void) {
    uint32_t now = timer_read32();
    uint32_t poll_threshold = 10; // Limit polling rate for power saving
    
    // Throttle polling if needed
    if (timer_elapsed32(last_motion_check) < poll_threshold) {
        return false;
    }
    last_motion_check = now;
    
    // Check for trackpad button press to wake up if sleeping
    if (!trackpad_enabled && is_trackpad_button_pressed()) {
        trackpad_wake();
        keyboard_activity_trigger();
        return false;  // Skip this cycle to allow trackpad to initialize
    }
    
    // Skip processing if trackpad is asleep
    if (!trackpad_enabled) return false;
    
    // Optimization: Check the motion pin first (active low)
    if (readPin(pin_TP_MOTION)) {
        // No motion detected, don't do expensive I2C read
        return false;
    }
    
    const uint8_t ifmotion = read_register8(REG_MOTION);
    
    if (ifmotion & BIT_MOTION_MOT) {
        // Update activity timestamp
        last_activity = now;
        keyboard_activity_trigger();
        
        int8_t y = read_register8(REG_DELTA_X);
        int8_t x = read_register8(REG_DELTA_Y);
        
        x = ((x < 127) ? x : (x - 256)) * 2.0;
        y = ((y < 127) ? y : (y - 256)) * 2.0;

        report_mouse_t currentReport = pointing_device_get_report();
        currentReport.x = x;
        currentReport.y = y;
        pointing_device_set_report(currentReport);
        
        return pointing_device_send();
    }
    
    return false;
}
