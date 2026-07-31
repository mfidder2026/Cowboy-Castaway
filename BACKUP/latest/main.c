/*
 * main.c  -  Cowboy op een onbewoond eiland (CC65 / cc65)
 * ------------------------------------------------------
 * Rechterpijltje -> loopt naar rechts   (rechtse animatie)
 * Linkerpijltje  -> loopt naar links    (linkse/gespiegelde animatie)
 * Pijltje omlaag -> loopt naar beneden  (beneden-animatie, sprite5/6)
 * Pijltje omhoog -> loopt naar boven    (boven-animatie, sprite7/8)
 * Niets ingedrukt-> bevriest op het laatste frame. Na ~3 seconden gaat de
 *                   cowboy uit zichzelf rondwandelen, en af en toe voert
 *                   hij dan een animatie op (zie anims.c). Zodra je een
 *                   pijltje aanraakt neem jij het weer over.
 * F              -> maakt meteen een kampvuurtje.
 * S              -> laat een haaienvin door de zee zwemmen. Elke animatie
 *                   in anims.c kan zo zijn eigen sneltoets krijgen.
 *
 * Bij het opstarten verschijnt eerst het titelscherm; de SPATIEBALK of de
 * VUURKNOP (joystickpoort 1 of 2) laat het eiland eroverheen uitpakken en
 * het programma beginnen.
 *
 * Compileren:  cl65 -O -t c64 main.c cowboy_frames.c island_gfx.c intro_gfx.c anims.c -o cowboy.prg
 * Draaien:     x64sc cowboy.prg
 *
 * ACHTERGROND
 * -----------
 * Het eiland staat in multicolor bitmap-modus. De VIC kan alleen graphics
 * zien binnen een 16KB-bank, dus we zetten de VIC in BANK 1 ($4000-$7FFF):
 *
 *   bitmap        -> $6000   (bank-offset $2000)
 *   screen-RAM    -> $5C00   (bank-offset $1C00)  kleuren 01 en 10
 *   sprite-data   -> $5000   (blok 64 en verder)
 *   sprite-ptrs   -> $5FF8   (= screen-RAM + $3F8)
 *   kleur-RAM     -> $D800   (vast, bank-onafhankelijk)  kleur 11
 *
 * De graphics zitten expres bovenin de bank: het programma zelf mag daardoor
 * doorgroeien tot $5000 (ruim 18 KB) als jij animaties toevoegt. Bron en
 * bestemming overlappen nooit; bij het opstarten kopieren we alles omhoog.
 *
 * SPRITE-INDELING
 * ---------------
 *   sprite 0  cowboy   (multicolor)   blok 64,65,66
 *   sprite 1  meeuw    (hi-res wit)   blok 67,68
 *   sprite 2  meeuw    (hi-res wit)   blok 67,68
 *   sprite 3  effect A (per animatie) blok 69 en verder
 *   sprite 4  effect B (per animatie) blok 69 en verder
 *
 * BOTSING MET HET WATER
 * ---------------------
 * In island_gfx.c staat een masker: per beeldrij de min/max x waar zand of
 * branding ligt. Voor elke stap wordt eerst gekeken of het voetpunt van de
 * sprite (midden onderaan) daarbinnen blijft; zo niet, dan gaat de stap niet
 * door. De cowboy kan het eiland dus niet af lopen.
 */
#include <c64.h>
#include <peekpoke.h>
#include <string.h>
#include "anims.h"

extern const unsigned char cowboy_frames[4][3][64];
extern const unsigned char cowboy_idle[64];
extern const unsigned char gull_frames[2][64];

/* De twee schermen staan INGEPAKT in het programma (RLE); zie island_gfx.c
   voor het formaat. Onverpakt kost elk scherm 10000 bytes en zouden ze samen
   niet meer onder de graphics in VIC-bank 1 passen. */
extern const unsigned char island_bitmap_rle[];
extern const unsigned char island_screen_rle[];
extern const unsigned char island_color_rle[];
extern const unsigned char intro_bitmap_rle[];
extern const unsigned char intro_screen_rle[];
extern const unsigned char intro_color_rle[];
#ifndef ISLAND_BGCOLOR
#define ISLAND_BGCOLOR 6
#endif
#ifndef INTRO_BGCOLOR
#define INTRO_BGCOLOR 3
#endif
#ifndef INTRO_BORDERCOLOR
#define INTRO_BORDERCOLOR 6
#endif

/* Eiland-masker: per beeldrij de min/max x (in multicolor-pixels) waar land
   ligt. Zie island_gfx.c. */
extern const unsigned char sand_min_x[41];
extern const unsigned char sand_max_x[41];
#define SAND_TOP      131u          /* bovenste beeldrij met zand  */
#define SAND_BOT      171u          /* onderste beeldrij met zand  */

/* Zelf rondlopen als er een tijdje geen toets is ingedrukt. */
#define IDLE_TIMEOUT  150u          /* beelden (~3 sec bij 50 Hz)  */

/* Meeuwen: twee witte V-tjes die langzaam van links naar rechts overvliegen.
   De lucht loopt van beeldrij 0 t/m 95 (horizon), dus schermrij 45 en 62
   liggen er ruim boven. VIC-y = schermrij + 50. */
#define GULL1_Y       95u
#define GULL2_Y       112u
#define GULL_WRAP     350u          /* voorbij rechts -> weer links binnen */

/* --- geheugenindeling in VIC-bank 1 --- */
#define VIC_BANK      1u
#define BANK_BASE     0x4000u
#define SCREEN_RAM    (BANK_BASE + 0x1C00u)   /* $5C00 */
#define BITMAP_RAM    (BANK_BASE + 0x2000u)   /* $6000 */
#define SPRITE_RAM    (BANK_BASE + 0x1000u)   /* $5000 */
#define SPRITE_PTR    (SCREEN_RAM + 0x03F8u)  /* $5FF8 */
#define FIRST_BLOCK   64u                     /* $5000 / 64 = 64  (cowboy) */
#define GULL_BLOCK    67u                     /* $50C0 en $5100  (meeuwen) */
#define FX_BLOCK      69u                     /* $5140 en verder (effecten) */
#define COLRAM_ADDR   0xD800u

#define ANIM_SPEED    6u        /* beelden per loop-frame (lager = sneller) */
#define WALK_STEP     1u        /* pixels verplaatsing per beeld            */

/* JUMP-state machine: de echte cowboy loopt naar de palmboom in het midden
   van het eiland, klimt omhoog, springt in een boog het water in, zwemt
   terug en klimt weer op het eiland. */
#define JUMP_TARGET_X 180       /* palmboom zit iets rechts van het midden   */
#define JUMP_TARGET_Y 170       /* voetpunt bij de palmboom                   */
#define JUMP_TOP_X    180       /* zelfde horizontale positie als palmboom     */
#define JUMP_TOP_Y    140       /* top: iets meer dan 1x cowboyhoogte        */
#define JUMP_WATER_X   50       /* landingspunt in water links van eiland    */
#define JUMP_WATER_Y  225       /* waterlijn, Y blijft gelijk tijdens sprong */
/* Linkerrand van het wandelgrid: sand_min_x is in multicolor-pixels.
   VIC-sprite x = (multicolor_x * 2) + 12.  Kleinste waarde is 25 (rij 152),
   dus de rand ligt ongeveer op x = 25*2+12 = 62.  Daar moet de cowboy
   weer op het eiland komen. */
#define JUMP_SWIM_X    62       /* meest linkere punt van het wandelgrid     */
#define JUMP_SWIM_Y   220       /* waterlijn, net onder het smalle middelste gedeelte */

