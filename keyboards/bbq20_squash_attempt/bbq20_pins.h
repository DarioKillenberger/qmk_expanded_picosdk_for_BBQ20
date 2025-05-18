#pragma once

// Shared pin definitions for BBQ20

// Keyboard Matrix
// Rows: 8, Columns: 7 (COL8/GP19 is tied to 3v3, COL7 is GP21/QSPI_SS)
#define MATRIX_ROW_PINS { GP7, GP6, GP5, GP4, GP3, GP2, GP1, GP20 }
#define MATRIX_COL_PINS { GP8, GP9, GP14, GP13, GP12, GP11, GP19 } // COL1-COL7

// Peripherals
#define BBQ20_PERIPH_POWER_PIN GP17 // Pin to control power for peripherals like the trackpad

// Backlight
#define pin_BLK_KBD GP25
#define pin_BLK_TP  GP0 