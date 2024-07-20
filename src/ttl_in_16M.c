#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "hsync_in_16M.pio.h"
#include "vsync_in_16M.pio.h"
#include "getpix.pio.h"

#include "ttl_in.h"


// MDA  720x350
// EGA2 640x350
// MDA640: 720 -> 640 (16MHz -> 14MHz)
void ttlIn_Init_16MHz(uint16_t div_int, uint8_t div_frac, int vm) {

    if (vm == MDA) {
        prelines = PRELINES_MDA;
        xscanlrd = LINE_SCAN_MDA; // 720 or 720*8/9=640
    }
    else {
        prelines = PRELINES_16M;
        xscanlrd = LINE_EGA; // 640
    }
    ylinesrd = YLNS_MDA; // 350

    PIO pio = pio1;

    uint hsync_offset = pio_add_program(pio, &hsync_in_16M_program);
    uint vsync_offset = pio_add_program(pio, &vsync_in_16M_program);
    uint getpx_offset = pio_add_program(pio, &getpix_program);

    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint getpx_sm = 2;

    hsync_in_16M_program_init(pio, hsync_sm, hsync_offset, HSYNC_IN, div_int, div_frac);
    vsync_in_16M_program_init(pio, vsync_sm, vsync_offset, VSYNC_IN);
    getpix_program_init(pio, getpx_sm, getpx_offset, RED2_IN, div_int, div_frac);

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    pio_sm_put_blocking(pio, hsync_sm, ylinesrd-1 + prelines);
    pio_sm_put_blocking(pio, getpx_sm, xscanlrd-1);

    // start them all simultaneously
    pio_enable_sm_mask_in_sync(pio, (1u << hsync_sm) | (1u << vsync_sm) | (1u << getpx_sm));

}


