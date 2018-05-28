#pragma once
#define boolean bool;

#ifndef __cplusplus
#if !defined(__bool_true_false_are_defined)
#define false     FALSE
#define true      TRUE
#endif
#endif

#ifndef HIGH
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
#define word unsigned int
#define byte unsigned char
#endif