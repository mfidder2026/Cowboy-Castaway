# AI Developer — CC65 / Commodore 64 Startbestand

> **Doel:** Dit bestand is jouw complete werkgeheugen. Lees dit één keer bij de start.
> Hierna weet je waar alles staat, hoe je C-code voor de C64 compileert, test en start in de emulator.
> Je hoeft **niets anders** in te lezen om aan de slag te gaan.

---

## 0. GOUDEN REGEL — Geheugen sparen (LEES DIT EERST)

Er is **te weinig geheugen** om de hele toolkit in te lezen. Houd je daarom strikt aan:

- ❌ **NOOIT** in bulk inlezen: de map `html/` (57 docs), álle `samples/`, álle `include/`-headers, of hele `.lib`-bestanden.
- ❌ Niet "even alles scannen om context te krijgen." De map hieronder ís je context.
- ✅ Lees een sample of doc **alleen** als een concrete taak erom vraagt, en dan **één specifiek bestand** (zie §9).
- ✅ Voor C-functies: gebruik de kennis in §6 + §7. Pas als iets écht onduidelijk is → open één header of één doc.
- ✅ Twijfel je of je iets mag inlezen? Antwoord dan: nee, tenzij de taak het direct vereist.

---

## 1. Wat is dit project?

We schrijven programma's in **C** (en soms 6502-assembly) voor de **Commodore 64**, met **CC65** als cross-compiler op Windows. De compiler zet C om naar echte 6502-machinecode (`.prg`). We testen in de **VICE**-emulator (`x64sc.exe`).

Platform: **Windows**, alles staat op de **D:-schijf** onder `D:\dev\cc65`.

---

## 2. Mappenstructuur — WAAR STAAT WAT

```
D:\dev\cc65\                          ← projectroot
├── cc65\                             ← de CC65 cross-compiler toolkit
│   ├── bin\                          ← alle .exe tools (compiler, linker, ...)  ⭐
│   ├── include\                      ← C-headers (.h) — o.a. c64.h, conio.h, stdio.h
│   │   ├── geos\  joystick\  mouse\  sys\  tgi\  em\  arpa\
│   ├── lib\                          ← libraries — voor C64: c64.lib  ⭐
│   ├── cfg\                          ← linker-configs — voor C64: c64.cfg
│   ├── target\c64\drv\               ← drivers (joy/mou/tgi/emd/ser) als losse bestanden
│   ├── samples\                      ← voorbeeldcode (NIET bulk inlezen!)
│   │   ├── cbm\                      ← C64/CBM-voorbeelden: fire.c, plasma.c, hello-asm.s
│   │   └── tutorial\                 ← hello.c (simpelste voorbeeld)
│   └── html\                         ← documentatie (NIET bulk inlezen! zie §9)
│
├── vice\                             ← VICE emulator
│   ├── bin\                          ← emulator-executables  ⭐
│   │   ├── x64sc.exe                 ← ⭐ accurate C64-emulator (GEBRUIK DEZE)
│   │   ├── x64dtv.exe  x128.exe      ← andere machines (meestal niet nodig)
│   │   ├── c1541.exe                 ← disk-image tool (.d64 maken/vullen)
│   │   └── petcat.exe                ← BASIC tokenizer/detokenizer
│   └── C64\                          ← C64 ROM's (basic/kernal/chargen) + paletten
│
└── nodejs\                           ← Node.js (node.exe, npm) — voor build-scripts/tooling
    └── install_tools.bat             ← setup-script (alleen draaien bij installatie)
```

⭐ = de mappen die je in de praktijk het vaakst nodig hebt.

**Handige omgevingsvariabelen om in te stellen (of paden voluit gebruiken):**
```
CC65_BIN = D:\dev\cc65\cc65\bin
CC65_HOME= D:\dev\cc65\cc65            (zodat cl65 include/, lib/, cfg/ zelf vindt)
VICE_BIN = D:\dev\cc65\vice\bin
```
> Tip: als `CC65_HOME` gezet is, vindt `cl65` de headers, libs en config's automatisch. Anders werkt het vaak ook al, omdat de tools relatief t.o.v. `bin\` zoeken.

---

## 3. De toolchain — welk tool waarvoor

