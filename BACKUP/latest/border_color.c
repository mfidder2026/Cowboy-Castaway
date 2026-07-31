#include <conio.h>
#include <c64.h>

/* C64 color constants */
#define COLOR_BLACK     0x00
#define COLOR_WHITE     0x01
#define COLOR_RED       0x02
#define COLOR_CYAN      0x03
#define COLOR_PURPLE    0x04
#define COLOR_GREEN     0x05
#define COLOR_BLUE      0x06
#define COLOR_YELLOW    0x07
#define COLOR_ORANGE    0x08
#define COLOR_BROWN     0x09
#define COLOR_LIGHTRED  0x0A
#define COLOR_GRAY1     0x0B
#define COLOR_GRAY2     0x0C
#define COLOR_LIGHTGREEN 0x0D
#define COLOR_LIGHTBLUE 0x0E
#define COLOR_GRAY3     0x0F

/* C64 original default colors */
#define C64_DEFAULT_BORDER  COLOR_LIGHTBLUE   /* Light blue border */
#define C64_DEFAULT_BG      COLOR_BLUE        /* Blue background */
#define C64_DEFAULT_TXT     COLOR_LIGHTBLUE   /* Light blue text */

/* Color palette - cycling through various colors */
static const unsigned char colors[] = {
    COLOR_RED, COLOR_CYAN, COLOR_GREEN, COLOR_BLUE,
    COLOR_YELLOW, COLOR_ORANGE, COLOR_PURPLE, COLOR_LIGHTGREEN,
    COLOR_LIGHTBLUE, COLOR_WHITE
};

static const unsigned char num_colors = sizeof(colors) / sizeof(colors[0]);

/* CIA1 Timer A registers */
#define CIA1_TA_LO      0xDC04
#define CIA1_TA_HI      0xDC05

/* Exact 63 bytes sprite data for the multi-color heart */
const unsigned char sprite_data[63] = {
    0,80,80,   5,245,245, 22,255,253,
    90,255,255, 91,255,255, 91,255,255,
    23,255,253, 23,255,253, 5,255,244,
    5,255,244,  1,127,208,  0,95,64,
    0,21,0,     0,0,0,      0,0,0,
    0,0,0,      0,0,0,      0,0,0,
    0,0,0,      0,0,0,      0,0,0
};

/* Wait approximately 'ms' milliseconds using CIA timer */
static void delay_ms(unsigned int ms) {
    unsigned int start;
    unsigned int now;
    unsigned int elapsed;
    
    start = *(unsigned int*)CIA1_TA_LO;
    
    do {
        now = *(unsigned int*)CIA1_TA_LO;
        if (now >= start) {
            elapsed = now - start;
        } else {
            elapsed = 65536U - start + now;
        }
    } while (elapsed < ms);
}

/* Initialize heart sprite - called once at startup */
static void init_heart_sprite(void) {
    unsigned char i;
    unsigned char* sprite_dest;
    
    /* Set sprite pointer to block 13 FIRST (like BASIC line 80) */
    /* Sprite pointer is at address $07F8 = 2040 */
    *(unsigned char*)2040 = 13;
    
    /* NOW copy sprite data to address 832 (like BASIC lines 100-120) */
    sprite_dest = (unsigned char*)832;
    for (i = 0; i < 63; i++) {
        sprite_dest[i] = sprite_data[i];
    }
}

/* Show the red heart sprite at given position */
static void show_heart(unsigned char x, unsigned char y) {
    /* Set sprite position */
    VIC.spr_pos[0].x = x;
    VIC.spr_pos[0].y = y;
}

/* Hide sprite */
static void hide_heart(void) {
    VIC.spr_ena = 0;
}

void main(void) {
    unsigned char idx = 0;
    unsigned char key;
    unsigned char i;
    unsigned int x;
    unsigned int t;
    
    /* Initialize screen with C64 default colors */
    bordercolor(C64_DEFAULT_BORDER);
    bgcolor(C64_DEFAULT_BG);
    textcolor(C64_DEFAULT_TXT);
    clrscr();
    
    /* Display header */
    gotoxy(0, 0);
    cputs("================================");
    gotoxy(0, 1);
    cputs("   BORDER COLOR CHANGER");
    gotoxy(0, 2);
    cputs("================================");
    gotoxy(0, 4);
    cputs("Press SPACE to exit");
    
    /* Initialize heart sprite ONCE at startup */
    init_heart_sprite();
    
    /* Set up heart sprite colors (matching BASIC example exactly) */
    VIC.bordercolor = 0;           /* Black border - line 20 */
    VIC.bgcolor0 = 0;              /* Black background - line 20 */
    VIC.spr_mcolor0 = 11;          /* Dark gray - line 30 */
    VIC.spr_mcolor1 = 15;          /* Light gray - line 40 */
    VIC.spr_color[0] = 2;          /* Red heart - line 50 */
    
    /* Enable multicolor mode for sprite 0 (line 70) */
    VIC.spr_mcolor = 1;
    
    /* Main loop - change colors every 3 seconds */
    while (1) {
        /* Set new border and background colors */
        bordercolor(colors[idx]);
        bgcolor((colors[idx] + 5) % 16);
        
        /* Wait 3 seconds (3000ms), checking for SPACE every 100ms */
        for (i = 0; i < 30; i++) {
            delay_ms(100);
            
            /* Check if SPACE was pressed */
            if (kbhit()) {
                key = cgetc();
                if (key == 32) {  /* SPACE is ASCII 32 */
                    /* Restore original C64 colors */
                    bordercolor(C64_DEFAULT_BORDER);
                    bgcolor(C64_DEFAULT_BG);
                    textcolor(C64_DEFAULT_TXT);
                    clrscr();
                    
                    /* Set black background for heart animation */
                    VIC.bordercolor = 0;
                    VIC.bgcolor0 = 0;
                    
                    /* Enable sprite 0 (line 140) */
                    VIC.spr_ena = 1;
                    
                    /* Animated heart moving from left to right (lines 150-190) */
                    for (x = 24; x < 250; x++) {
                        VIC.spr_pos[0].x = x;
                        VIC.spr_pos[0].y = 120;
                        
                        /* Delay loop (line 180) */
                        for (t = 0; t < 20; t++) {
                            /* Empty delay loop */
                        }
                    }
                    
                    /* Hide heart and show message */
                    VIC.spr_ena = 0;
                    
                    /* Restore colors for message */
                    bordercolor(C64_DEFAULT_BORDER);
                    bgcolor(C64_DEFAULT_BG);
                    textcolor(C64_DEFAULT_TXT);
                    
                    gotoxy(8, 12);
                    cputs("Thank you!");
                    
                    /* Wait for any key to exit */
                    cgetc();
                    
                    return;
                }
            }
        }
        
        /* Next color index */
        idx++;
        if (idx >= num_colors) {
            idx = 0;
        }
    }
}
