/*
    GPIO 0 --> VGA Hsync
    GPIO 1 --> VGA Vsync
    GPIO 2 --> 1k  Ohm R --> VGA RED2 (LSB)
    GPIO 3 --> 499 Ohm R --> VGA RED1 (MSB)
    GPIO 4 --> 1k  Ohm R --> VGA GREEN2 (LSB)
    GPIO 5 --> 499 Ohm R --> VGA GREEN1 (MSB)
    GPIO 6 --> 1k  Ohm R --> VGA BLUE2 (LSB)
    GPIO 7 --> 499 Ohm R --> VGA BLUE1 (MSB)
       GND --> VGA GND
*/

// horizontal fp/sync/bp multiple of 8 ?
// 114/4 : 720,16,108,56 x 400,12,2,35 : H=-31.4kHz,V=+70Hz (VGA)
// 114/4 : 720,16,108,56 x 350,37,2,60 : H=+31.4kHz,V=-70Hz (EGA)

#define XACTIVE 720
#define X_FP     16
#define X_SYNC  108
#define X_BP     56

#define YACTIVE 351 // 350..351 (351: disable pico_enable_stdio_usb() , DBG_USB: 350)
#define Y_FP     37 //-25=12
#define Y_SYNC    2
#define Y_BP     60 //-25=35
//400 -> 350 visible

// VGA timing constants
#define H_ACTIVE   (XACTIVE+X_FP-2 -1)  // one cycle delay for mov, one for set
#define V_ACTIVE   (YACTIVE -1)
#define RGB_ACTIVE (XACTIVE -1)         // 1 pix/byte

#define VGALINE (XACTIVE)


#define CLKkHZ 114000  // default: 125000  // 114/4=28.5
#define PCLK 4


// Length of the pixel array, and number of DMA transfers
// 640x400=256000
// 720x350=252000
#define FRMBUFSZ (VGALINE*YACTIVE)

unsigned char vga_data_array[FRMBUFSZ+4];


// LSB,MSB
enum vga_pins {HSYNC=0, VSYNC, RED_PIN2, RED_PIN1, GREEN_PIN2, GREEN_PIN1, BLUE_PIN2, BLUE_PIN1};


void initVGA(void) ;