Alle tools staan in `D:\dev\cc65\cc65\bin\`.

| Tool | Waarvoor |
|------|----------|
| **`cl65.exe`** | ⭐ **Alles-in-één**: compileert C → assembleert → linkt → klaar `.prg`. Gebruik dit standaard. |
| `cc65.exe` | Alleen C → assembly (`.s`). Zelden los nodig. |
| `ca65.exe` | Assembler: `.s` → object (`.o`). |
| `ld65.exe` | Linker: objects + lib → `.prg`. |
| `ar65.exe` | Library-beheer (`.lib` maken/bewerken). |
| `od65.exe` | Object-bestanden inspecteren. |
| `da65.exe` | Disassembler. |

**Vuistregel:** voor gewone C-programma's gebruik je **alleen `cl65`**.

---

## 4. Compileren — HET GOUDEN COMMANDO

Standaard C-programma voor de C64 bouwen:

```bat
cl65 -O -t c64 -o hello.prg hello.c
```

Uitleg van de vlaggen:
- `-t c64` → doelplatform Commodore 64 (kiest automatisch `c64.lib` + `c64.cfg` + zet de BASIC-start-stub `10 SYS 2061` voor je klaar).
- `-O` → optimalisatie aan (kleinere/snellere code).
- `-o hello.prg` → naam van het uitvoerbestand.
- `hello.c` → je bronbestand (meerdere `.c`-bestanden mag: gewoon achter elkaar zetten).

**Meerdere bronbestanden:**
```bat
cl65 -O -t c64 -o game.prg main.c sprites.c sound.c
```

**Assembly meelinken:**
```bat
cl65 -O -t c64 -o game.prg main.c fastcode.s
```

**Handige extra vlaggen (alleen indien nodig):**
- `-m game.map` → genereer een map-file (geheugenoverzicht; handig bij "past het nog?").
- `-Ln labels.lbl` → labels exporteren voor debugging in VICE monitor.
- `--config <pad>\c64.cfg` → eigen linker-config (custom geheugenlayout).
- `-g` → debug-info meenemen.

> **Let op:** een C64 `.prg` begint met 2 bytes laadadres. Bij `-t c64` is dat standaard `$0801` (BASIC-start), zodat het programma met `RUN` start. Dit regelt `cl65` automatisch — niet handmatig aanpassen tenzij je weet wat je doet.

---

## 5. Testen & starten in de emulator (VICE)

Emulator: `D:\dev\cc65\vice\bin\x64sc.exe` (accuraat; gebruik deze, niet het snellere maar minder nauwkeurige `x64.exe`).

**Programma direct autostarten (meest gebruikt):**
```bat
D:\dev\cc65\vice\bin\x64sc.exe -autostart hello.prg
```
Dit laadt én runt het programma meteen.

**Programma alleen laden (zelf `RUN` typen):**
```bat
D:\dev\cc65\vice\bin\x64sc.exe hello.prg
```

**Nuttige VICE-opties:**
- `-autostart <file>` → laden + starten.
- `-warp` → op volle snelheid draaien (handig voor snelle tests/rekenwerk).
- `-VICIIborders 0` → schermrand verbergen.
- `+confirmonexit` → niet vragen bij afsluiten.
- Monitor openen in VICE: **Alt+M** (of menu) → machinecode debuggen, geheugen bekijken.

**Snelle test-loop (compileren + starten in één regel):**
```bat
cl65 -O -t c64 -o hello.prg hello.c && D:\dev\cc65\vice\bin\x64sc.exe -autostart hello.prg
```

---

## 6. C64-C in het kort — hoe de taal hier werkt

CC65 is grotendeels **standaard C**, maar je draait op een 8-bit machine met 64 KB. Belangrijkste aandachtspunten:

**Geheugen & performance**
- Totaal 64 KB RAM; voor je programma is grofweg **~38 KB** vrij onder BASIC (adres `$0801`–`$9FFF`). Meer kan met bank-switching, maar begin simpel.
- De **stack is klein** — vermijd diepe recursie en enorme lokale arrays.
- `int` is **16-bit** (−32768..32767). `char` is standaard **unsigned** (0..255).
- ⭐ Gebruik `unsigned char` voor lus-tellers en waarden 0–255 → dat is veruit het snelst op de 6502. Vermijd `int`/`long` waar het niet hoeft.
- **Floating point** en `printf("%f")` bestaan hier niet/nauwelijks — vermijd floats, gebruik integer-rekenwerk.
- Elke `printf`/`stdio` sleept veel code mee → voor schermuitvoer liever `conio.h` (zie onder).

**Belangrijkste headers (in `include\`)**
| Header | Waarvoor |
|--------|----------|
| `conio.h` | ⭐ Scherm/tekst: `clrscr()`, `gotoxy()`, `cputs()`, `cputc()`, `textcolor()`, `bordercolor()`, `bgcolor()`, `cgetc()`. Dé manier voor tekstuitvoer op C64. |
| `c64.h` | C64-specifiek: geheugenadressen, VIC-II/SID registers, kleuren-constanten (`COLOR_BLACK`…), `#define`s voor hardware. |
| `peekpoke.h` | `POKE(addr,val)` / `PEEK(addr)` — direct geheugen lezen/schrijven. |
| `stdlib.h` | `malloc`, `rand`, `srand`, conversies. |
| `string.h` | `memcpy`, `memset`, `strcpy`, ... |
| `stdio.h` | `printf` e.d. — werkt, maar zwaar; liever `conio`. |
| `joystick.h` | Joystick uitlezen (driver nodig, zie §8). |
| `cbm.h` | Commodore KERNAL-routines, bestands-I/O op disk. |
| `6502.h` | Inline hardware/interrupt-zaken. |

