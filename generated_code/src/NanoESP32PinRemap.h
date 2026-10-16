/* Copyright 2026 The MathWorks, Inc. */

#ifndef MW_NANOESP32_PIN_REMAP_H
#define MW_NANOESP32_PIN_REMAP_H

#include "rtwtypes.h"
#include <stdint.h>
// Pin remapping support
#if defined(BOARD_HAS_PIN_REMAP) && !defined(BOARD_USES_HW_GPIO_NUMBERS)

// Board-specific pin mapping table
static const int8_t TO_GPIO_NUMBER[] = {
    44,  // [ 0] D0, RX
    43,  // [ 1] D1, TX
    5,   // [ 2] D2
    6,   // [ 3] D3, CTS
    7,   // [ 4] D4, DSR
    8,   // [ 5] D5
    9,   // [ 6] D6
    10,  // [ 7] D7
    17,  // [ 8] D8
    18,  // [ 9] D9
    21,  // [10] D10, SS
    38,  // [11] D11, MOSI
    47,  // [12] D12, MISO
    48,  // [13] D13, SCK, LED_BUILTIN
    46,  // [14] LED_RED
    0,   // [15] LED_GREEN
    45,  // [16] LED_BLUE, RTS
    1,   // [17] A0, DTR
    2,   // [18] A1
    3,   // [19] A2
    4,   // [20] A3
    11,  // [21] A4, SDA
    12,  // [22] A5, SCL
    13,  // [23] A6
    14,  // [24] A7
};

// Convert digital pin to GPIO pin number
inline int8_t convertPinToGPIO(uint8_T pin) {
    return TO_GPIO_NUMBER[pin];
}

#else

// No remapping - direct pass-through
inline int8_t convertPinToGPIO(uint8_T pin) {
    return (int8_t)pin;
}

#endif

#endif // MW_NANOESP32_PIN_REMAP_H