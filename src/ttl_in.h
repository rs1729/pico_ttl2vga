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

#define LINE_MDA  720
#define LINE_CGA  640
#define LINE_EGA  640

#define YLNS_MDA  351  // 350..351 <= VGA_YACTIVE , DBG_USB: 350
#define YLNS_CGA  208  // 200 + 8  // addition buffer, CGA/EGA FP/BP variations
#define YLNS_EGA  YLNS_CGA
#define YLNS_EGA2 YLNS_MDA

// VSYNC BP before pix
#define PRELINES_MDA 1
#define PRELINES_16M 1   // OAK:2  ET3000:1  (350+2 YLNS)
#define PRELINES_14M 30  // 33..35 , EGA: 33 , CGA: 35 , ET3000: CGA=32, EGA=33


// DIV = DIV_INT + DIV_FRAC/256
//V-
#define DIV16M_INT   1
#define DIV16M_FRAC  207  // 207..232..240   // OAK:207(EGA2),232(MDA)(207:GREEN/BLUE)  ET4000:222-223
#define DIVMDA_FRAC  232  // 207..232..240   // OAK:207(EGA2),232(MDA)(207:GREEN/BLUE)  ET4000:240
//V+
#define DIV14M_INT   2
#define DIV14M_FRAC  77   // 51..122         // OAK:51..55..77  ET4000:122
//H-CLK=2: OAK=72


//enum ega_pins {HSYNC_IN=8, VSYNC_IN, RED2_IN, RED1_IN, GREEN2_IN, GREEN1_IN, BLUE2_IN, BLUE1_IN, GND1, GND2};
// read 8 pins into 8bits=1byte
enum ega_pins {RED2_IN=8, RED1_IN, GREEN2_IN, GREEN1_IN, BLUE2_IN, BLUE1_IN, HSYNC_IN, VSYNC_IN};
// MDA->EGA: MONO -> BLUE2 (pin7) , INTENSITY -> GREEN2 (pin6)

enum col_pals {PAL_EGA=0, PAL_CGA, PAL_MDA};
//enum vidmodes {UNKNOWN=0, MDA, CGA, EGA, EGA2};
enum vidmodes {UNKNOWN=0, MDA, CGAEGA, EGA2};


// init in ttlIn_Init()
uint32_t prelines;
uint32_t ylinesrd;
uint32_t xscanlrd;

void ttlIn_Init_16MHz(uint16_t, uint8_t, int);
void ttlIn_Init_14MHz(uint16_t, uint8_t);

#endif // TTL_IN_H

