#pragma once
#include <cstdint>

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#ifndef WINDOWS
#define boolean bool;
#endif

#ifndef __cplusplus
#if !defined(__bool_true_false_are_defined)
#define false     FALSE
#define true      TRUE
#endif
#endif

#define DISABLE   0
#define ENABLE    1
#define DISABLED  0
#define ENABLED   1
#define OFF       0
#define ON        1
#define FALSE     0
#define TRUE      1
#define KO        0
#define OK        1
#define PASS      0
#define FAIL      1
#define LOW       0
#define HIGH      1
#define CLR       0
#define SET       1
#define INPUT     0
#define OUTPUT    1
typedef uint16_t word;
typedef uint8_t byte;