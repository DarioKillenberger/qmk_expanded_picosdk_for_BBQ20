#pragma once

#include "quantum.h"
#include <stdint.h>
#include "pointing_device.h"
#include "report.h"
#include "stdio.h"
#include "i2c_master.h"

#ifndef TRACKPAD_ADDRESS
#    define TRACKPAD_ADDRESS 0x3B
#endif
#define TRACKPAD_WRITE ((TRACKPAD_ADDRESS << 1) )
#define TRACKPAD_READ  ((TRACKPAD_ADDRESS << 1) )

// Function declarations
uint8_t read_register8(uint8_t reg);
void keyboard_activity_trigger(void);
bool is_trackpad_button_pressed(void);
void trackpad_sleep(void);
void trackpad_wake(void); 