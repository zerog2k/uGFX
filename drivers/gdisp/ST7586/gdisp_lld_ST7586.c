/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if GFX_USE_GDISP

#define GDISP_DRIVER_VMT			GDISPVMT_ST7586
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "board_ST7586.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		160
#endif
#ifndef GDISP_SCREEN_WIDTH
        // leftmost 22 horizontal pixels do not display
	#define GDISP_SCREEN_WIDTH		360
        #define GDISP_SCREEN_WIDTH_BYTES        (GDISP_SCREEN_WIDTH/8)
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	51
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

#define GDISP_FLG_NEEDFLUSH			(GDISP_FLG_DRIVER<<0)

#include "st7586.h"

#define FRAMEBUFFER_SIZE    (GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH_BYTES)
static uint8_t display_buffer[GDISP_SCREEN_HEIGHT][GDISP_SCREEN_WIDTH_BYTES];
#define DISPLAY_OFFSET  21/3      // horizontal shift between display and controller

/*===========================================================================*/
/* Driver config defaults for backward compatibility.               	     */
/*===========================================================================*/


/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define RAM(g)							((uint8_t *)g->priv)
#define write_cmd2(g, cmd1, cmd2)	{ write_cmd(g, cmd1); write_cmd(g, cmd2); }
#define write_cmd3(g, cmd1, cmd2, cmd3)	{ write_cmd(g, cmd1); write_cmd(g, cmd2); write_cmd(g, cmd3); }

#define write_arg2(g, arg1, arg2)       { write_arg(g, arg1); write_arg(g, arg2); }
#define write_arg4(g, arg1, arg2, arg3, arg4)       { write_arg(g, arg1); write_arg(g, arg2); write_arg(g, arg3); write_arg(g, arg4); }

#define _C(c1)                      { write_cmd(g, c1); }
#define _CA(c1, a1)                 { _C(c1); write_arg(g, a1); }
#define _CAA(c1, a1, a2)            { _CA(c1, a1); write_arg(g, a2); }
#define _CAAAA(c1, a1, a2, a3, a4)  { _CAA(c1, a1, a2); write_arg(g, a3); write_arg(g, a4); }

// Some common routines and macros
#define _delay(us)		gfxSleepMicroseconds(us)
#define _delay_ms(ms)		gfxSleepMilliseconds(ms)

//#define byteaddr(x, y)		(((y) * (GDISP_SCREEN_WIDTH_BYTES)) + ((x)/8))
#define bitaddr(x)		(1 << (7 - ((x) % 8)))


uint16_t byteaddr(int16_t x, int16_t y)
{
   return (y * GDISP_SCREEN_WIDTH_BYTES + (x/8));
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/*
 * As this controller can't update on a pixel boundary we need to maintain the
 * the entire display surface in memory so that we can do the necessary bit
 * operations. Fortunately it is a small display in monochrome.
 * 64 * 128 / 8 = 1024 bytes.
 */

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
        NRF_LOG_INFO("gdisp_lld_init\n");
	// The private area is the display surface.
	//g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH_BYTES);
        memset(display_buffer, 0, FRAMEBUFFER_SIZE);
	g->priv = display_buffer;
        if (!g->priv) {
            return FALSE;
	}

	// Initialise the board interface
	init_board(g);

	// Hardware reset
	setpin_reset(g, 0); // active low
	gfxSleepMilliseconds(20);
	setpin_reset(g, 1); 
	gfxSleepMilliseconds(20);

	acquire_bus(g);
        _C(ST7586_SOFT_RESET);           // soft reset
        _CA(ST7586_AUTO_READ_CONTROL, 0x9f);    // disable auto read control
        _CA(0xe0, 0x00);    // enable otp read
        _delay_ms(50);
        _C(0xe3);           // otp upload procedure
        _delay_ms(50);
        _C(0xe1);           // otp control out
        _C(ST7586_SLEEP_OFF);           // sleep out mode
        _C(ST7586_DISPLAY_OFF);           // display off
        _delay_ms(50);   
        _CAA(ST7586_SET_VOP, 0x00, 0x01); // set vop, 100h (contrast)
        _CA(ST7586_SET_BIAS, 0x05);        // set bias system, 1/9
        _CA(ST7586_SET_BOOSTER, 0x07);        // set booster level, 8x
        _CA(ST7586_SET_NLINE_INV, 0x8d);        // set n-line inversion
        _C(ST7586_DISPLAY_MODE_GRAY);               // set display gray mode????
        _CA(ST7586_ENABLE_DDRAM, 0x02);        // enable ddram interface?
        _CA(ST7586_SCAN_DIRECTION, 0xc8);        // scan direction, MY=1, MX=1
        //_CA(0x36, 0x08);        // scan direction
        _CA(ST7586_DISPLAY_DUTY, 0x9f);        // display duty cycle
        _C(ST7586_INVERSE_OFF);               // display invert off
        //_C(0x21);             // display invert on
        _CAAAA(ST7586_SET_COLADDR, 0x00, 0x2b, 0x00, 0x5a);  // col addr, XS, XE
        _CAAAA(ST7586_SET_ROWADDR, 0x00, 0x00, 0x00, 0x9f);  // row addr, YS, YE
        _CA(ST7586_FIRST_OUTPUT_COM, 0x00);        // first output COM, FC=00h
        _CAAAA(0xf1, 0x15, 0x15, 0x15, 0x15);   // frame rate, mono-mode, 92Hz
        _CA(0x25, 0xc3);           //?
        _C(ST7586_DISPLAY_ON);
        // Finish Init
        post_init_board(g);

 	// Release the bus
	release_bus(g);

	// Initialise the GDISP structure
	g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;

	return TRUE;
}