/* JUMP-toestanden */
#define JUMP_INACTIVE 0
#define JUMP_WALK     1
#define JUMP_TURN     2
#define JUMP_CLIMB    3
#define JUMP_LOOK     4
#define JUMP_LEAP     5
#define JUMP_SPLASH   6
#define JUMP_SWIM     7
#define JUMP_CLIMB_OUT 8
#define JUMP_DONE     9

/* Hoofd van de cowboy boven water: klein effect-sprite. */
#define SWIM_HEAD_BLOCK (FX_BLOCK + FX_SWIM_HEAD)
#define SPLASH0_BLOCK   (FX_BLOCK + FX_SPLASH0)
#define SPLASH1_BLOCK   (FX_BLOCK + FX_SPLASH1)
/* Hoe lang wacht hij tussen twee animaties? De klok gaat lopen zodra een
   animatie AFGELOPEN is, en de volgende start zodra hij op nul staat.
   50 beelden = 1 seconde (PAL), dus dit is 10 tot 20 seconden. */
#define ANIM_WAIT_MIN 500u      /* ondergrens: 10 seconden                  */
#define ANIM_WAIT_VAR 511u      /* daar komt 0..511 beelden bij (~10 sec)   */

/* Richtingen = index in cowboy_frames[]. */
#define DIR_RIGHT     0
#define DIR_LEFT      1
#define DIR_DOWN      2
#define DIR_UP        3
#define LOADED_IDLE   0xFF      /* geen loop-set maar de rustframe          */

/* Loopcyclus 0,1,2,1. Bij beneden/boven is frame2 == frame0, dus dit
   wordt vanzelf een nette 2-frame alternatie. */
static const unsigned char cycle[4] = { 0, 1, 2, 1 };

/* Zacht op-en-neer voor de meeuwen (offset in pixels). */
static const unsigned char bob[8] = { 0, 0, 1, 2, 2, 2, 1, 0 };

/* =====================================================================
 *  Lichtkrant-font (6x8, witte pixels in bits 7..2 van elke byte)
 * ===================================================================== */
static const unsigned char font6x8[40][8] = {
    /* A-Z */
    { 0x78, 0x84, 0x84, 0xFC, 0x84, 0x84, 0x84, 0x00 }, /* A */
    { 0xF8, 0x84, 0x84, 0xF8, 0x84, 0x84, 0xF8, 0x00 }, /* B */
    { 0x7C, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7C, 0x00 }, /* C */
    { 0xF0, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x00 }, /* D */
    { 0xFC, 0x80, 0x80, 0xF8, 0x80, 0x80, 0xFC, 0x00 }, /* E */
    { 0xFC, 0x80, 0x80, 0xF8, 0x80, 0x80, 0x80, 0x00 }, /* F */
    { 0x7C, 0x80, 0x80, 0x9C, 0x84, 0x84, 0x78, 0x00 }, /* G */
    { 0x84, 0x84, 0x84, 0xFC, 0x84, 0x84, 0x84, 0x00 }, /* H */
    { 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00 }, /* I */
    { 0x3C, 0x08, 0x08, 0x08, 0x08, 0x88, 0x70, 0x00 }, /* J */
    { 0x84, 0x88, 0x90, 0xE0, 0x90, 0x88, 0x84, 0x00 }, /* K */
    { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFC, 0x00 }, /* L */
    { 0x84, 0xCC, 0xB4, 0x84, 0x84, 0x84, 0x84, 0x00 }, /* M */
    { 0x84, 0xC4, 0xA4, 0x94, 0x8C, 0x84, 0x84, 0x00 }, /* N */
    { 0x78, 0x84, 0x84, 0x84, 0x84, 0x84, 0x78, 0x00 }, /* O */
    { 0xF8, 0x84, 0x84, 0xF8, 0x80, 0x80, 0x80, 0x00 }, /* P */
    { 0x78, 0x84, 0x84, 0x84, 0x94, 0x88, 0x68, 0x00 }, /* Q */
    { 0xF8, 0x84, 0x84, 0xF8, 0x90, 0x88, 0x84, 0x00 }, /* R */
    { 0x7C, 0x80, 0x80, 0x78, 0x04, 0x04, 0xF8, 0x00 }, /* S */
    { 0xFC, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00 }, /* T */
    { 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x78, 0x00 }, /* U */
    { 0x84, 0x84, 0x84, 0x84, 0x84, 0x48, 0x30, 0x00 }, /* V */
    { 0x84, 0x84, 0x84, 0x84, 0xB4, 0xCC, 0x84, 0x00 }, /* W */
    { 0x84, 0x84, 0x48, 0x30, 0x48, 0x84, 0x84, 0x00 }, /* X */
    { 0x84, 0x84, 0x48, 0x30, 0x30, 0x30, 0x30, 0x00 }, /* Y */
    { 0xFC, 0x04, 0x08, 0x10, 0x20, 0x40, 0xFC, 0x00 }, /* Z */
    /* 0-9 */
    { 0x78, 0x8C, 0x94, 0xA4, 0x94, 0x8C, 0x78, 0x00 }, /* 0 */
    { 0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0x7C, 0x00 }, /* 1 */
    { 0x78, 0x84, 0x04, 0x18, 0x20, 0x40, 0xFC, 0x00 }, /* 2 */
    { 0x78, 0x84, 0x04, 0x38, 0x04, 0x84, 0x78, 0x00 }, /* 3 */
    { 0x18, 0x30, 0x60, 0x90, 0xFC, 0x10, 0x10, 0x00 }, /* 4 */
    { 0xFC, 0x80, 0xF8, 0x04, 0x04, 0x84, 0x78, 0x00 }, /* 5 */
    { 0x78, 0x80, 0x80, 0xF8, 0x84, 0x84, 0x78, 0x00 }, /* 6 */
    { 0xFC, 0x04, 0x08, 0x10, 0x20, 0x20, 0x20, 0x00 }, /* 7 */
    { 0x78, 0x84, 0x84, 0x78, 0x84, 0x84, 0x78, 0x00 }, /* 8 */
    { 0x78, 0x84, 0x84, 0x7C, 0x04, 0x04, 0x78, 0x00 }, /* 9 */
    /* spatie, komma, punt, asterisk */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* spatie */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x20, 0x40 }, /* komma */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30 }, /* punt */
    { 0x00, 0x88, 0x48, 0xFC, 0x48, 0x88, 0x00, 0x00 }  /* asterisk */
};

static const char marquee_text[] =
    "Cowboy Castaway, a 2026 Commodore 64 Screensaver brought to you by the famous Retro8BITShop, press SPACE to continue * * * ";

static unsigned char marquee_pos     = 0;  /* karakterpositie in de tekst  */
static unsigned char marquee_pix_off = 0;  /* sub-pixel offset, 0..11      */
static unsigned char marquee_tick    = 0;  /* beeld-teller voor de snelheid */

static unsigned char char_to_font(char ch)
{
    if (ch >= 'A' && ch <= 'Z') return (unsigned char)(ch - 'A');
    if (ch >= '0' && ch <= '9') return (unsigned char)(ch - '0' + 26);
    if (ch == ' ')  return 36;
    if (ch == ',')  return 37;
    if (ch == '.')  return 38;
    if (ch == '*')  return 39;
    return 0xFF;
}

/* Tekent één scanlijn van de lichtkrant in de bitmap. Elk font-bit uit
 * font6x8 wordt verdubbeld tot een witte multicolor-pixel (bits 11). Een
 * letter is daardoor 6 hi-res = 12 multicolor pixels breed. */
