#ifndef PinMode_PRJ_H_INCLUDED
#define PinMode_PRJ_H_INCLUDED

#include "hwdefs.h"

#define DIG_IO_LIST \
    DIG_IO_ENTRY(IgnIn,       GPIOA, GPIO4,  PinMode::INPUT_PD)      \
    DIG_IO_ENTRY(LedOut,      GPIOC, GPIO13, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(SendDone,    GPIOB, GPIO9,  PinMode::OUTPUT)      \
    DIG_IO_ENTRY(Relay,       GPIOA, GPIO8,  PinMode::OUTPUT)      \
    DIG_IO_ENTRY(SensPower,   GPIOB, GPIO0,  PinMode::OUTPUT_OD)   \
    DIG_IO_ENTRY(WifiPower,   GPIOB, GPIO1,  PinMode::OUTPUT_OD)   \
    DIG_IO_ENTRY(BoardPower,  GPIOC, GPIO0,  PinMode::INPUT_PU)   \

#endif // PinMode_PRJ_H_INCLUDED
