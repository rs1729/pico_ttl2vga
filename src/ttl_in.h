#ifndef TTL_IN_H
#define TTL_IN_H

/*
    Voltage Divider TTL 5V -> 5V*1k/1.499k=3.3V
    DB9 --> 499 Ohm --> 1k Ohm --> GND
     TTL RED2       -->  GPIO  8
     TTL RED1       -->  GPIO  9
     TTL GREEN2/I   -->  GPIO 10
     TTL GREEN1     -->  GPIO 11
     TTL BLUE2/MONO -->  GPIO 12
     TTL BLUE1      -->  GPIO 13
     TTL HSYNC      -->  GPIO 14
     TTL VSYNC      -->  GPIO 15  (optional 1nF Cap --> GND)
     TTL GND        -->  GND
    GPIO 16 --> MDA/CGA/EGA HSync
    GPIO 17 --> MDA/CGA/EGA VSync
*/

/*
    MDA           720x350   H+:18.4kHz  V-:50Hz  px_clk:16.257MHz   vsync: 16 lines    total: 369 lines
    CGA           640x200   H+:15.7kHz  V+:60Hz  px_clk:14.318MHz   vsync:  3 lines    total: 262 lines
    EGA (Mode 1)  640x200   H+:15.7kHz  V+:60Hz  px_clk:14.318MHz   vsync:  3 lines    total: 262 lines
    EGA (Mode 2)  640x350   H+:21.8kHz  V-:60Hz  px_clk:16.257MHz   vsync: 13 lines    total: 364 lines
*/

/*
    720:640 = 9:8
    MDA640 = MDA*8/9 = 640 ,  px_clk: 16.257MHz*8/9 = 14.45MHz
    if VGA monitor displays only 640 pix in 720x400 mode, scan at 8/9 speed:
    -> toggle PAL or set mode_MDA.xscanlrd=640
*/


#define LINE_MDA  720
#define LINE_CGA  640
#define LINE_EGA  640

// #define YLNS_MDA  351  // 350..351 <= VGA_YACTIVE , DBG_USB: 350
// #define YLNS_CGA  208  // 200 + 8  // addition buffer, CGA/EGA FP/BP variations
// #define YLNS_EGA  YLNS_CGA
// #define YLNS_EGA2 YLNS_MDA

// //VSYNC BP before pix
// #define PRELINES_MDA 1
// #define PRELINES_16M 1   // OAK:2  ET3000:1  (350+2 YLNS)
// #define PRELINES_14M 30  // 33..35 , EGA: 33 , CGA: 35 , ET3000: CGA=32, EGA=33


typedef struct {
    int vmode;
    uint16_t hline_px;
    uint16_t vline_px;
    uint16_t div_int;
    int16_t  div_frac;
    int16_t  h_offset;
    uint16_t prelines;
    uint16_t xscanlrd;
    uint16_t ylinesrd;
    int pal;
} ttlmode_t;

/*
// DIV = DIV_INT + DIV_FRAC/256
//V-
// DIV:(1,207)  // (1,207..232..240)
// OAK:EGA2=(1,207), MDA=(1,232) ((1,207) GREEN/BLUE)
// ET3000:(1,222-223..240)
//V+
// DIV:(2,77)   // (2,51..122)
// OAK:(2,51..55..77) , (2,3)=2+3/256 (ofs=450) -> 14.167MHz no jitter, line to long ?
// ET3000:(2,122)
*/

//enum ega_pins {HSYNC_IN=8, VSYNC_IN, RED2_IN, RED1_IN, GREEN2_IN, GREEN1_IN, BLUE2_IN, BLUE1_IN, GND1, GND2};
// read 8 pins into 8bits=1byte
enum ega_pins {RED2_IN=8, RED1_IN, GREEN2_IN, GREEN1_IN, BLUE2_IN, BLUE1_IN, HSYNC_IN, VSYNC_IN};
// MDA->EGA: MONO -> BLUE2 (pin7) , INTENSITY -> GREEN2 (pin6)

enum col_pals {PAL_EGA=0, PAL_CGA, PAL_MDA};
//enum vidmodes {UNKNOWN=0, MDA, CGA, EGA, EGA2};
enum vidmodes {UNKNOWN=0, MDA, CGAEGA, EGA2};

static char *modestr[] = {
    [UNKNOWN] = "unknown",
    [MDA]     = "MDA",
    [CGAEGA]  = "CGA/EGA",
    [EGA2]    = "EGA2"
};

static const int16_t OFS_STEP = 1;  //8

static ttlmode_t mode_MDA = {
    .vmode    = MDA,
    .hline_px = 720,
    .div_int  = 2,
    .div_frac = 4,    //720px: (2,4) -> 14.140MHz , 640px: (2,68) -> 12.579MHz
    .h_offset = 144,
    .prelines = 1,
    .xscanlrd = 720,  // default:720 , compressed:640
    .ylinesrd = 351,
    .pal      = 0
};

static ttlmode_t mode_CGAEGA = {
    .vmode    = CGAEGA,
    .hline_px = 640,
    .div_int  = 2,
    .div_frac = 3,  // 77   // OAK: (2,3)=2+3/256 (ofs=450) -> 14.167MHz no jitter
    .h_offset = 450,
    .prelines = 30,
    .xscanlrd = 640,
    .ylinesrd = 208,
    .pal      = 0
};

static ttlmode_t mode_EGA2 = {
    .vmode    = EGA2,
    .hline_px = 640,
    .div_int  = 1,
    .div_frac = 207,
    .h_offset = 152,
    .prelines = 2,
    .xscanlrd = 640,
    .ylinesrd = 351,
    .pal      = 0
};

/*
static ttlmode_t mode_MDA640 = {
    .vmode    = MDA,
    .hline_px = 720,
    .div_int  = 2,
    .div_frac = 77,
    .h_offset = 0,
    .prelines = 1,
    .xscanlrd = 640,
    .ylinesrd = 351
};

static ttlmode_t _mode_CGA = {
    .vmode    = CGAEGA,
    .hline_px = 640,
    .div_int  = 2,
    .div_frac = 77,
    .h_offset = 0,
    .prelines = 30,
    .xscanlrd = 640,
    .ylinesrd = 208
};
static ttlmode_t _mode_EGA = {
    .vmode    = CGAEGA,
    .hline_px = 640,
    .div_int  = 2,
    .div_frac = 77,
    .h_offset = 0,
    .prelines = 30,
    .xscanlrd = 640,
    .ylinesrd = 208
};
*/


void ttlIn_Init_Vminus(ttlmode_t*);
void ttlIn_Init_Vplus(ttlmode_t*);

#endif // TTL_IN_H

