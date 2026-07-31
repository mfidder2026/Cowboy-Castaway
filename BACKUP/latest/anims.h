#ifndef ANIMS_H
#define ANIMS_H

/* =====================================================================
 *  ANIMATIE-FRAMEWORK
 * =====================================================================
 *
 *  Een "animatie" is een klein toneelstukje dat de cowboy af en toe
 *  opvoert terwijl hij rondloopt: hij blijft staan, er gebeurt iets naast
 *  hem (vuurtje, rook, ...), en daarna loopt hij weer verder.
 *
 *  Er zijn twee EFFECT-SPRITES beschikbaar naast de cowboy en de meeuwen:
 *
 *      effect A  = sprite 3   (in het kampvuur: de vlammen)
 *      effect B  = sprite 4   (in het kampvuur: de rook)
 *
 *  Elke stap van een animatie zegt: welk plaatje elk effect laat zien,
 *  in welke kleur, hoeveel pixels naast/boven de cowboy het staat, en
 *  hoe lang die stap duurt. Meer heb je niet nodig.
 *
 * ---------------------------------------------------------------------
 *  ZELF EEN ANIMATIE TOEVOEGEN -- in 3 stappen, allemaal in anims.c
 * ---------------------------------------------------------------------
 *
 *  1) TEKEN JE PLAATJES
 *     Bovenaan anims.c staan de effect-frames als 64-byte blokken, met
 *     de originele ASCII-tekening erboven in commentaar. Plak er nieuwe
 *     frames achter en verhoog FX_FRAME_COUNT hieronder. Geef elk frame
 *     een naam met een #define (net als FX_FIRE0 hieronder).
 *
 *     Effect A is een MULTICOLOR sprite (12 dubbelbrede pixels per rij,
 *     4 kleuren) en effect B is HI-RES (24 smalle pixels, 1 kleur).
 *     Welke van de twee je gebruikt bepaal je per animatie met het
 *     mcolor-veld; zie punt 3.
 *
 *  2) SCHRIJF DE STAPPEN
 *     Maak een tabel zoals campfire_steps[] in anims.c. Eén regel is één
 *     stap. Gebruik FX_NONE als een effect even niets moet laten zien.
 *     De duur is in beelden: 50 = ongeveer een seconde (PAL).
 *
 *     Speelt het zich naast de cowboy af (ANIM_FOLLOW) of ergens anders in
 *     de wereld met een eigen snelheid (ANIM_WORLD)? Zie de uitleg bij de
 *     vlaggen verderop; het kampvuur is een FOLLOW, de haai een WORLD.
 *
 *  3) MELD 'M AAN
 *     Zet je tabel erbij in animations[] onderaan anims.c, en geef 'm
 *     desgewenst een SNELTOETS (bijvoorbeeld KEYROW_F / KEYBIT_F voor de
 *     F van Fire). Klaar -- de cowboy pikt hem vanzelf op: hij speelt af
 *     en toe een willekeurige animatie uit de lijst, en jij kunt er zelf
 *     een starten met de bijbehorende toets.
 *
 * =====================================================================
 */

/* Gebruik dit als een effect-sprite even niets moet tonen. */
#define FX_NONE         0xFF