#if GDISP_HARDWARE_FLUSH
  LLDSPEC void gdisp_lld_flush(GDisplay *g) {
    uint32_t elapsed = gfxSystemTicks();
    uint16_t  row = 0;
    uint32_t input = 0;
    uint8_t output[8];
    #define ROW_OUT_BYTES   (GDISP_SCREEN_WIDTH/3)
    //uint8_t rowbuf[ROW_OUT_BYTES];
    uint8_t   i, col, outidx, byte = 0;

    // Don't flush if we don't need it.
    if (!(g->flags & GDISP_FLG_NEEDFLUSH)) {
      elapsed = gfxSystemTicks() - elapsed;
      NRF_LOG_INFO("gdisp_lld_noflush: %d\n", elapsed);
      return;
    }
    acquire_bus(g);
      write_cmd(g, ST7586_SET_ROWADDR);
      write_arg4(g, 0, 0, 0, GDISP_SCREEN_HEIGHT); 
      write_cmd(g, ST7586_SET_COLADDR);
      write_arg4(g, 0, DISPLAY_OFFSET, 0, DISPLAY_OFFSET + (ROW_OUT_BYTES)-1);
      write_cmd(g, ST7586_WRITE_DATA);
    do {
      // row loop
      col = 0;
      //outidx = 0;
      do {
        // column loop
        // crazy mapping each 3 column pixels to 1 output byte
        // so take 3 bytes (24 bits) input, and output 8 bytes to controller
        input = (display_buffer[row][col] << 16) |  (display_buffer[row][col+1] << 8) | (display_buffer[row][col+2]);
        for (i=0; i<8; i++)
        {
          byte = 0;
          if (input & (0b10000000 << 16))
              byte |= 0b11000000;
          if (input & (0b01000000 << 16))
              byte |= 0b00011000;
          if (input & (0b00100000 << 16))
              byte |= 0b00000011;
          output[i] = byte;
          //rowbuf[outidx+i] = byte;
          input <<= 3;
        }

        write_data(g, output, 8);
        //memcpy(rowbuf, output, 8);
        //*rowbuf += 8;
        //outidx += 8;
        col += 3;
      } while ( col < (GDISP_SCREEN_WIDTH_BYTES) );
      //write_data(g, rowbuf, ROW_OUT_BYTES);
    } while (row++ < GDISP_SCREEN_HEIGHT);
    release_bus(g);    
    g->flags &= ~GDISP_FLG_NEEDFLUSH;
    elapsed = gfxSystemTicks() - elapsed;
    NRF_LOG_INFO("gdisp_lld_flush: %d\n", elapsed);
  }
#endif

