# SRC += lib/pico-extras/src/rp2_common/pico_sleep/sleep.c
# INC += lib/pico-extras/src/rp2_common/pico_sleep/include
OLED_ENABLE = yes                     # start using i2c 
POINTING_DEVICE_ENABLE = yes          # Mouse keys
POINTING_DEVICE_DRIVER = custom       # customer driver for trackpad on the BB9900 keyboard


OPT_DEFS += -DHAL_USE_I2C=TRUE
# Add support for the BB9900 trackpad
SRC += trackpad.c

MCU = RP2040

# Power saving configuration
SLEEP_LED_ENABLE = no       # Breathing sleep LED during USB suspend
SRC += keyboard.c           # Include our power management file
WPM_ENABLE = no             # Disable WPM counter to save power
CONSOLE_ENABLE = no         # Disable console to save power
COMMAND_ENABLE = no         # Disable command to save power
MOUSEKEY_ENABLE = yes       # Mouse keys
EXTRAKEY_ENABLE = yes       # Audio control and System control
NKRO_ENABLE = no            # Disable NKRO to save power
BACKLIGHT_ENABLE = no       # Disable backlight to save power
RGBLIGHT_ENABLE = no        # Disable RGB to save power
AUDIO_ENABLE = no           # Disable audio to save power
SPLIT_KEYBOARD = no         # No split keyboard
WAIT_FOR_USB = no           # Faster startup
DEFERRED_EXEC_ENABLE = no   # Disable deferred execution
DYNAMIC_MACRO_ENABLE = no   # Disable dynamic macros to save power
LTO_ENABLE = yes            # Enable Link Time Optimization for smaller code size

# Enable dormant mode for RP2040
RP2040_BOOTLOADER_DOUBLE_TAP_RESET = yes
RP2040_BOOTLOADER_DOUBLE_TAP_RESET_LED = GP25