/* --- namen van de effect-frames (index in fx_frames[]) --------------- */
#define FX_FIRE0        0       /* vonkje, net aangestoken   (multicolor) */
#define FX_FIRE1        1       /* klein vlammetje                        */
#define FX_FIRE2        2       /* vlam, stand A                          */
#define FX_FIRE3        3       /* vlam, stand B (flikkering)             */
#define FX_FIRE4        4       /* dovende vlam                           */
#define FX_FIRE5        5       /* nagloeiende sintels                    */
#define FX_SMOKE0       6       /* rookslierje, laag        (hi-res)      */
#define FX_SMOKE1       7       /* rook, hoger                            */
#define FX_SMOKE2       8       /* rookpluim                              */
#define FX_SMOKE3       9       /* rook die vervaagt                      */
#define FX_FIN0        10       /* haaienvin, stand A       (hi-res)      */
#define FX_FIN1        11       /* haaienvin, stand B                     */
#define FX_WAKE0       12       /* kielzog, stand A         (hi-res)      */
#define FX_WAKE1       13       /* kielzog, stand B                       */
#define FX_WAKE2       14       /* kielzog, ijler                         */
#define FX_ROD0        15       /* hengel geheven, nog geen lijn (hi-res) */
#define FX_ROD1        16       /* de worp: lijn vliegt naar rechts       */
#define FX_ROD2        17       /* lijn hangt stil                        */
#define FX_ROD3        18       /* lijn hangt, iets verschoven (dobberen) */
#define FX_ROD4        19       /* hengel gebogen: beet!                  */
#define FX_RIPPLE0     20       /* rimpeling op het water   (multicolor)  */
#define FX_RIPPLE1     21       /* rimpeling, andere stand                */
#define FX_FISH        22       /* het gevangen visje                     */
#define FX_LROD0       23       /* dezelfde hengel, maar gespiegeld:      */
#define FX_LROD1       24       /*   voor vissen aan de LINKERkant        */
#define FX_LROD2       25
#define FX_LROD3       26
#define FX_LROD4       27
#define FX_LRIPPLE0    28
#define FX_LRIPPLE1    29
#define FX_LFISH       30
#define FX_BINO_R      31       /* zwarte verrekijker, naar rechts (hi-res) */
#define FX_BINO_L      32       /* dezelfde, gespiegeld naar links          */

/* JUMP-animatie: plons en zwemmend hoofd (de cowboy zelf beweegt) */
#define FX_SPLASH0     33       /* plons in het water                       */
#define FX_SPLASH1     34       /* plons, andere stand                      */
#define FX_SWIM_HEAD   35       /* hoofdje boven water                        */

#define FX_FRAME_COUNT  36      /* <-- ophogen als je frames toevoegt     */

/* --- één stap van een animatie -------------------------------------- */
typedef struct {
    unsigned char fxa_frame;    /* effect A: frame, of FX_NONE            */
    unsigned char fxa_color;    /*           kleur (COLOR_...)            */
    signed   char fxa_dx;       /*           pixels naast de cowboy       */
    signed   char fxa_dy;       /*           pixels hoger(-)/lager(+)     */
    unsigned char fxb_frame;    /* effect B: idem                         */
    unsigned char fxb_color;
    signed   char fxb_dx;
    signed   char fxb_dy;
    unsigned char ticks;        /* duur in beelden (50 = ~1 seconde)      */
    unsigned char cow;          /* houding van de cowboy, zie COW_...      */
} anim_step;

/* --- houding van de cowboy tijdens een stap --------------------------
 * Meestal staat hij gewoon stil in zijn rusthouding en gebruik je COW_KEEP
 * (dan verandert er niets). Wil je hem laten meedraaien -- bijvoorbeeld naar
 * links kijken door een verrekijker -- kies dan COW_LEFT of COW_RIGHT. Dat
 * zijn de eerste plaatjes uit de loop-animaties.
 */
#define COW_RIGHT       0       /* kijkt naar rechts                       */
#define COW_LEFT        1       /* kijkt naar links                        */
#define COW_DOWN        2       /* kijkt naar de kijker toe                */
#define COW_UP          3       /* met de rug naar je toe                  */
#define COW_IDLE        0xFE    /* de rusthouding (sprite4)                */
#define COW_KEEP        0xFF    /* laat staan zoals het is                 */

/* --- waar speelt de animatie zich af? -------------------------------- */
/* ANIM_FOLLOW: naast de cowboy. De dx/dy van elke stap zijn dan het aantal
 *   pixels vanaf hem, en hij blijft stilstaan zolang het duurt -- hij is
 *   immers degene die het doet (zoals het kampvuur).
 *
 * ANIM_WORLD: op een eigen plek in de wereld, met een eigen snelheid. De
 *   animatie begint op (start_x, start_y) en schuift elk beeld vx/vy pixels
 *   op; de dx/dy van elke stap zijn dan offsets vanaf DAT punt. De cowboy
 *   trekt zich er niets van aan en loopt gewoon door (zoals de haai).
 *
 * ANIM_XEXP_A/B rekt een effect-sprite 2x uit in de breedte: 48 pixels in
 *   plaats van 24. Handig voor brede dingen zoals een kielzog.
 */
