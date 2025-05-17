MCU = RP2040
# INC += -I$(ROOT_DIR)/lib/pico-sdk/src/common/pico_stdlib/include # This is usually handled by the platform
# INC += -I$(ROOT_DIR)/lib/pico-extras/src/rp2_common/pico_sleep/include # Old attempt
# OPT_DEFS += -I$(ROOT_DIR)/lib/pico-extras/src/rp twice_common/pico_sleep/include # Reverted: Now handled by platform
OLED_ENABLE = yes                     # start using i2c 
POINTING_DEVICE_ENABLE = yes          # Mouse keys
POINTING_DEVICE_DRIVER = custom       # customer driver for trackpad on the BB9900 keyboard

# Power Management
BACKLIGHT_ENABLE = yes               # Enable backlight for keyboard LEDs
CONSOLE_ENABLE = no                  # Disable console to save power
COMMAND_ENABLE = no                  # Disable command to save power
MOUSEKEY_ENABLE = yes                # Enable mouse keys
EXTRAKEY_ENABLE = yes                # Enable extra keys
NKRO_ENABLE = no                     # Disable NKRO to save power
RGBLIGHT_ENABLE = no                 # Disable RGB to save power
AUDIO_ENABLE = no                    # Disable audio to save power
CORTEX_ENABLE_WFI_IDLE = yes         # Enable WFI idle for better power savings
LTO_ENABLE = yes                     # Enable Link Time Optimization for smaller code size

OPT_DEFS += -DHAL_USE_I2C=TRUE
# Force include pico_sdk_config_qmk.h to ensure assertion macros are defined early - Replaced by COMPILE_DEFS
# CFLAGS += -include pico_sdk_config_qmk.h

COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_TIMER=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_GPIO=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_ADC=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_DMA=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_I2C=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_INTERP=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_IRQ=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_PIO=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_PLL=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_PWM=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_RTC=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_SPI=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_SYSINFO=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_UART=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_XOSC=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLED_ROSC=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_ENABLE_ALL=0
COMPILE_DEFS += -DPARAM_ASSERTIONS_DISABLE_ALL=0

# Add support for files
SRC += trackpad.c
SRC += keyboard.c
# SRC += $(ROOT_DIR)/lib/pico-extras/src/rp2_common/pico_sleep/sleep.c # Reverted: Now handled by platform