static void render_marquee_line(unsigned char y)
{
    unsigned char byte_x, pair;
    unsigned char *bm;
    unsigned int text_len, virtual_x;
    unsigned char char_col, font_col, hr_bit, ch, idx, font_byte, pix_byte;

    text_len = (unsigned int)(sizeof(marquee_text) - 1);
    bm = (unsigned char *)(BITMAP_RAM + (192u + y) * 40u);

    for (byte_x = 0; byte_x < 40; ++byte_x)
    {
        pix_byte = 0;
        for (pair = 0; pair < 4; ++pair)
        {
            /* x = multicolor-pixelpositie op het scherm, 0..159 */
            virtual_x = marquee_pos * 12u
                      + (unsigned int)byte_x * 8u
                      + (unsigned int)pair * 2u
                      + (unsigned int)marquee_pix_off;

            char_col = (unsigned char)(virtual_x / 12u);
            font_col = (unsigned char)(virtual_x % 12u); /* 0..11 */
            hr_bit   = (unsigned char)(7 - (font_col / 2u));

            /* Alleen de bovenste 6 bits (7..2) van het font bevatten pixels. */
            if (hr_bit < 2)
                continue;

            ch = marquee_text[char_col % text_len];
            idx = char_to_font(ch);
            if (idx >= 40)
                continue;

            font_byte = font6x8[idx][y];
            if (font_byte & (1u << hr_bit))
            {
                /* Witte multicolor-pixel (bits 11) op deze pair-positie. */
                pix_byte |= (unsigned char)(0xC0u >> (pair * 2u));
            }
        }
        bm[byte_x] = pix_byte;
    }
}

/* Render de hele lichtkrant-rij opnieuw. */
static void render_marquee(void)
{
    unsigned char y;
    for (y = 0; y < 8; ++y)
        render_marquee_line(y);
}

/* Update de lichtkrant; roep elke beeld aan. De tekst schuift vloeiend
 * van rechts naar links, met sub-karakter precisie. */
static void marquee_update(void)
{
    unsigned char len = (unsigned char)(sizeof(marquee_text) - 1);

    if (++marquee_tick < 3) return;
    marquee_tick = 0;

    render_marquee();

    if (++marquee_pix_off >= 12)
    {
        marquee_pix_off = 0;
        marquee_pos = (unsigned char)((marquee_pos + 1) % len);
    }
}

/* Zet color-RAM van de onderste tekenrij op wit, zodat de lichtkrant-letters
 * zichtbaar zijn ongeacht de oorspronkelijke kleurinstelling van het scherm. */
static void setup_marquee_colors(void)
{
    memset((void *)(COLRAM_ADDR + 24u * 40u), COLOR_WHITE, 40);
}

/* --- toestand van de cowboy (file-scope, zodat de animatiespeler erbij kan) */
static unsigned int  cow_x  = 160;          /* horizontale positie (9-bit) */
static unsigned char cow_y  = 170;          /* verticale positie           */
static unsigned char loaded = LOADED_IDLE;  /* wat er in blok 32 staat     */

/* --- toestand van de animatiespeler --- */
static const anim_step *anim_steps;
static const anim_step *anim_cur;           /* stap die nu aan de beurt is */
static unsigned char anim_count   = 0;
static unsigned char anim_idx     = 0;
static unsigned char anim_timer   = 0;
static unsigned char anim_playing = 0;
static unsigned char anim_world   = 0;      /* eigen plek i.p.v. bij de cowboy */
static unsigned int  anim_wx      = 0;      /* meebewegend startpunt (WORLD)   */
static unsigned char anim_wy      = 0;
static signed   char anim_vx      = 0;      /* snelheid per beeld  (WORLD)     */
static signed   char anim_vy      = 0;
static unsigned char anim_pending = 0xFF;   /* animatie die op de oever wacht  */
static unsigned int  anim_cool    = ANIM_WAIT_MIN;  /* wachttijd tot de volgende */

/* --- toestand van de JUMP-animatie --- */
static unsigned char jump_state    = JUMP_INACTIVE;
static unsigned char jump_timer    = 0;
static unsigned int  jump_start_x  = 0;
static unsigned char jump_start_y  = 0;
static unsigned int  jump_arc_x    = 0;
static unsigned char jump_arc_y    = 0;
static unsigned char jump_arc_t    = 0;
static unsigned char jump_look_dir = 0;      /* 0 = links, 1 = rechts          */
static unsigned char jump_step     = 0;      /* voor klim-loop en zwem-loop    */

/* =====================================================================
 *  Hulpjes
 * ===================================================================== */

/* Zet de positie van sprite n. De X is 9 bits: de onderste 8 in $D000+2n,
 * het negende bit in $D010. Met vijf sprites wordt dat anders snel rommelig. */
static void set_sprite_pos(unsigned char n, unsigned int vx, unsigned char vy)
{
    unsigned char mask = (unsigned char)(1u << n);

    POKE(0xD000u + (n << 1), (unsigned char)(vx & 0xFF));
    POKE(0xD001u + (n << 1), vy);

    if (vx & 0x100) VIC.spr_hi_x |= mask;
    else            VIC.spr_hi_x = (unsigned char)(VIC.spr_hi_x & ~mask);
}

/* Staat de cowboy met zijn voeten op het land?
 * De VIC-spritecoordinaten lopen voor op het zichtbare beeld: schermpixel
 * x = vic_x - 24 en y = vic_y - 50. Het "voetpunt" is het midden onderaan
 * de sprite: 12 pixels naar rechts en 20 omlaag vanaf de linkerbovenhoek.
 *   voet_x (scherm) = vic_x - 24 + 12 = vic_x - 12
 *   voet_y (scherm) = vic_y - 50 + 20 = vic_y - 30
 */
static unsigned char on_land(unsigned int vx, unsigned char vy)
{
    unsigned int fx;
    unsigned char fy, i, fx2;

    if (vx < 12 || vy < 30) return 0;          /* voorkom onderloop */
    fx = vx - 12;
    fy = vy - 30;

    if (fy < SAND_TOP || fy > SAND_BOT) return 0;
    if (fx > 319) return 0;

    i   = (unsigned char)(fy - SAND_TOP);
    fx2 = (unsigned char)(fx >> 1);            /* naar multicolor-pixels */

    if (fx2 < sand_min_x[i] || fx2 > sand_max_x[i]) return 0;
    return 1;
}

/* Simpele 16-bits pseudo-random (xorshift). */
static unsigned int rnd_state = 0xACE1;
static unsigned int rnd(void)
{
    rnd_state ^= rnd_state << 7;
    rnd_state ^= rnd_state >> 9;
    rnd_state ^= rnd_state << 8;
    return rnd_state;
}

/* Laad de 3 frames van een richting in blok 32,33,34. */
static void load_dir(unsigned char dir)
{
    unsigned char fr;
    for (fr = 0; fr < 3; ++fr)
        memcpy((void *)(SPRITE_RAM + fr * 64u), cowboy_frames[dir][fr], 64);
}

/* Laad de rustframe in blok 32. */
static void load_idle(void)
{
    memcpy((void *)SPRITE_RAM, cowboy_idle, 64);
}

static void wait_frame(void)
{
    while (VIC.rasterline != 250) { }
    while (VIC.rasterline == 250) { }
}

/* =====================================================================
 *  Animatiespeler
 * ===================================================================== */

