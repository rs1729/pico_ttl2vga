#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "hsync_in_14M.pio.h"
#include "vsync_in_14M.pio.h"
#include "getpix.pio.h"

#include "ttl_in.h"


// CGA  640x200
// EGA1 640x200
void ttlIn_Init_14MHz(uint16_t div_int, uint8_t div_frac) {

    prelines = PRELINES_14M;
    ylinesrd = YLNS_CGA;
    xscanlrd = LINE_EGA;

    PIO pio = pio1;

    uint hsync_offset = pio_add_program(pio, &hsync_in_14M_program);
    uint vsync_offset = pio_add_program(pio, &vsync_in_14M_program);
    uint getpx_offset = pio_add_program(pio, &getpix_program);

    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint getpx_sm = 2;

    hsync_in_14M_program_init(pio, hsync_sm, hsync_offset, HSYNC_IN, div_int, div_frac);
    vsync_in_14M_program_init(pio, vsync_sm, vsync_offset, VSYNC_IN);
    getpix_program_init(pio, getpx_sm, getpx_offset, RED2_IN, div_int, div_frac);

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    pio_sm_put_blocking(pio, hsync_sm, ylinesrd-1 + prelines);
    pio_sm_put_blocking(pio, getpx_sm, xscanlrd-1);

    // start them all simultaneously
    pio_enable_sm_mask_in_sync(pio, (1u << hsync_sm) | (1u << vsync_sm) | (1u << getpx_sm));

}

