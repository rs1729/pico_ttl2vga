#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

/*
    VGA output
    cf. https://vanhunteradams.com/Pico/VGA/VGA.html
*/

#include "hsync_out.pio.h"
#include "vsync_out.pio.h"
#include "rgb_out.pio.h"

#include "vga_out.h"
#include "font5x7.h"

#ifdef NO_COMMON
unsigned char vga_data_array[FRMBUFSZ+4];
#endif

char *address_pointer = vga_data_array; //= &vga_data_array[0];

void initVGA() {

    PIO pio = pio0;

    uint hsync_offset = pio_add_program(pio, &hsync_program);
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    uint rgb_offset = pio_add_program(pio, &rgb_program);

    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint rgb_sm = 2;

    hsync_program_init(pio, hsync_sm, hsync_offset, HSYNC, PCLK);
    vsync_program_init(pio, vsync_sm, vsync_offset, VSYNC, PCLK);
    rgb_program_init(pio, rgb_sm, rgb_offset, RED_PIN2, PCLK);

    // Initialize PIO state machine counters
    pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
    pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
    pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // PIO DMA Channels

    // DMA channels - 0 sends color data, 1 reconfigures and restarts 0
    int rgb_chan_0 = 0;
    int rgb_chan_1 = 1;

    // Channel Zero (sends color data to PIO VGA machine)
    dma_channel_config c0 = dma_channel_get_default_config(rgb_chan_0);  // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);              // 8-bit txfers
    channel_config_set_read_increment(&c0, true);                        // yes read incrementing
    channel_config_set_write_increment(&c0, false);                      // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX2) ;                        // DREQ_PIO0_TX2 pacing (FIFO)
    channel_config_set_chain_to(&c0, rgb_chan_1);                        // chain to other channel

    dma_channel_configure(
        rgb_chan_0,                 // Channel to be configured
        &c0,                        // The configuration we just created
        &pio->txf[rgb_sm],          // write address (RGB PIO TX FIFO)
        &vga_data_array,            // The initial read address (pixel color array)
        FRMBUFSZ,                   // Number of transfers; in this case each is 1 byte
        false                       // Don't start immediately.
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c1 = dma_channel_get_default_config(rgb_chan_1);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                        // no read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, rgb_chan_0);                         // chain to other channel

    dma_channel_configure(
        rgb_chan_1,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &dma_hw->ch[rgb_chan_0].read_addr,  // Write address (channel 0 read address)
        &address_pointer,                   // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // start them all simultaneously
    pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

    dma_start_channel_mask((1u << rgb_chan_0)) ;
}


#define _width  XACTIVE //720
#define _height YACTIVE //351

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

int cursor_y, cursor_x;
char textcolor, textbgcolor;

void drawPixel(int x, int y, char color) {
    if (x < 0 || x >= _width) return;
    if (y < 0 || y >= _height) return;

    //color &= 0x3F; // 6 bit

    vga_data_array[VGALNBF*y + x] = color;
    //vga_data_array[VGALNBF*y + x] ^= color;
}

void drawChar(int x, int y, unsigned char c, char color, char bg) {
    int i, j;

    if ( x < 0 || x >= _width ||
         y < 0 || y >= _height ) return;

    for (i=0; i<6; i++) {
        unsigned char line;
        if (i == 5) line = 0x0;
        else        line = pgm_read_byte(font+(c*5)+i);
        for (j=0; j<8; j++) {
            if (line & 0x1) {
                drawPixel(x+i, y+j, color);
            }
            else if (bg != color) {
                drawPixel(x+i, y+j, bg);
            }
            line >>= 1;
        }
    }
}

void tft_write(unsigned char c) {
    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor);
    cursor_x += 6;
    if (cursor_x > _width-6) {
      cursor_y += 8;
      cursor_x = 0;
    }
}

inline void wrtxt(int x, int y, char *str, char color) {
    textcolor = color;
    cursor_x = x;
    cursor_y = y;
    while (*str) {
        tft_write(*str++);
    }
}