/* Zet de effect-sprites op hun plek. Bij een WORLD-animatie gebeurt dit elk
 * beeld opnieuw, want dan schuift het basispunt steeds op. Ligt een sprite
 * buiten beeld, dan zetten we 'm gewoon uit -- dat scheelt gedoe met
 * negatieve coordinaten aan de linkerrand. */
static void anim_place(void)
{
    unsigned char ena = (unsigned char)(VIC.spr_ena & 0xE7);  /* 3 en 4 uit */
    unsigned int  bx;
    unsigned char by;
    int px;

    if (anim_world) { bx = anim_wx; by = anim_wy; }
    else            { bx = cow_x;   by = cow_y;   }

    if (anim_cur->fxa_frame != FX_NONE)
    {
        px = (int)bx + anim_cur->fxa_dx;
        if (px >= 0 && px < 400)
        {
            set_sprite_pos(3, (unsigned int)px,
                              (unsigned char)((int)by + anim_cur->fxa_dy));
            ena |= 0x08;
        }
    }
    if (anim_cur->fxb_frame != FX_NONE)
    {
        px = (int)bx + anim_cur->fxb_dx;
        if (px >= 0 && px < 400)
        {
            set_sprite_pos(4, (unsigned int)px,
                              (unsigned char)((int)by + anim_cur->fxb_dy));
            ena |= 0x10;
        }
    }

    VIC.spr_ena = ena;
}

/* Zet de huidige stap klaar: de juiste plaatjes, kleuren en posities. */
static void anim_apply(void)
{
    anim_cur = &anim_steps[anim_idx];

    /* Houding van de cowboy. COW_KEEP laat hem staan zoals hij staat; met
       COW_LEFT/COW_RIGHT draait hij mee, bijvoorbeeld als hij door de
       verrekijker naar links tuurt. */
    if (anim_cur->cow != COW_KEEP)
    {
        if (anim_cur->cow == COW_IDLE)
            load_idle();
        else
            memcpy((void *)SPRITE_RAM, cowboy_frames[anim_cur->cow][0], 64);

        loaded = LOADED_IDLE;      /* blok 32 bevat nu geen loop-set meer */
        POKE(SPRITE_PTR, FIRST_BLOCK);
    }

    if (anim_cur->fxa_frame != FX_NONE)
    {
        POKE(SPRITE_PTR + 3, FX_BLOCK + anim_cur->fxa_frame);
        POKE(0xD027u + 3, anim_cur->fxa_color);
    }
    if (anim_cur->fxb_frame != FX_NONE)
    {
        POKE(SPRITE_PTR + 4, FX_BLOCK + anim_cur->fxb_frame);
        POKE(0xD027u + 4, anim_cur->fxb_color);
    }

    anim_place();
    anim_timer = anim_cur->ticks;
}

/* Effect-sprites uit en de besturing teruggeven. */
static void anim_stop(void)
{
    anim_playing   = 0;
    anim_world     = 0;
    anim_cool      = ANIM_WAIT_MIN + (rnd() & ANIM_WAIT_VAR);
    VIC.spr_ena    = (unsigned char)(VIC.spr_ena & 0xE7);
    VIC.spr_mcolor = 0x01;                 /* alleen de cowboy weer  */
    VIC.spr_exp_x  = (unsigned char)(VIC.spr_exp_x & 0xE7);  /* niet uitgerekt */
}

/* JUMP-animatie stoppen en alles weer normaal maken. */
static void jump_stop(void)
{
    jump_state = JUMP_INACTIVE;
    VIC.spr_ena = (unsigned char)(VIC.spr_ena & 0xE7);  /* effect-sprites uit */
    load_idle();
    loaded = LOADED_IDLE;
    POKE(SPRITE_PTR, FIRST_BLOCK);
}

/* Start de JUMP-animatie: cowboy loopt naar de palmboom. */
static void jump_start(void)
{
    jump_state   = JUMP_WALK;
    jump_timer   = 0;
    jump_look_dir = 0;
    jump_step    = 0;
}

/* Plaats effect-sprite 3 op (x,y) met het opgegeven blok. */
static void place_effect(unsigned char block, unsigned int x, unsigned char y,
                         unsigned char color, unsigned char multicolor)
{
    set_sprite_pos(3, x, y);
    POKE(SPRITE_PTR + 3, block);
    POKE(0xD027u + 3, color);
    if (multicolor)
        VIC.spr_mcolor |= 0x08;
    else
        VIC.spr_mcolor = (unsigned char)(VIC.spr_mcolor & ~0x08);
    VIC.spr_ena |= 0x08;
}