**Minimaal patroon voor directe hardware:**
```c
#include <peekpoke.h>
POKE(0xd020, 0);   /* schermrand zwart  (VIC-II border) */
POKE(0xd021, 6);   /* achtergrond blauw */
```

**Kleuren (0–15):** 0=zwart 1=wit 2=rood 3=cyaan 4=paars 5=groen 6=blauw 7=geel 8=oranje 9=bruin 10=lichtrood 11=donkergrijs 12=grijs 13=lichtgroen 14=lichtblauw 15=lichtgrijs.

---

## 7. Werkend "Hello World" (kopieer-klaar)

`hello.c`:
```c
#include <conio.h>

void main(void) {
    bordercolor(0);   /* zwarte rand */
    bgcolor(0);       /* zwarte achtergrond */
    textcolor(5);     /* groene tekst */
    clrscr();
    gotoxy(0, 0);
    cputs("HELLO FROM CC65 ON THE C64!");
    cgetc();          /* wacht op toets */
}
```
Bouwen + starten:
```bat
cl65 -O -t c64 -o hello.prg hello.c && D:\dev\cc65\vice\bin\x64sc.exe -autostart hello.prg
```

---

## 8. Disk-images & drivers (alleen als een taak het vraagt)

**`.d64`-diskimage maken en een `.prg` erop zetten** (met `c1541.exe`):
```bat
D:\dev\cc65\vice\bin\c1541.exe -format "mijndisk,id" d64 mijndisk.d64 -write hello.prg hello
D:\dev\cc65\vice\bin\x64sc.exe -autostart mijndisk.d64
```

**Drivers** (joystick, muis, grafisch/TGI, extra geheugen) staan als losse bestanden in
`D:\dev\cc65\cc65\target\c64\drv\`. Bijvoorbeeld joystick: `joy\c64-stdjoy.joy`.
Je laadt ze op runtime met de bijbehorende `*_load_driver()`-functie **of** linkt ze statisch in. Pak de details er pas bij als een concrete taak joystick/muis/graphics nodig heeft — lees dan de **één** relevante doc (§9), niet alles.

---

## 9. Documentatie-index — lees NOOIT in bulk

Alle docs staan in `D:\dev\cc65\cc65\html\`. Open **hooguit één** bestand, en alleen als de taak het echt vereist:

| Als je nodig hebt... | Open dan (en niets anders) |
|----------------------|----------------------------|
| C64-specifieke functies/geheugenlayout | `c64.html` |
| Overzicht/gebruik van de C-compiler + vlaggen | `cc65.html` |
| Complete C-functiereferentie (groot!) | `funcref.html` — zoek gericht, lees niet volledig |
| Efficiënt/optimaal coderen voor 6502 | `coding.html` |
| `cl65` alles-in-één opties | `cl65.html` |
| Linker & geheugenlayout / `.cfg` aanpassen | `ld65.html` + `customizing.html` |
| Assembler-syntax (ca65) | `ca65.html` (groot!) |
| Grafische TGI-library | `tgi.html` |
| Debuggen | `debugging.html` |
| Algemene intro | `intro.html` |

**Voorbeeldcode** in `samples\cbm\` (`fire.c`, `plasma.c`) en `samples\tutorial\hello.c`: open er **maximaal één**, alleen als een taak echt om een voorbeeld vraagt.

---

## 10. Standaard werkwijze per taak (checklist)

1. Schrijf/wijzig de `.c` (en eventueel `.s`) bestanden.
2. Bouw met het gouden commando: `cl65 -O -t c64 -o <naam>.prg <bronnen>`.
3. Los compile-/linkfouten op (denk aan: `int` is 16-bit, kleine stack, `unsigned char` voor snelheid).
4. Test: `x64sc.exe -autostart <naam>.prg`.
5. Werkt het? Klaar. Werkt het niet? Debug (VICE-monitor, of `-m map`-file bij geheugenproblemen).
6. Iets nieuws/handigs geleerd? → **Noteer het in §11.**

---

## 11. Geleerde lessen (BIJWERKEN door de AI Developer)

> **Instructie aan jou (AI Developer):** kom je een valkuil, truc, werkend commando of C64-eigenaardigheid tegen die de volgende sessie tijd bespaart? Voeg hem hier toe als kort, concreet bullet-punt met datum. Houd het bondig (één regel per les) zodat dit bestand efficiënt blijft. Verwijder verouderde/foute lessen.

<!-- Format: - [JJJJ-MM-DD] Korte, concrete les. -->

- [2026-07-22] Startbestand aangemaakt. Gouden build-commando: `cl65 -O -t c64 -o out.prg in.c`. Emulator: `x64sc.exe -autostart out.prg`.
- _(voeg hieronder nieuwe lessen toe)_