#if GDISP_HARDWARE_DRAWPIXEL
	LLDSPEC void gdisp_lld_draw_pixel(GDisplay *g) {
		coord_t		x, y;
                uint16_t    addr;
                uint8_t     mask;
		switch(g->g.Orientation) {
		default:
		case GDISP_ROTATE_0:
			x = g->p.x;
			y = g->p.y;
			break;
		case GDISP_ROTATE_90:
			x = g->p.y;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.x;
			break;
		case GDISP_ROTATE_180:
			x = GDISP_SCREEN_WIDTH-1 - g->p.x;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			break;
		case GDISP_ROTATE_270:
			x = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			y = g->p.x;
			break;
		}
                //addr = byteaddr(x,y);
                mask = bitaddr(x);
		if (gdispColor2Native(g->p.color) != Black)
			display_buffer[y][x/8] |= mask;
		else
			display_buffer[y][x/8] &= ~mask;
		g->flags |= GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_PIXELREAD
	LLDSPEC color_t gdisp_lld_get_pixel_color(GDisplay *g) {
		coord_t		x, y;

		switch(g->g.Orientation) {
		default:
		case GDISP_ROTATE_0:
			x = g->p.x;
			y = g->p.y;
			break;
		case GDISP_ROTATE_90:
			x = g->p.y;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.x;
			break;
		case GDISP_ROTATE_180:
			x = GDISP_SCREEN_WIDTH-1 - g->p.x;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			break;
		case GDISP_ROTATE_270:
			x = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			x = g->p.x;
			break;
		}
		return (RAM(g)[byteaddr(x, y)] & bitaddr(x)) ? White : Black;
	}
#endif

#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
  LLDSPEC void gdisp_lld_control(GDisplay *g) {
          switch(g->p.x) {
          case GDISP_CONTROL_POWER:
            if (g->g.Powermode == (powermode_t)g->p.ptr)
                    return;
            switch((powermode_t)g->p.ptr) {
              case powerOff:
              case powerSleep:
              case powerDeepSleep:
                  acquire_bus(g);
                  write_cmd(g, ST7586_DISPLAY_OFF);
                  write_cmd(g, ST7586_SLEEP_ON);
                  release_bus(g);
                  break;
              case powerOn:
                  acquire_bus(g);
                  write_cmd(g, ST7586_SLEEP_OFF);
                  write_cmd(g, ST7586_DISPLAY_ON);
                  release_bus(g);
                  break;
              default:
                  return;
            }
            g->g.Powermode = (powermode_t)g->p.ptr;
            return;

          case GDISP_CONTROL_ORIENTATION:
            if (g->g.Orientation == (orientation_t)g->p.ptr)
                    return;
            switch((orientation_t)g->p.ptr) {
            // Rotation is handled by the drawing routines
            case GDISP_ROTATE_0:
            case GDISP_ROTATE_180:
                    g->g.Height = GDISP_SCREEN_HEIGHT;
                    g->g.Width = GDISP_SCREEN_WIDTH;
                    break;
            case GDISP_ROTATE_90:
            case GDISP_ROTATE_270:
                    g->g.Height = GDISP_SCREEN_WIDTH;
                    g->g.Width = GDISP_SCREEN_HEIGHT;
                    break;
            default:
                    return;
            }
            g->g.Orientation = (orientation_t)g->p.ptr;
            return;

          case GDISP_CONTROL_CONTRAST:
            if ((unsigned)g->p.ptr > 100)
                g->p.ptr = (void *)100;
            acquire_bus(g);
            //_CAA(ST7586_SET_VOP, ((((unsigned)g->p.ptr)<<6)/101) & 0x3F, 0x01);
            release_bus(g);
            g->g.Contrast = (unsigned)g->p.ptr;
            return;

          case GDISP_CONTROL_ALL_PIXELS:
            if ((unsigned)g->p.ptr > 1)
                g->p.ptr = (void * )1;            
            acquire_bus(g);
            _C(ST7586_ALL_PIXELS | (unsigned) g->p.ptr);
            release_bus(g);
          return;
          
          case GDISP_CONTROL_INVERSE:
            if ((unsigned)g->p.ptr > 1)
                g->p.ptr = (void *) 1;            
            acquire_bus(g);
            _C(ST7586_INVERSE | (unsigned) g->p.ptr);
            release_bus(g);
          return;
          }
  }
#endif // GDISP_NEED_CONTROL

#endif // GFX_USE_GDISP
