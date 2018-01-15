/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _ST7586_H
#define _ST7586_H



#define ST7586_SOFT_RESET           0x01
#define ST7586_SLEEP_ON             0x10
#define ST7586_SLEEP_OFF            0x11

#define ST7586_PARTIAL_ON           0x12
#define ST7586_PARTIAL_OFF          0x13

#define ST7586_INVERSE              0x20
#define ST7586_INVERSE_OFF          0x20
#define ST7586_INVERSE_ON           0x21

#define ST7586_ALL_PIXELS           0x22
#define ST7586_ALL_PIXELS_OFF       0x22
#define ST7586_ALL_PIXELS_ON        0x23

#define ST7586_DISPLAY_OFF          0x28
#define ST7586_DISPLAY_ON           0x29

#define ST7586_SET_COLADDR          0x2a
#define ST7586_SET_ROWADDR          0x2b
#define ST7586_WRITE_DATA           0x2c
#define ST7586_READ_DATA            0x2e

#define ST7586_PARTIAL_AREA         0x30
#define ST7586_SCROLL_AREA          0x33

#define ST7586_SCAN_DIRECTION       0x36
#define ST7586_START_LINE           0x37

#define ST7586_DISPLAY_MODE_GRAY    0x38
#define ST7586_DISPLAY_MODE_MONO    0x39

#define ST7586_ENABLE_DDRAM         0x3a

#define ST7586_DISPLAY_DUTY         0xb0
#define ST7586_FIRST_OUTPUT_COM     0xb1
#define ST7586_FOSC_DIVIDER         0xb3
#define ST7586_PARTIAL_DISPLAY      0xb4
#define ST7586_SET_NLINE_INV        0xb5

#define ST7586_SET_RMW_ENABLE       0xb8
#define ST7586_SET_RMW_DISABLE      0xb9

#define ST7586_SET_VOP              0xc0
#define ST7586_VOP_INC              0xc1
#define ST7586_VOP_DEC              0xc2
#define ST7586_SET_BIAS             0xc3
#define ST7586_SET_BOOSTER          0xc4
#define ST7586_SET_VOP_OFFSET       0xc7
#define ST7586_ENABLE_ANALOG        0xd0

#define ST7586_AUTO_READ_CONTROL    0xd7


// control defines for gdispControl(g, control_field, value)
#define GDISP_CONTROL_INVERSE       (GDISP_CONTROL_LLD + 1)  // pass 0 for inverse off, 1 for inverse on
#define GDISP_CONTROL_ALL_PIXELS    (GDISP_CONTROL_LLD + 2) // 0 for all off, 1 for all on

#endif /* _ST7586_H */