/* Eén beeld verder in de JUMP-animatie. */
static void jump_tick(void)
{
    unsigned char phase;
    static unsigned char j_tick = 0;
    static unsigned char j_step = 0;

    if (jump_state == JUMP_INACTIVE) return;

    switch (jump_state)
    {
        case JUMP_WALK:
            /* Loop naar de palmboom. Eerst horizontaal, dan verticaal. */
            if (cow_x < JUMP_TARGET_X)
            {
                if (loaded != DIR_RIGHT) { load_dir(DIR_RIGHT); loaded = DIR_RIGHT; }
                if (++j_tick >= ANIM_SPEED) { j_tick = 0; j_step = (j_step + 1) & 3; }
                cow_x += WALK_STEP;
                if (cow_x > JUMP_TARGET_X) cow_x = JUMP_TARGET_X;
            }
            else if (cow_x > JUMP_TARGET_X)
            {
                if (loaded != DIR_LEFT) { load_dir(DIR_LEFT); loaded = DIR_LEFT; }
                if (++j_tick >= ANIM_SPEED) { j_tick = 0; j_step = (j_step + 1) & 3; }
                if (cow_x >= WALK_STEP) cow_x -= WALK_STEP;
                if (cow_x < JUMP_TARGET_X) cow_x = JUMP_TARGET_X;
            }
            else if (cow_y > JUMP_TARGET_Y)
            {
                /* Nu op de juiste X; lopen omhoog naar de palmboom. */
                if (loaded != DIR_UP) { load_dir(DIR_UP); loaded = DIR_UP; }
                if (++j_tick >= ANIM_SPEED) { j_tick = 0; j_step = (j_step + 1) & 3; }
                if (cow_y >= WALK_STEP) cow_y -= WALK_STEP;
                if (cow_y < JUMP_TARGET_Y) cow_y = JUMP_TARGET_Y;
            }
            else if (cow_y < JUMP_TARGET_Y)
            {
                if (loaded != DIR_DOWN) { load_dir(DIR_DOWN); loaded = DIR_DOWN; }
                if (++j_tick >= ANIM_SPEED) { j_tick = 0; j_step = (j_step + 1) & 3; }
                cow_y += WALK_STEP;
                if (cow_y > JUMP_TARGET_Y) cow_y = JUMP_TARGET_Y;
            }
            else
            {
                j_tick = 0;
                jump_state = JUMP_TURN;
                jump_timer = 25;             /* even staan, rug naar kijker */
                load_dir(DIR_UP);            /* rug naar ons toe            */
                loaded = DIR_UP;
            }
            POKE(SPRITE_PTR, FIRST_BLOCK + cycle[j_step]);
            set_sprite_pos(0, cow_x, cow_y);
            break;

        case JUMP_TURN:
            /* Staat met de rug naar ons voor de palmboom. */
            if (--jump_timer == 0)
            {
                jump_state = JUMP_CLIMB;
                jump_timer = 0;
                jump_step  = 0;
            }
            break;

        case JUMP_CLIMB:
            /* Klim in kleine stapjes omhoog, wisselend tussen up en idle. */
            if (++jump_timer >= 8)
            {
                jump_timer = 0;
                if ((jump_step & 1) == 0)
                {
                    if (cow_y > JUMP_TOP_Y) cow_y -= 2;
                    load_dir(DIR_UP);
                    loaded = DIR_UP;
                }
                else
                {
                    /* Even "pauze" met rusthouding om klein-stapjes te simuleren. */
                    load_idle();
                    loaded = LOADED_IDLE;
                }
                POKE(SPRITE_PTR, FIRST_BLOCK);
                set_sprite_pos(0, cow_x, cow_y);
                if (++jump_step >= 14 && cow_y <= JUMP_TOP_Y)
                {
                    jump_state = JUMP_LOOK;
                    jump_timer = 30;         /* kijken rond                 */
                    jump_look_dir = 0;
                }
            }
            break;

        case JUMP_LOOK:
            /* Kijkt links, rechts, links, rechts. */
            phase = jump_timer / 15;
            if (phase == 0 || phase == 2)
            {
                if (loaded != DIR_LEFT) { load_dir(DIR_LEFT); loaded = DIR_LEFT; }
            }
            else
            {
                if (loaded != DIR_RIGHT) { load_dir(DIR_RIGHT); loaded = DIR_RIGHT; }
            }
            POKE(SPRITE_PTR, FIRST_BLOCK + cycle[j_step]);
            set_sprite_pos(0, cow_x, cow_y);
            if (--jump_timer == 0)
            {
                jump_state = JUMP_LEAP;
                jump_arc_t = 0;
                jump_start_x = cow_x;
                jump_start_y = cow_y;
                /* Zet de cowboy voor de sprong naar een zijwaartse houding. */
                load_dir(DIR_RIGHT);
                loaded = DIR_RIGHT;
            }
            break;

        case JUMP_LEAP:
        {
            /* Horizontale sprong naar links, Y blijft gelijk. t loopt 0..32. */
            unsigned char t = jump_arc_t;
            unsigned int dx = ((unsigned int)(JUMP_TOP_X - JUMP_WATER_X) * t) / 32u;
            unsigned int arc = 0;
            unsigned int dy;

            /* Kleine boog omhoog in het midden van de sprong. */
            if (t < 16)
                arc = (unsigned int)t * (16u - (unsigned int)t) / 8u;
            else
                arc = (unsigned int)(32u - t) * ((unsigned int)t - 16u) / 8u;

            cow_x = (unsigned int)(JUMP_TOP_X - dx);
            dy = (unsigned int)JUMP_TOP_Y - arc;
            if (dy > 250) dy = 0;            /* veiligheid tegen onderloop    */
            cow_y = (unsigned char)dy;

            set_sprite_pos(0, cow_x, cow_y);
            /* Wissel tussen linker en idle frame tijdens de sprong. */
            if ((t & 3) == 0)
            {
                if (loaded != DIR_LEFT) { load_dir(DIR_LEFT); loaded = DIR_LEFT; }
                j_step = (j_step + 1) & 3;
                POKE(SPRITE_PTR, FIRST_BLOCK + cycle[j_step]);
            }

            if (++jump_arc_t >= 32)
            {
                jump_state = JUMP_SPLASH;
                jump_timer = 20;             /* plons-effect                */
                /* Cowboy even verbergen onder water. */
                VIC.spr_ena = (unsigned char)(VIC.spr_ena & ~0x01);
                place_effect(SPLASH0_BLOCK, cow_x, (unsigned char)(cow_y + 10), COLOR_WHITE, 1);
            }
            break;
        }

        case JUMP_SPLASH:
            /* Toon plons, dan hoofdje boven water. */
            if (jump_timer == 15)
                place_effect(SPLASH1_BLOCK, cow_x, (unsigned char)(cow_y + 10), COLOR_WHITE, 1);
            if (--jump_timer == 0)
            {
                jump_state = JUMP_SWIM;
                jump_step  = 0;
                cow_x = JUMP_WATER_X;
                cow_y = JUMP_SWIM_Y;
                /* Hoofd boven water als effect. */
                place_effect(SWIM_HEAD_BLOCK, cow_x, (unsigned char)(cow_y + 10), COLOR_ORANGE, 1);
                set_sprite_pos(0, cow_x, cow_y); /* cowboy-sprite volgt onzichtbaar mee */
            }
            break;

        case JUMP_SWIM:
            /* Zwem van links (water) naar rechts naar de linkerrand van het
               wandelgrid (JUMP_SWIM_X).  Onderweg blijft het hoofdje zichtbaar. */
            if (++jump_timer >= 6)
            {
                jump_timer = 0;
                if (cow_x < JUMP_SWIM_X)
                {
                    cow_x += 2;
                    /* Bob op het water. */
                    cow_y = (unsigned char)(JUMP_SWIM_Y + ((jump_step & 3) == 0 ? 1 : 0));
                    place_effect(SWIM_HEAD_BLOCK, cow_x, (unsigned char)(cow_y + 10), COLOR_ORANGE, 1);
                    set_sprite_pos(0, cow_x, cow_y);
                    ++jump_step;
                }
                else
                {
                    jump_state = JUMP_CLIMB_OUT;
                    jump_timer = 0;
                    jump_step  = 0;
                    VIC.spr_ena |= 0x01;     /* cowboy weer zichtbaar        */
                    load_dir(DIR_UP);
                    loaded = DIR_UP;
                }
            }
            break;

        case JUMP_CLIMB_OUT:
            /* Klim weer op het eiland in kleine stapjes.
               De linkerrand van het grid ligt lager dan de palmboom; de
               cowboy moet hier omhoog naar SAND_TOP+21+30 = 182, net binnen
               het wandelgrid op zijn smalste punt. */
#define JUMP_EXIT_Y 182
            if (++jump_timer >= 10)
            {
                jump_timer = 0;
                if ((jump_step & 1) == 0)
                {
                    if (cow_y > JUMP_EXIT_Y) cow_y += 3;
                    load_dir(DIR_UP);
                    loaded = DIR_UP;
                }
                else
                {
                    load_idle();
                    loaded = LOADED_IDLE;
                }
                POKE(SPRITE_PTR, FIRST_BLOCK);
                set_sprite_pos(0, cow_x, cow_y);
                if (++jump_step >= 8 && cow_y >= JUMP_EXIT_Y)
                {
                    jump_state = JUMP_DONE;
                    jump_timer = 20;
                    load_idle();
                    loaded = LOADED_IDLE;
                    POKE(SPRITE_PTR, FIRST_BLOCK);
                }
            }
            break;

        case JUMP_DONE:
            /* Even rusten, dan is de animatie afgelopen. */
            if (--jump_timer == 0)
            {
                jump_stop();
                anim_cool = ANIM_WAIT_MIN + (rnd() & ANIM_WAIT_VAR);
            }
            break;
    }
}

