MCU = RP2040
INC += -I$(ROOT_DIR)/lib/pico-sdk/src/common/pico_stdlib/include
OLED_ENABLE = yes                     # start using i2c 
POINTING_DEVICE_ENABLE = yes          # Mouse keys
POINTING_DEVICE_DRIVER = custom       # customer driver for trackpad on the BB9900 keyboard

# Power Management
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
# Add support for files
SRC += trackpad.c
SRC += keyboard.c