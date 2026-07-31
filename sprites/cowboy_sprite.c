/*
 * cowboy_sprite.c  -  C64 multicolor sprite voor de CC65 compiler
 * ---------------------------------------------------------------
 * Gegenereerd uit de aangeleverde pixel-art (24x21, 4 kleuren).
 *
 * MULTICOLOR bit-paren per pixel:
 *   00 = transparant  (achtergrond)
 *   01 = multicolor 0  -> register $D025 (VIC_SPR_MCOLOR0)  [hoed]
 *   10 = sprite-kleur  -> register $D027.. (individueel)    [donkerbruin]
 *   11 = multicolor 1  -> register $D026 (VIC_SPR_MCOLOR1)  [lichtbruin]
 *
 * 63 databytes (3 per rij x 21 rijen) + 1 padding = 64 bytes.
 * Sprite-pointer = adres/64.  Plaats dit blok dus op een 64-byte grens.
 */

/* De sprite-pointer verwacht data op een 64-byte grens. Dat regelen we
 * niet hier, maar door de bytes bij het opstarten naar een VIC-locatie op
 * een 64-byte grens te kopieren (zie main.c). Zo blijft dit bestand puur data. */
const unsigned char cowboy_sprite[64] = {
    0x00, 0x00, 0x00,  // row  0
    0x00, 0x54, 0x00,  // row  1
    0x00, 0x54, 0x00,  // row  2
    0x00, 0xA8, 0x00,  // row  3
    0x01, 0x55, 0x50,  // row  4
    0x06, 0xAA, 0xA0,  // row  5
    0x0A, 0xF3, 0x00,  // row  6
    0x02, 0xFF, 0x00,  // row  7
    0x00, 0xFC, 0x00,  // row  8
    0x0A, 0xAA, 0x80,  // row  9
    0x0A, 0xA6, 0x80,  // row 10
    0x0E, 0xAA, 0xC0,  // row 11
    0x0F, 0xE6, 0xF0,  // row 12
    0x0F, 0xEA, 0xF0,  // row 13
    0x0B, 0xE6, 0x00,  // row 14
    0x0A, 0xAA, 0x00,  // row 15
    0x0A, 0x0A, 0x00,  // row 16
    0x0F, 0x0F, 0x00,  // row 17
    0x0F, 0x0F, 0x00,  // row 18
    0x0F, 0xCF, 0xC0,  // row 19
    0x03, 0xCF, 0xC0,  // row 20
    0x00               // padding to 64 bytes
};