/* Begin animatie n uit de lijst in anims.c. */
static void anim_start(unsigned char n)
{
    const animation *a = &animations[n];

    anim_steps   = a->steps;
    anim_count   = a->count;
    anim_idx     = 0;
    anim_playing = 1;

    /* Speelt het zich af naast de cowboy, of ergens anders in de wereld? */
    anim_world = (unsigned char)(a->flags & ANIM_WORLD);
    anim_wx    = a->start_x;
    anim_wy    = a->start_y;
    anim_vx    = a->vx;
    anim_vy    = a->vy;

    /* Welke effect-sprites zijn multicolor? Sprite 0 blijft dat altijd. */
    VIC.spr_mcolor = (unsigned char)(0x01
                     | ((a->mcolor & 1) ? 0x08 : 0)
                     | ((a->mcolor & 2) ? 0x10 : 0));

    /* Sprites die 2x zo breed getekend moeten worden. */
    VIC.spr_exp_x = (unsigned char)((VIC.spr_exp_x & 0xE7)
                    | ((a->flags & ANIM_XEXP_A) ? 0x08 : 0)
                    | ((a->flags & ANIM_XEXP_B) ? 0x10 : 0));

    /* Doet de cowboy het zelf? Dan blijft hij staan kijken. Speelt het
       ergens anders in de wereld, dan loopt hij gewoon door. */
    if (!anim_world)
    {
        load_idle();
        loaded = LOADED_IDLE;
        POKE(SPRITE_PTR, FIRST_BLOCK);
    }

    anim_apply();
}

/* Eén beeld verder in de lopende animatie. */
static void anim_tick(void)
{
    if (anim_timer) --anim_timer;
    if (anim_timer == 0)
    {
        ++anim_idx;
        if (anim_idx >= anim_count) anim_stop();
        else                        anim_apply();
    }
}

/* =====================================================================
 *  Achtergrond
 * ===================================================================== */

/* Pak een met RLE ingepakt blok uit naar dst.
 *   stuurbyte n = 0..127   -> de volgende n+1 bytes staan er letterlijk achter
 *   stuurbyte n = 129..255 -> herhaal de volgende byte 257-n keer (2..128)
 */
static void unpack(const unsigned char *src, unsigned char *dst, unsigned int len)
{
    unsigned char n, v;
    unsigned int done = 0;

    while (done < len)
    {
        n = *src++;
        if (n < 128)                       /* letterlijke bytes */
        {
            ++n;
            done += n;
            while (n--) *dst++ = *src++;
        }
        else                               /* herhaling */
        {
            n = (unsigned char)(257u - n);
            v = *src++;
            done += n;
            while (n--) *dst++ = v;
        }
    }
}

/* Pak een compleet scherm uit naar de VIC-adressen. Het beeld staat tijdens
   het uitpakken uit, anders zie je het plaatje regel voor regel opbouwen. */
static void show_screen(const unsigned char *bm, const unsigned char *sc,
                        const unsigned char *cr, unsigned char bg,
                        unsigned char border)
{
    VIC.ctrl1 = (unsigned char)(VIC.ctrl1 & 0xEF);     /* beeld uit ($D011 bit4) */

    unpack(bm, (unsigned char *)BITMAP_RAM, 8000);
    unpack(sc, (unsigned char *)SCREEN_RAM, 1000);
    unpack(cr, (unsigned char *)COLRAM_ADDR, 1000);

    POKE(0xD021, bg);
    VIC.bordercolor = border;

    VIC.ctrl1 |= 0x10;                                 /* beeld weer aan        */
}

/* Wordt er een vuurknop ingedrukt? Bij allebei de joystickpoorten is bit 4
 * de vuurknop, en een 0-bit betekent ingedrukt:
 *
 *   joystick 1 -> CIA1 poort B ($DC01), dezelfde poort waarop we de
 *                 toetsenbordkolommen lezen. Door eerst $FF naar $DC00 te
 *                 schrijven selecteren we geen enkele toetsenbordrij, zodat
 *                 alleen de joystick nog bits laag kan trekken.
 *
 *   joystick 2 -> CIA1 poort A ($DC00). Dat is juist de poort waarmee we
 *                 toetsenbordrijen SELECTEREN, en die staat daarom als
 *                 uitgang ingesteld; dan lees je je eigen uitgangswaarde
 *                 terug in plaats van de joystick. We zetten hem dus heel
 *                 even op ingang ($DC02 = 0) en meteen daarna terug.
 */
static unsigned char fire_pressed(void)
{
    unsigned char v;

    /* joystick 1 */
    POKE(0xDC00, 0xFF);                      /* geen toetsenbordrij actief */
    if ((PEEK(0xDC01) & 0x10) == 0) return 1;

    /* joystick 2 */
    POKE(0xDC02, 0x00);                      /* poort A even als ingang    */
    v = PEEK(0xDC00);
    POKE(0xDC02, 0xFF);                      /* en weer als uitgang        */

    return (unsigned char)((v & 0x10) == 0);
}

/* Wacht op de spatiebalk (rij 7, bit 4 van de toetsenbordmatrix) of op een
 * vuurknop. Eerst wachten tot alles los is, zodat een toets of knop die nog
 * van het opstarten nagalmt het titelscherm niet meteen wegklikt.
 * Tegelijkertijd draait de lichtkrant aan de onderkant van het scherm. */
static void wait_start(void)
{
    for (;;)                                 /* wachten tot alles los is */
    {
        wait_frame();
        marquee_update();
        POKE(0xDC00, KEYROW_SPACE);
        if ((PEEK(0xDC01) & KEYBIT_SPACE) && !fire_pressed()) break;
    }
    for (;;)                                 /* en dan op de startknop    */
    {
        wait_frame();
        marquee_update();
        POKE(0xDC00, KEYROW_SPACE);
        if ((PEEK(0xDC01) & KEYBIT_SPACE) == 0) break;
        if (fire_pressed()) break;
    }
}

/* Ligt de linkeroever dichterbij dan de rechter? Het masker geeft per
 * beeldrij de rand van het zand, dus we vergelijken gewoon de afstand tot
 * links met die tot rechts op de rij waar hij staat. */
static unsigned char left_is_nearer(void)
{
    unsigned char i   = (unsigned char)(cow_y - 30u - SAND_TOP);
    unsigned char fx2 = (unsigned char)((cow_x - 12u) >> 1);

    return (unsigned char)((fx2 - sand_min_x[i]) < (sand_max_x[i] - fx2));
}

/* Vraag animatie n aan.
 *  - Kan hij meteen beginnen, dan gebeurt dat en komt 0xFF terug.
 *  - Moet hij ervoor aan het water staan (ANIM_AT_SHORE), dan kiest deze
 *    functie de dichtstbijzijnde oever -- en zonodig de gespiegelde versie
 *    van de animatie -- en geeft de looprichting terug. De hoofdlus laat hem
 *    daarheen lopen en start het toneelstukje zodra hij het water raakt.
 */
static unsigned char request_anim(unsigned char n)
{
    unsigned char want_left, is_left;

    if (animations[n].flags & ANIM_AT_SHORE)
    {
        want_left = left_is_nearer();
        is_left   = (unsigned char)((animations[n].flags & ANIM_TO_LEFT) != 0);

        if (want_left != is_left && animations[n].mirror != ANIM_NO_MIRROR)
            n = animations[n].mirror;          /* pak de gespiegelde versie */

        anim_pending = n;
        return want_left ? DIR_LEFT : DIR_RIGHT;
    }

    anim_start(n);
    return 0xFF;
}

/* Zet de VIC in multicolor bitmap-modus in bank 1. Doet nog geen plaatje;
   dat gaat via show_screen(). */
static void setup_background(void)
{
    /* 2. VIC naar bank 1 ($4000-$7FFF) via CIA2 poort A.
          Bits 0-1 zijn geinverteerd: bankwaarde = 3 - bank. */
    POKE(0xDD02, PEEK(0xDD02) | 0x03);                 /* bits als uitgang  */
    POKE(0xDD00, (PEEK(0xDD00) & 0xFC) | (3u - VIC_BANK));

    /* 3. Screen op offset $1C00, bitmap op offset $2000 binnen de bank.
          $D018 = (screen/1024)<<4 | (bitmap/8192)<<3 = 7<<4 | 1<<3 = $78.
          De graphics zitten expres hoog in de bank, zodat het programma
          zelf kan doorgroeien tot $5000 als jij animaties toevoegt. */
    POKE(0xD018, 0x78);

    /* 4. Bitmap-modus + multicolor aan. */
    VIC.ctrl1 |= 0x20;                                 /* $D011 bit5 = BMM  */
    VIC.ctrl2 |= 0x10;                                 /* $D016 bit4 = MCM  */

}

