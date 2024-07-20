
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

#ifdef NO_COMMON
uint32_t prelines, ylinesrd, xscanlrd;
#endif

void scan_in(int pal) {

    while ( !(_PIO->irq & (1<<6)) ) _PIO->rxf[_SM];  // sync with vsync_in, empty FIFO

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < prelines; y++) {
        for (uint x = 0; x < xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            _PIO->rxf[_SM];
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < ylinesrd; y++) {
        for (uint x = 0; x < xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            uint32_t col4 = _PIO->rxf[_SM] & 0x3F3F3F3F;
            if (pal == PAL_MDA) col4 = (col4 & MASK_GREEN2INT) | ((col4 & MASK_BLUE2MONO)>>1);
            //(a) VGALNBF==VGALINE: if (x == VGALINE-4) col4 &= 0x1F3F3F3F; // MDA pixel 720, bit 6
            //(b) VGALNBF==VGALINE+4:
            *((uint32_t*)(vga_data_array+(VGALNBF*y + x))) = col4;  // 4 byte aligned ?
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    #endif
}

void scan2_in(/*int pal*/) {

    while ( !(_PIO->irq & (1<<6)) ) _PIO->rxf[_SM];  // sync with vsync_in, empty FIFO

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < prelines; y++) {
        for (uint x = 0; x < xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            _PIO->rxf[_SM];
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    uint n = 0;
    for (uint y = 0; y < ylinesrd; y++, n++) {  //n = 0 .. 3*ylinesrd/2
        for (uint x = 0; x < xscanlrd; x+=4) {
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
            memcpy(vga_data_array+VGALNBF*(n+1), vga_data_array+VGALNBF*n, xscanlrd);
            n++;
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    #endif
}

void scan2cga_in(/*int pal*/) {

    while ( !(_PIO->irq & (1<<6)) ) _PIO->rxf[_SM];  // sync with vsync_in, empty FIFO

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    for (uint y = 0; y < prelines; y++) {
        for (uint x = 0; x < xscanlrd; x+=4) {
            while (pio_sm_is_rx_fifo_empty(_PIO, _SM)) tight_loop_contents();
            _PIO->rxf[_SM];
        }
    }

    #if DBG_PIN
    gpio_put(TP_FRMBUF_IN, 0);
    gpio_put(TP_FRMBUF_IN, 1);
    #endif

    uint n = 0;
    for (uint y = 0; y < ylinesrd; y++, n++) {  //n = 0 .. 3*ylinesrd/2
        for (uint x = 0; x < xscanlrd; x+=4) {
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
            memcpy(vga_data_array+VGALNBF*(n+1), vga_data_array+VGALNBF*n, xscanlrd);
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


int main() {

    //uint64_t t0 = -1 //time_us_64()
    uint32_t t0 = -1; //time_us_32()
    uint32_t t1 = 0;
    uint32_t dt = 0;

    uint hpix = LINE_EGA;
    int pal = PAL_EGA;
    int vmode = EGA2,
        vmode0 = vmode;

    int clkadj_14M = 0,
        clkadj014M = clkadj_14M;
    int clkadj_16M = 0,
        clkadj016M = clkadj_16M;
    int clkadj_MDA = 0,
        clkadj0MDA = clkadj_MDA;
    int divfrac = DIV16M_FRAC,
        divfrac0 = divfrac;

    uint32_t avg = 0;
    uint32_t n = 0;
    uint8_t avgbuff[5] = {0};
    uint8_t pol_VSYNC = 0;  // 1:V+ , 0:V-
    uint8_t cnt = 0;

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
    

    ttlIn_Init_16MHz(DIV16M_INT, DIV16M_FRAC, LINE_EGA);


    while ( !(_PIO->irq & (1<<6)) ) ; // irq6 == vsync_in
    sleep_ms(2);
    initVGA();


    sleep_ms(100);

    while (1)
    {
        vmode == CGAEGA ? (pal == PAL_CGA ? scan2cga_in() : scan2_in()) : scan_in(pal);  // 50-60 Hz

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
            vmode = (pol_VSYNC > 0) ? CGAEGA : (hpix==LINE_MDA ? MDA : EGA2);

            #if TOGGLEPAL
            if (gpio_get(BUTTON_PAL) == 0) {
                if (vmode == CGAEGA) pal = (pal != PAL_CGA) ? PAL_CGA : PAL_EGA;
                if (vmode == MDA   ) pal = (pal != PAL_MDA) ? PAL_MDA : PAL_EGA;
            }
            if (pal == PAL_MDA && vmode != MDA   ) pal = PAL_EGA; // LINE_MDA: vmode==MDA
            if (pal == PAL_CGA && vmode != CGAEGA) pal = PAL_EGA; // pol_VSYNC==0: vmode==EGA2,MDA
            #endif

            #if ADJ_CLK
            if (gpio_get(BUTTON_PLS) == 0) {
                if (vmode == CGAEGA && DIV14M_FRAC+clkadj_14M < 255) clkadj_14M += 1;
                if (vmode == EGA2   && DIV16M_FRAC+clkadj_16M < 255) clkadj_16M += 1;
                if (vmode == MDA    && DIVMDA_FRAC+clkadj_MDA < 255) clkadj_MDA += 1;
            }
            if (gpio_get(BUTTON_MIN) == 0) {
                if (vmode == CGAEGA && DIV14M_FRAC+clkadj_14M > 0) clkadj_14M -= 1;
                if (vmode == EGA2   && DIV16M_FRAC+clkadj_16M > 0) clkadj_16M -= 1;
                if (vmode == MDA    && DIVMDA_FRAC+clkadj_MDA > 0) clkadj_MDA -= 1;
            }
            #endif

            if      (vmode == CGAEGA) divfrac = DIV14M_FRAC+clkadj_14M;
            else if (vmode == EGA2  ) divfrac = DIV16M_FRAC+clkadj_16M;
            else                      divfrac = DIVMDA_FRAC+clkadj_MDA;

            if (vmode != vmode0 || divfrac != divfrac0) {
                //float fq = 1e7/dt;

                reset_TTLin(_PIO);

                // pol_VSYNC > 0 ? ttlIn_Init_14MHz(DIV14M_INT, divfrac)
                //               : ttlIn_Init_16MHz(DIV16M_INT, divfrac, vmode);
                switch (vmode)
                {
                    case CGAEGA:
                                ttlIn_Init_14MHz(DIV14M_INT, divfrac);
                                break;
                    case EGA2:  // MDA720
                                ttlIn_Init_16MHz(DIV16M_INT, divfrac, vmode);
                                break;
                    case MDA:   // if MDA640, this is 14MHz, but same EGA2 polarity
                                ttlIn_Init_16MHz(DIVMDA_INT, divfrac, vmode);
                                break;
                }

                memset(vga_data_array, 0, FRMBUFSZ);
                // pixel clock 14MHz:
                //      H:15kHz,V=60Hz -> 640 pixel/line EGA/CGA
                // pixel clock 16MHz:
                //      H:18kHz,V=50Hz -> 720 pixel/line MDA
                //      H:22kHz,V=60Hz -> 640 pixel/line EGA_Mode2

                #if DBG_USB
                printf("MODE:%d,%d DIVFRAC:%d,%d\n", vmode, vmode0, divfrac, divfrac0);
                #endif
            }

            vmode0 = vmode;
            clkadj014M = clkadj_14M;
            clkadj016M = clkadj_16M;
            clkadj0MDA = clkadj_MDA;
            divfrac0 = divfrac;

            t0 = t1;
            cnt = 0;

            #if ADJ_CLK
            if (gpio_get(BUTTON_OSD) == 0) {
                sprintf(txtbuf, "Mode: %s", modestr[vmode]);
                wrtxt(20, 20, txtbuf, 0x3F);
                sprintf(txtbuf,  "hpix = %3d", hpix);
                wrtxt(20, 30, txtbuf, 0x3F);
                sprintf(txtbuf, "divfrac = %d", divfrac);
                wrtxt(20, 40, txtbuf, 0x3F);
                if (dt > 0) {
                    float fq = 1e7/dt;
                    sprintf(txtbuf, "V = %+.2fHz", pol_VSYNC>0 ? fq : -fq);
                    wrtxt(20, 50, txtbuf, 0x3F);
                }
                sleep_ms(4000);
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