/* ANIM_AT_SHORE: de animatie kan alleen aan het water. Vraag je hem aan
 *   terwijl de cowboy midden op het eiland staat, dan loopt hij eerst naar
 *   de waterkant en begint hij daar. ANIM_TO_LEFT zegt naar welke kant de
 *   animatie speelt, zodat hij de juiste oever opzoekt.
 *
 * mirror: het nummer van de gespiegelde tegenhanger in animations[], of
 *   ANIM_NO_MIRROR. Ligt de andere oever dichterbij, dan pakt hij die.
 */
#define ANIM_FOLLOW     0x00
#define ANIM_WORLD      0x01
#define ANIM_XEXP_A     0x02
#define ANIM_XEXP_B     0x04
#define ANIM_AT_SHORE   0x08
#define ANIM_TO_LEFT    0x10
#define ANIM_NO_MIRROR  0xFF

/* --- een complete animatie ------------------------------------------ */
typedef struct {
    const anim_step *steps;
    unsigned char    count;     /* aantal stappen                         */
    unsigned char    mcolor;    /* bit0: A multicolor, bit1: B multicolor */
    unsigned char    key_row;   /* sneltoets: waarde voor $DC00, 0 = geen */
    unsigned char    key_bit;   /* sneltoets: bitmasker voor $DC01        */
    unsigned char    flags;     /* ANIM_FOLLOW / ANIM_WORLD / ANIM_XEXP_* */
    unsigned int     start_x;   /* startpunt   (alleen bij ANIM_WORLD)    */
    unsigned char    start_y;
    signed   char    vx;        /* pixels per beeld (alleen bij WORLD)    */
    signed   char    vy;
    unsigned char    mirror;    /* gespiegelde tegenhanger, of ANIM_NO_MIRROR */
} animation;

/* --- sneltoetsen ----------------------------------------------------- */
/* De C64 leest zijn toetsenbord als een matrix: je schrijft een rij naar
 * $DC00 en leest de kolommen terug uit $DC01 (een 0-bit = ingedrukt).
 * Hieronder staan alle lettertoetsen al uitgewerkt, dus je hoeft niets
 * op te zoeken: gebruik KEYROW_x en KEYBIT_x in de animations[]-tabel.
 * Wil je geen sneltoets, vul dan KEY_GEEN, KEY_GEEN in.
 */
#define KEY_GEEN     0x00

#define KEYROW_A     0xFD
#define KEYBIT_A     0x04
#define KEYROW_B     0xF7
#define KEYBIT_B     0x10
#define KEYROW_C     0xFB
#define KEYBIT_C     0x10
#define KEYROW_D     0xFB
#define KEYBIT_D     0x04
#define KEYROW_E     0xFD
#define KEYBIT_E     0x40
#define KEYROW_F     0xFB
#define KEYBIT_F     0x20
#define KEYROW_G     0xF7
#define KEYBIT_G     0x04
#define KEYROW_H     0xF7
#define KEYBIT_H     0x20
#define KEYROW_I     0xEF
#define KEYBIT_I     0x02
#define KEYROW_J     0xEF
#define KEYBIT_J     0x04
#define KEYROW_K     0xEF
#define KEYBIT_K     0x20
#define KEYROW_L     0xDF
#define KEYBIT_L     0x04
#define KEYROW_M     0xEF
#define KEYBIT_M     0x10
#define KEYROW_N     0xEF
#define KEYBIT_N     0x80
#define KEYROW_O     0xEF
#define KEYBIT_O     0x40
#define KEYROW_P     0xDF
#define KEYBIT_P     0x02
#define KEYROW_Q     0x7F
#define KEYBIT_Q     0x40
#define KEYROW_R     0xFB
#define KEYBIT_R     0x02
#define KEYROW_S     0xFD
#define KEYBIT_S     0x20
#define KEYROW_T     0xFB
#define KEYBIT_T     0x40
#define KEYROW_U     0xF7
#define KEYBIT_U     0x40
#define KEYROW_V     0xF7
#define KEYBIT_V     0x80
#define KEYROW_W     0xFD
#define KEYBIT_W     0x02
#define KEYROW_X     0xFB
#define KEYBIT_X     0x80
#define KEYROW_Y     0xF7
#define KEYBIT_Y     0x02
#define KEYROW_Z     0xFD
#define KEYBIT_Z     0x10
#define KEYROW_SPACE 0x7F
#define KEYBIT_SPACE 0x10

extern const unsigned char fx_frames[FX_FRAME_COUNT][64];
extern const animation     animations[];
extern const unsigned char animation_count;

#endif /* ANIMS_H */