/* =====================================================================
 *  Hoofdprogramma
 * ===================================================================== */

int main(void)
{
    unsigned char dir  = DIR_RIGHT;      /* huidige looprichting        */
    unsigned char step = 0;              /* index in cycle[]            */
    unsigned char tick = 0;              /* beeld-teller                */

    unsigned char row0;
    unsigned char crsr_lr, crsr_ud, shift;
    unsigned char want_right, want_left, want_down, want_up, keypress;

    unsigned char active = 0;            /* loopt hij dit beeld?        */
    unsigned char idle_timer  = 0;       /* beelden zonder toets        */
    unsigned char wandering   = 0;       /* dwaalt hij zelf rond?       */
    unsigned char wander_left = 0;       /* beelden in deze richting    */
    unsigned char wander_rest = 0;       /* beelden even stilstaan      */
    unsigned int  r;

    unsigned int  nx;                    /* voorgestelde nieuwe positie */
    unsigned char ny;
    unsigned char i;
    unsigned char hot;                   /* animatie met ingedrukte toets */
    unsigned char hot_prev = 0xFF;       /* zelfde, vorig beeld           */
    unsigned char d;                     /* looprichting naar de oever    */

    /* --- meeuwen (sprites 1 en 2) --- */
    unsigned int  g1x = 30,  g2x = 170;  /* horizontale posities        */
    unsigned char g1sub = 0, g2sub = 0;  /* deelteller voor de snelheid */
    unsigned char g1fl = 0,  g2fl = 0;   /* deelteller voor het klapper */
    unsigned char g1fr = 0,  g2fr = 1;   /* huidig vleugelframe         */
    unsigned char g1bob = 0, g2bob = 4;  /* fase van het op-en-neer     */

    /* VIC in multicolor bitmap-modus zetten (bank 1). */
    setup_background();

    /* ---------------- TITELSCHERM ----------------
       Sprites uit, titelscherm uitpakken en wachten op de spatiebalk.
       Onderaan verschijnt een witte lichtkrant. */
    VIC.spr_ena = 0x00;
    show_screen(intro_bitmap_rle, intro_screen_rle, intro_color_rle,
                INTRO_BGCOLOR, INTRO_BORDERCOLOR);
    setup_marquee_colors();
    wait_start();

    /* ---------------- HET EILAND ----------------
       Het eiland eroverheen uitpakken; daarna begint het programma. */
    show_screen(island_bitmap_rle, island_screen_rle, island_color_rle,
                ISLAND_BGCOLOR, ISLAND_BGCOLOR);

    /* Random-generator zaaien met de CIA-timer, zodat het dwalen niet
       elke keer exact hetzelfde patroon volgt. */
    rnd_state = (unsigned int)PEEK(0xDC04) | ((unsigned int)PEEK(0xDC05) << 8);
    if (rnd_state == 0) rnd_state = 0xACE1;

    /* Sprite 0 (cowboy): multicolor + gedeelde kleuren.
       Sprites 1 en 2 (meeuwen): hi-res wit, dus GEEN multicolor-bit. */
    VIC.spr_mcolor  = 0x01;              /* alleen sprite 0 multicolor  */
    VIC.spr_mcolor0 = COLOR_YELLOW;      /* $D025 code 01 = hoed        */
    VIC.spr_mcolor1 = COLOR_ORANGE;      /* $D026 code 11 = lichtbruin  */
    VIC.spr0_color  = COLOR_BROWN;       /* $D027 code 10 = donkerbruin */
    VIC.spr1_color  = COLOR_WHITE;       /* $D028 meeuw 1               */
    VIC.spr2_color  = COLOR_WHITE;       /* $D029 meeuw 2               */
    VIC.spr_ena     = 0x07;              /* sprites 0, 1 en 2 aan       */

    /* Meeuw-frames een keer inladen; die veranderen verder niet. */
    memcpy((void *)(SPRITE_RAM + 3 * 64u), gull_frames[0], 64);
    memcpy((void *)(SPRITE_RAM + 4 * 64u), gull_frames[1], 64);
    POKE(SPRITE_PTR + 1, GULL_BLOCK + g1fr);
    POKE(SPRITE_PTR + 2, GULL_BLOCK + g2fr);
    set_sprite_pos(1, g1x, GULL1_Y);
    set_sprite_pos(2, g2x, GULL2_Y);

    /* Effect-frames voor de animaties inladen (blok 37 en verder). */
    for (i = 0; i < FX_FRAME_COUNT; ++i)
        memcpy((void *)(SPRITE_RAM + (5 + i) * 64u), fx_frames[i], 64);

    /* Beginpositie meteen naar de VIC-registers, anders staat de sprite
       op x=0 (in de linkerrand) tot je voor het eerst beweegt. */
    set_sprite_pos(0, cow_x, cow_y);

    load_idle();                         /* begin stilstaand           */
    POKE(SPRITE_PTR, FIRST_BLOCK);

    while (1)
    {
        wait_frame();

        /* Wachttijd tot de volgende spontane animatie laten aftikken. */
        if (anim_cool) --anim_cool;

        /* --- lopende animatie een beeld verder --- */
        if (jump_state != JUMP_INACTIVE)
        {
            jump_tick();
        }
        else if (anim_playing)
        {
            if (anim_world)
            {
                /* Schuift zijn eigen weg door de wereld, los van de cowboy. */
                anim_wx = (unsigned int)((int)anim_wx + anim_vx);
                anim_wy = (unsigned char)((int)anim_wy + anim_vy);
                anim_place();
            }
            anim_tick();
        }

        /* ============== meeuwen (sprites 1 en 2) ============== */
        /* Ze schuiven langzaam naar rechts en komen links weer binnen.
           Verschillende snelheden en klapper-tempo's, zodat het geen
           tweeling wordt. Het "bob"-tabelletje geeft ze een zacht
           op-en-neer, alsof ze op de wind drijven. */
        if (++g1sub >= 2) { g1sub = 0; if (++g1x > GULL_WRAP) g1x = 0; }
        if (++g2sub >= 3) { g2sub = 0; if (++g2x > GULL_WRAP) g2x = 0; }

        if (++g1fl >= 10) { g1fl = 0; g1fr ^= 1; ++g1bob;
                            POKE(SPRITE_PTR + 1, GULL_BLOCK + g1fr); }
        if (++g2fl >= 13) { g2fl = 0; g2fr ^= 1; ++g2bob;
                            POKE(SPRITE_PTR + 2, GULL_BLOCK + g2fr); }

        set_sprite_pos(1, g1x, (unsigned char)(GULL1_Y + bob[g1bob & 7]));
        set_sprite_pos(2, g2x, (unsigned char)(GULL2_Y + bob[g2bob & 7]));

        /* --- toetsenbord uitlezen (matrix) --- */
        POKE(0xDC00, 0xFE);                        /* rij 0 selecteren        */
        row0 = PEEK(0xDC01);
        crsr_lr = (row0 & 0x04) == 0;              /* cursor L/R-toets in?    */
        crsr_ud = (row0 & 0x80) == 0;              /* cursor U/D-toets in?    */

        shift = 0;
        POKE(0xDC00, 0xFD);                        /* rij 1: left shift bit7  */
        if ((PEEK(0xDC01) & 0x80) == 0) shift = 1;
        POKE(0xDC00, 0xBF);                        /* rij 6: right shift bit4 */
        if ((PEEK(0xDC01) & 0x10) == 0) shift = 1;

        want_right = crsr_lr && !shift;
        want_left  = crsr_lr &&  shift;
        want_down  = crsr_ud && !shift;
        want_up    = crsr_ud &&  shift;
        keypress   = want_right || want_left || want_down || want_up;

        /* --- sneltoetsen voor de animaties (bv. F van Fire) ---
           We kijken welke animatie een ingedrukte toets heeft. Alleen bij
           een NIEUWE aanslag starten we 'm, anders zou hij blijven
           herstarten zolang je de toets vasthoudt. */
        hot = 0xFF;
        for (i = 0; i < animation_count; ++i)
        {
            if (animations[i].key_row == KEY_GEEN) continue;
            POKE(0xDC00, animations[i].key_row);
            if ((PEEK(0xDC01) & animations[i].key_bit) == 0) { hot = i; break; }
        }
        /* Speciale J-sneltoets: start de grote JUMP-state machine.
           We moeten expliciet rij J selecteren; na de loop staat $DC00
           op een andere rij. */
        POKE(0xDC00, KEYROW_J);
        if ((PEEK(0xDC01) & KEYBIT_J) == 0
            && jump_state == JUMP_INACTIVE && !anim_playing
            && anim_pending == 0xFF && hot_prev != 0xFE)
        {
            hot = 0xFE;                  /* marker: J is ingedrukt          */
            jump_start();
        }
        else if (hot != 0xFF && hot != hot_prev && !anim_playing
                 && anim_pending == 0xFF && jump_state == JUMP_INACTIVE)
        {
            d = request_anim(hot);
            if (d != 0xFF) dir = d;      /* eerst naar de waterkant lopen */
        }
        if (hot != 0xFE) hot_prev = hot;
        else             hot_prev = 0xFF; /* J wordt pas losgelaten herkend */

        /* ================= wie bepaalt de richting? ================= */
        if (keypress)
        {
            /* Jij hebt voorrang: dwalen stopt, en een toneelstukje dat de
               cowboy zelf opvoert breken we af. Een animatie die ergens
               anders in de wereld speelt (de haai) laten we doorlopen. */
            if (anim_playing && !anim_world) anim_stop();
            if (jump_state != JUMP_INACTIVE) jump_stop();
            anim_pending = 0xFF;         /* naar de oever lopen vervalt   */
            idle_timer  = 0;
            wandering   = 0;
            wander_left = 0;
            wander_rest = 0;

            if      (want_right) dir = DIR_RIGHT;   /* horizontaal wint bij */
            else if (want_left)  dir = DIR_LEFT;    /* een diagonaal        */
            else if (want_down)  dir = DIR_DOWN;
            else                 dir = DIR_UP;
            active = 1;
        }
        else if (jump_state != JUMP_INACTIVE)
        {
            /* JUMP-sequence bezet de cowboy volledig. */
            active = 0;
        }
        else if (anim_playing && !anim_world)
        {
            /* Hij voert zelf iets op: staat stil tot het klaar is. */
            active = 0;
        }
        else if (anim_pending != 0xFF)
        {
            /* Onderweg naar de waterkant; dir staat al goed. Zodra hij het
               water raakt begint de animatie (zie hieronder bij "botsen"). */
            active = 1;
        }
        else if (wandering)
        {
            if (wander_rest)                        /* even uitrusten       */
            {
                --wander_rest;
                active = 0;
            }
            else
            {
                if (wander_left == 0)               /* nieuwe koers kiezen  */
                {
                    r = rnd();

                    /* Is de wachttijd om, dan geen nieuwe koers maar een
                       willekeurige animatie. */
                    if (anim_cool == 0 && animation_count)
                    {
                        d = request_anim((unsigned char)((r >> 8) % animation_count));
                        if (d != 0xFF) dir = d;     /* eerst naar het water */
                        active = (unsigned char)(d != 0xFF);
                        goto moved;
                    }

                    /* Het eiland is breed (218 px) maar plat (41 px). Een
                       lange verticale stap knalt dus altijd tegen de boven-
                       of onderrand, en juist daar is het eiland smal -- dan
                       blijft hij in het middenstuk hangen. Daarom: meestal
                       horizontaal en lang, af en toe verticaal en kort. */
                    if ((r & 3) == 0)
                    {
                        dir = (unsigned char)(DIR_DOWN + ((r >> 2) & 1));
                        wander_left = (unsigned char)(6 + ((r >> 4) & 15));
                    }
                    else
                    {
                        dir = (unsigned char)((r >> 2) & 1);   /* RIGHT/LEFT */
                        wander_left = (unsigned char)(30 + ((r >> 4) & 127));
                    }
                    if ((r & 0x1C) == 0)            /* soms even blijven staan */
                        wander_rest = (unsigned char)(30 + ((r >> 10) & 31));
                }
                --wander_left;
                active = 1;
            }
        }
        else
        {
            /* Niets ingedrukt: aftellen tot hij uit zichzelf gaat lopen. */
            active = 0;
            if (++idle_timer >= IDLE_TIMEOUT)
            {
                wandering   = 1;
                wander_left = 0;
                idle_timer  = 0;
            }
        }

        /* ===================== bewegen + botsen ===================== */
        if (active)
        {
            /* Zorg dat de juiste loopframes in blok 32/33/34 staan. Nodig bij
               richtingwissel EN bij overgang vanuit stilstand (waar blok 32
               door de rustframe is overschreven). */
            if (loaded != dir)
            {
                load_dir(dir);
                loaded = dir;
            }

            /* Eerst de voorgestelde stap uitrekenen... */
            nx = cow_x;
            ny = cow_y;
            switch (dir)
            {
                case DIR_RIGHT: nx += WALK_STEP; break;
                case DIR_LEFT:  if (nx >= WALK_STEP) nx -= WALK_STEP; break;
                case DIR_DOWN:  ny += WALK_STEP; break;
                default:        if (ny >= WALK_STEP) ny -= WALK_STEP; break;
            }

            /* ...en die alleen uitvoeren als zijn voeten op het zand blijven.
               Zo loopt hij niet het water in. */
            if (on_land(nx, ny))
            {
                cow_x = nx;
                cow_y = ny;
                set_sprite_pos(0, cow_x, cow_y);
            }
            else if (anim_pending != 0xFF)
            {
                /* Hij staat met zijn voeten aan het water: hier wilde hij
                   zijn. Nu pas begint het toneelstukje. */
                anim_start(anim_pending);
                anim_pending = 0xFF;
            }
            else
            {
                /* Water! Blijf staan. Als hij zelf aan het dwalen is, meteen
                   een andere koers kiezen zodat hij niet vastloopt tegen de
                   waterkant. */
                if (wandering) wander_left = 0;
            }

            /* --- animatie doortikken --- */
            if (++tick >= ANIM_SPEED)
            {
                tick = 0;
                step = (step + 1) & 3;
                POKE(SPRITE_PTR, FIRST_BLOCK + cycle[step]);
            }
        }
        else
        {
            /* Stil: NIET terug naar de rustframe, maar bevriezen op het
               laatst getoonde frame. */
            tick = 0;
        }
moved: ;
    }
}
