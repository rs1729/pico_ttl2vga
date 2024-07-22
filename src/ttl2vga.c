
/*
    simple converter  DB9 TTL (MDA/CGA/EGA)  to  DB15 analog VGA

    IN : GP8..GP15     (ttl_in.h)
    OUT: GP0..GP7      (vga_out.h)

    OSD: GP18      -  show video mode on screen
    CLK: GP19..20  -  adjust pixel clock / horizontal width
    PAL: GP21      -  toggle palette: (a) CGA <-> EGA , (b) MDA mono/intensity <-> dark/light green
    DBG: GP22      -  signal reading pixel into frame buffer
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

#include "vga_out.h"
#include "ttl_in.h"


// DBG_USB 0: set  vga_out.h:YACTIVE=351 , tth_in.h:YLNS_MDA=351 , CMakeLists.txt:pico_enable_stdio_usb(ttl2vga 0)
// DBG_USB 1: set  vga_out.h:YACTIVE=349 , ttl_in.h:YLNS_MDA=349 , CMakeLists.txt:pico_enable_stdio_usb(ttl2vga 1)
#define DBG_USB 0  // interference
#define DBG_PIN 0

#define TOGGLEPAL 1
#define ADJ_CLK 1


#if DBG_PIN
const uint TP_FRMBUF_IN = 22; // GP22 = pin29
#endif


#if TOGGLEPAL
const uint BUTTON_PAL = 21; // GP21 = pin27
#endif

#if ADJ_CLK
const uint BUTTON_PLS = 20; // GP20 = pin26
const uint BUTTON_MIN = 19; // GP19 = pin25
const uint BUTTON_OSD = 18; // GP18 = pin24
#endif


// TTL input pio: pio=pio1
// TTL pixel state machine: sm=2
#define _PIO  pio1
#define _SM   2

// LSB: rRgGbB
#define MASK_GREEN2INT 0x04040404
#define MASK_BLUE2MONO 0x10101010


ttlmode_t *ttlmode = &mode_EGA2;


void scan_in(int pal) {

    while ( !(_PIO->irq & (1u<<6)) ) _PIO->rxf[_SM];  // sync with vsync_in, empty FIFO

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < ttlmode->prelines; y++) {

        // BP / h-offset
        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

        for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            _PIO->rxf[_SM];
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < ttlmode->ylinesrd; y++) {

        // BP / h-offset
        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

        for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            uint32_t col4 = _PIO->rxf[_SM] & 0x3F3F3F3F;
            // pal := (ttlmode->vmode == MDA && (ttlmode->pal & 0x1))
            if (pal) col4 = (col4 & MASK_GREEN2INT) | ((col4 & MASK_BLUE2MONO)>>1);
            //(a) VGALNBF==VGALINE: if (x == VGALINE-4) col4 &= 0x1F3F3F3F; // MDA pixel 720, bit 6
            //(b) VGALNBF==VGALINE+4:
            *((uint32_t*)(vga_data_array+(VGALNBF*y + x))) = col4;  // 4 byte aligned ?
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    #endif
}

void scan2_in(int pal) {

    while ( !(_PIO->irq & (1u<<6)) ) _PIO->rxf[_SM];  // sync with vsync_in, empty FIFO

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < ttlmode->prelines; y++) {

        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

        for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            _PIO->rxf[_SM];
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    uint n = 0;
    for (uint y = 0; y < ttlmode->ylinesrd; y++, n++) {  //n = 0 .. 3*ylinesrd/2

        // BP / h-offset
        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

        for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            uint32_t col4 = _PIO->rxf[_SM] & 0x3F3F3F3F;
            if (pal)
            {
                if ( col4 & 0x00000004 ) col4 |= 0x00000015;
                if ( col4 & 0x00000400 ) col4 |= 0x00001500;
                if ( col4 & 0x00040000 ) col4 |= 0x00150000;
                if ( col4 & 0x04000000 ) col4 |= 0x15000000;
            }
            *((uint32_t*)(vga_data_array+(VGALNBF*n + x))) = col4;
        }
        if (y & 1) {  // duplicate every second line
            memcpy(vga_data_array+VGALNBF*(n+1), vga_data_array+VGALNBF*n, ttlmode->xscanlrd);
            n++;
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    #endif
}

void scan2ega_in(/*int pal*/) {

    while ( !(_PIO->irq & (1u<<6)) ) _PIO->rxf[_SM];  // sync with vsync_in, empty FIFO

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < ttlmode->prelines; y++) {

        // BP / h-offset
        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

        for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            _PIO->rxf[_SM];
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    uint n = 0;
    for (uint y = 0; y < ttlmode->ylinesrd; y++, n++) {  //n = 0 .. 3*ylinesrd/2

        // BP / h-offset
        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

       for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            uint32_t col4 = _PIO->rxf[_SM] & 0x3F3F3F3F;
            /*
            if (pal == PAL_CGA)
            {
                if ( col4 & 0x00000004 ) col4 |= 0x00000015;
                if ( col4 & 0x00000400 ) col4 |= 0x00001500;
                if ( col4 & 0x00040000 ) col4 |= 0x00150000;
                if ( col4 & 0x04000000 ) col4 |= 0x15000000;
            }
            */
            *((uint32_t*)(vga_data_array+(VGALNBF*n + x))) = col4;
        }
        if (y & 1) {  // duplicate every second line
            memcpy(vga_data_array+VGALNBF*(n+1), vga_data_array+VGALNBF*n, ttlmode->xscanlrd);
            n++;
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    #endif
}

void scan2cga_in(/*int pal*/) {

    while ( !(_PIO->irq & (1u<<6)) ) _PIO->rxf[_SM];  // sync with vsync_in, empty FIFO

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < ttlmode->prelines; y++) {

        // BP / h-offset
        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

        for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            _PIO->rxf[_SM];
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    uint n = 0;
    for (uint y = 0; y < ttlmode->ylinesrd; y++, n++) {  //n = 0 .. 3*ylinesrd/2

        // BP / h-offset
        while ( !(_PIO->irq & (1u<<7)) ) {};
        busy_wait_at_least_cycles(ttlmode->h_offset);
        pio_sm_exec(_PIO, _SM, 0xc004); // 0xc004: irq nowait 4

        for (uint x = 0; x < ttlmode->xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            uint32_t col4 = _PIO->rxf[_SM] & 0x3F3F3F3F;
            //if (pal == PAL_CGA)
            {
                if ( col4 & 0x00000004 ) col4 |= 0x00000011;
                if ( col4 & 0x00000400 ) col4 |= 0x00001100;
                if ( col4 & 0x00040000 ) col4 |= 0x00110000;
                if ( col4 & 0x04000000 ) col4 |= 0x11000000;
            }
            *((uint32_t*)(vga_data_array+(VGALNBF*n + x))) = col4;
        }
        if (y & 1) {  // duplicate every second line
            memcpy(vga_data_array+VGALNBF*(n+1), vga_data_array+VGALNBF*n, ttlmode->xscanlrd);
            n++;
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    #endif
}

void reset_TTLin() {
    pio_set_sm_mask_enabled(_PIO, (1u << 0) | (1u << 1) | (1u << 2), false);
    pio_clear_instruction_memory(_PIO);
}


void inc_clk() {
    ttlmode->div_frac += 1;
    if (ttlmode->div_frac > 255) {
        ttlmode->div_int += 1;
        ttlmode->div_frac = 0;
    }
    if (ttlmode->div_int > 2) {
        ttlmode->div_int = 2;
        ttlmode->div_frac = 255;
    }
}

void dec_clk() {
    ttlmode->div_frac -= 1;
    if (ttlmode->div_frac < 0) {
        ttlmode->div_int -= 1;
        ttlmode->div_frac = 255;
    }
    if (ttlmode->div_int < 1) {
        ttlmode->div_int = 1;
        ttlmode->div_frac = 0;
    }
}

void inc_hofs() {
    ttlmode->h_offset += OFS_STEP;
    if (ttlmode->h_offset > 1023) ttlmode->h_offset = 1023;
}
void dec_hofs() {
    ttlmode->h_offset -= OFS_STEP;
    if (ttlmode->h_offset < 0) ttlmode->h_offset = 0;
}

int toggle_pal() {
    int _toggle_hscan = 0;
    uint16_t hscan0 = ttlmode->xscanlrd;
    switch (ttlmode->vmode)
    {
        case CGAEGA:// pal = (pal != PAL_CGA) ? PAL_CGA : PAL_EGA;
                    ttlmode->pal ^= 0x1;
                    break;
        case MDA:   // pal = (pal != PAL_MDA) ? PAL_MDA : PAL_EGA;
                    hscan0 = ttlmode->xscanlrd;
                    ttlmode->pal += 1;
                    ttlmode->pal &= 0x3;
                    ttlmode->xscanlrd = (ttlmode->pal & 0x2) ? 640 : 720;
                    if (ttlmode->xscanlrd != hscan0) {
                        _toggle_hscan = 1;
                        float d0 = ttlmode->div_int + ttlmode->div_frac/256.0f;
                        float d1 = d0 * hscan0/(float)ttlmode->xscanlrd;
                        // hscan0/xscanlrd == (pal & 0x2) ? 8/9 : 9/8
                        ttlmode->div_int = (uint16_t)d1;
                        ttlmode->div_frac = (int16_t)((d1-ttlmode->div_int)*256.0f);
                        if (ttlmode->div_int > 2) {
                            ttlmode->div_int = 2;
                            ttlmode->div_frac = 255;
                        }
                        if (ttlmode->div_int < 1) {
                            ttlmode->div_int = 1;
                            ttlmode->div_frac = 0;
                        }
                    }
                    break;
    }
    return _toggle_hscan;
}

// OSD menu
const int menu_posX = 50;
enum menu_items { MENU_CLK=0, MENU_hOFS, MENU_PAL };
int menu_pos[] = { 60, 70, 90 };
int menu_select = 0;

int main() {

    //uint64_t t0 = -1 //time_us_64()
    uint32_t t0 = -1; //time_us_32()
    uint32_t t1 = 0;
    uint32_t dt = 0;

    uint16_t hpix = LINE_EGA;

    int vmode = ttlmode->vmode,
        vmode0 = vmode;
    int pal = 0;
    int toggle_hscan = 0;
    int adj_clk = 0;

    uint32_t avg = 0;
    uint32_t n = 0;
    uint8_t avgbuff[5] = {0};
    uint8_t pol_VSYNC = 0;  // 1:V+ , 0:V-
    uint8_t cnt = 0;

    uint32_t vm_fail_cnt = 0;

    char txtbuf[32] = { 0 };


    stdio_init_all();


    #if DBG_PIN
    gpio_init(TP_FRMBUF_IN);
    gpio_set_dir(TP_FRMBUF_IN, GPIO_OUT);
    gpio_put(TP_FRMBUF_IN, 0);
    #endif

    #if TOGGLEPAL
    gpio_init(BUTTON_PAL);
    gpio_set_dir(BUTTON_PAL, GPIO_IN);
    gpio_pull_up(BUTTON_PAL);    // connect PIN - BUTTON - GND
    #endif

    #if ADJ_CLK
    gpio_init(BUTTON_PLS);
    gpio_set_dir(BUTTON_PLS, GPIO_IN);
    gpio_pull_up(BUTTON_PLS);    // connect PIN - BUTTON - GND

    gpio_init(BUTTON_MIN);
    gpio_set_dir(BUTTON_MIN, GPIO_IN);
    gpio_pull_up(BUTTON_MIN);    // connect PIN - BUTTON - GND

    gpio_init(BUTTON_OSD);
    gpio_set_dir(BUTTON_OSD, GPIO_IN);
    gpio_pull_up(BUTTON_OSD);    // connect PIN - BUTTON - GND
    #endif

    gpio_init(VSYNC_IN);
    gpio_set_dir(VSYNC_IN, GPIO_IN);
    gpio_pull_down(VSYNC_IN); // default avgV=0:16M


    // set_sys_clock_khz(114000, true);
    // 114.0 MHz  ->  70.1375Hz / 31.49kHz (720x400)
    uint vco_freq = 1596000000;
    uint post_div1 = 7;
    uint post_div2 = 2;
    set_sys_clock_pll(vco_freq, post_div1, post_div2);
    

    // MDA or EGA2
    ttlIn_Init_Vminus(ttlmode);


    while ( !(_PIO->irq & (1u<<6)) ) ; // irq6 == vsync_in
    sleep_ms(2);
    initVGA();

    sleep_ms(100);

    while (1)
    {
        if (vm_fail_cnt > 5) {
            // RESET Vfreq
            t0 = -1;
            t1 = 0;
            dt = 0;
            cnt = 0;
            vm_fail_cnt = 0;
        }

        pal = (ttlmode->pal & 0x1);

        #if 0
        ttlmode->vmode == CGAEGA ? (pal ? scan2cga_in() : scan2ega_in()) : scan_in(pal);  // 50-60 Hz
        #else
        ttlmode->vmode == CGAEGA ? scan2_in(pal) : scan_in(pal);  // 50-60 Hz
        #endif

        cnt += 1;
        if (cnt == 10) {  // 5-6 Hz

            // VSYNC freq timer
            //
            t1 = time_us_32();

            if (t1 > t0) dt = t1-t0;
            else dt = 0;
            // V=50Hz: 1e6 * 10/50 = 200000
            // V=60Hz: 1e6 * 10/60 = 166666
            // > 10Hz after OSD:
            if (dt > 190000 && dt < 227272) hpix = LINE_MDA; // 44Hz..52Hz (+1 cnt)
            else if (dt < 1000000 )         hpix = LINE_EGA; // 54Hz..60Hz (+1 cnt)

            // read VSYNC polarity
            //
            avgbuff[n] = gpio_get(VSYNC_IN);
            n = (n > 4) ? 0 : n+1;
            // lowpass VSYNC
            avg = avgbuff[0]+avgbuff[1]+avgbuff[2]+avgbuff[3]+avgbuff[4];
            pol_VSYNC = (avg < 3); //!(avg > 2);

            // pol_VSYNC == 0 (-) , hpix == LINE_MDA (V=50Hz)  -->  VID MODE = MDA
            // pol_VSYNC == 0 (-) , hpix == LINE_EGA (V=60Hz)  -->  VID MODE = EGA2
            // pol_VSYNC == 1 (+)                              -->  VID MODE = CGA/EGA
            if (dt < 227272) {
                vmode = (pol_VSYNC > 0) ? CGAEGA : (hpix==LINE_MDA ? MDA : EGA2);
            }
            else {
                vm_fail_cnt += 1;
            }


            #if TOGGLEPAL
            if (gpio_get(BUTTON_PAL) == 0) {
                toggle_hscan = toggle_pal();
            }
            #endif

            #if ADJ_CLK
            if (gpio_get(BUTTON_PLS) == 0) {
                switch (menu_select)
                {
                    case MENU_CLK : inc_clk(); adj_clk = 1; break;
                    case MENU_hOFS: inc_hofs(); break;
                    //case MENU_PAL : toggle_pal(); break;
                }
            }
            if (gpio_get(BUTTON_MIN) == 0) {
                switch (menu_select)
                {
                    case MENU_CLK : dec_clk(); adj_clk = 1; break;
                    case MENU_hOFS: dec_hofs(); break;
                }
            }
            #endif


            if (vmode != vmode0 || adj_clk|| toggle_hscan) {
                //float fq = 1e7/dt;

                adj_clk = 0;
                toggle_hscan = 0;
                vm_fail_cnt = 0;

                if (vmode != vmode0)
                {
                    switch (vmode)
                    {
                        case CGAEGA:
                                    ttlmode = &mode_CGAEGA;
                                    break;
                        case EGA2:  // MDA720
                                    ttlmode = &mode_EGA2;
                                    break;
                        case MDA:   // if MDA640, this is 14MHz, but same EGA2 polarity
                                    ttlmode = &mode_MDA;
                                    break;
                    }
                }

                reset_TTLin(_PIO);
                sleep_ms(100);

                pol_VSYNC > 0 ? ttlIn_Init_Vplus(ttlmode)
                              : ttlIn_Init_Vminus(ttlmode);

                memset(vga_data_array, 0, FRMBUFSZ);
                // pixel clock 14MHz:
                //      H:15kHz,V=60Hz -> 640 pixel/line EGA/CGA
                // pixel clock 16MHz:
                //      H:18kHz,V=50Hz -> 720 pixel/line MDA
                //      H:22kHz,V=60Hz -> 640 pixel/line EGA_Mode2

                #if DBG_USB
                printf("MODE:%d,%d DIVFRAC:%d\n", vmode, vmode0, ttlmode->div_frac);
                #endif
            }

            vmode0 = vmode;

            t0 = t1;
            cnt = 0;

            #if ADJ_CLK
            if (gpio_get(BUTTON_OSD) == 0) {
                sprintf(txtbuf, "Mode : %s", modestr[vmode]);
                wrtxt(menu_posX, 20, txtbuf, 0x3F);
                sprintf(txtbuf,  "hpix = %3d", hpix);
                wrtxt(menu_posX, 30, txtbuf, 0x3F);
                if (dt > 0) {
                    float fq = 1e7/dt;
                    sprintf(txtbuf, "V = %+.2fHz", pol_VSYNC>0 ? fq : -fq);
                    wrtxt(menu_posX, 40, txtbuf, 0x3F);
                }

                sprintf(txtbuf, "div  = (%d,%d)", ttlmode->div_int, ttlmode->div_frac);
                wrtxt(menu_posX, 50, txtbuf, 0x3F);
                // CLKkHZ/1e3 / PCLK = 114.0/4=28.5
                float pxclk = (114.0f/4.0f)/(ttlmode->div_int + ttlmode->div_frac/256.0f);
                sprintf(txtbuf, "pix_clk = %.3fMHz", pxclk);
                wrtxt(menu_posX, menu_pos[MENU_CLK], txtbuf, 0x3F);
                sprintf(txtbuf, "hoffset = %d", ttlmode->h_offset);
                wrtxt(menu_posX, menu_pos[MENU_hOFS], txtbuf, 0x3F);
                sprintf(txtbuf, "pal     = %d", ttlmode->pal);
                wrtxt(menu_posX, menu_pos[MENU_PAL], txtbuf, 0x3F);

                for (int j = 0; j < 3; j++) {
                    sprintf(txtbuf, "%s", j == menu_select ? "[*]" : "[ ]");
                    wrtxt(30, menu_pos[j], txtbuf, 0x3F);
                }

                sprintf(txtbuf, "Mode set: %s", modestr[ttlmode->vmode]);
                wrtxt(menu_posX, 100, txtbuf, 0x3F);
                sprintf(txtbuf, "hscan   = %d", ttlmode->xscanlrd);
                wrtxt(menu_posX, 110, txtbuf, 0x3F);

                sleep_ms(200);

                while (gpio_get(BUTTON_OSD) != 0) {

                    if (gpio_get(BUTTON_PAL) == 0) {
                        wrtxt(30, menu_pos[menu_select], "[ ]", 0x3F);
                        menu_select = (menu_select+1) % 3;
                        wrtxt(30, menu_pos[menu_select], "[*]", 0x3F);
                    }

                    if (gpio_get(BUTTON_PLS) == 0) {
                        switch (menu_select)
                        {
                            case MENU_CLK : inc_clk();    break;
                            case MENU_hOFS: inc_hofs();   break;
                            case MENU_PAL : toggle_pal(); break;
                        }
                    }
                    if (gpio_get(BUTTON_MIN) == 0) {
                        switch (menu_select)
                        {
                            case MENU_CLK : dec_clk();  break;
                            case MENU_hOFS: dec_hofs(); break;
                        }
                    }

                    sprintf(txtbuf, "div  = (%d,%d)  ", ttlmode->div_int, ttlmode->div_frac);
                    wrtxt(menu_posX, 50, txtbuf, 0x3F);
                    // CLKkHZ/1e3 / PCLK = 114.0/4=28.5
                    float pxclk = (114.0f/4.0f)/(ttlmode->div_int + ttlmode->div_frac/256.0f);
                    sprintf(txtbuf, "pix_clk = %.3fMHz", pxclk);
                    wrtxt(menu_posX, menu_pos[MENU_CLK], txtbuf, 0x3F);
                    sprintf(txtbuf, "hoffset = %d   ", ttlmode->h_offset);
                    wrtxt(menu_posX, menu_pos[MENU_hOFS], txtbuf, 0x3F);
                    sprintf(txtbuf, "pal     = %d", ttlmode->pal);
                    wrtxt(menu_posX, menu_pos[MENU_PAL], txtbuf, 0x3F);
                    sprintf(txtbuf, "hscan   = %d", ttlmode->xscanlrd);
                    wrtxt(menu_posX, 110, txtbuf, 0x3F);

                    sleep_ms(200);
                }

                reset_TTLin(_PIO);

                pol_VSYNC > 0 ? ttlIn_Init_Vplus(ttlmode)
                              : ttlIn_Init_Vminus(ttlmode);

                sleep_ms(100);

                //
                memset(vga_data_array, 0, FRMBUFSZ);
            }
            #endif

            #if DBG_USB
            // enable usb output (only if frame_buffer 720x350)
            // e.g. /dev/ttyACM0
            // 8N1 115200 baud
            if (dt > 0) {
                float fq = 1e7/dt;
                //printf("%d %6d: V=%+.2fHz  #  PAL: %d\n", pol_VSYNC, dt, pol_VSYNC>0 ? fq : -fq, pal);
                printf("%d %6d: V=%+.2fHz  #  PAL: %d", pol_VSYNC, dt, pol_VSYNC>0 ? fq : -fq, pal);
                //printf("  #  adj:%+3d", pol_VSYNC > 0 ? clkadj_14M : clkadj_16M);
                printf("  #  divfrac:%3d", divfrac);
                printf("\n");
            }
            #endif
        }

    }

    return 0;
}

