BACKLIGHT_ENABLE = yes

# Power Management
CONSOLE_ENABLE = no                  # Disable console to save power
COMMAND_ENABLE = no                  # Disable command to save power
NKRO_ENABLE = no                     # Disable NKRO to save power
RGBLIGHT_ENABLE = no                 # Disable RGB to save power
AUDIO_ENABLE = no                    # Disable audio to save power
CORTEX_ENABLE_WFI_IDLE = yes         # Enable WFI idle for better power savings

# Note: We're using the root version of keyboard.c and trackpad.c
# which have the proper power management implementation

# Use the keyboard files from root directory
# SRC += ../keyboard.c                # Removed to avoid conflicts
