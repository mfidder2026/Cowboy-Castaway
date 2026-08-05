# Cowboy Castaway

A "Cowboy on a Desert Island" demo/game for the **Commodore 64**, written in C using the [`cc65`](https://cc65.github.io/) compiler. Explore the island, trigger animations, and enjoy the retro C64 graphics and sound!

![Cowboy Castaway](https://github.com/mfidder2026/Cowboy-Castaway/blob/main/sprites/intro1_processed.png)

---

## 🎮 Controls

### Startup (Title Screen)
- Press **Space** or the **fire button** (joystick in port 1 or 2) to unpack the island and start the program.

### Movement
Use the **cursor (arrow) keys** to move the cowboy:
- **Cursor ⇄** → Walk **right**
- **Shift + Cursor ⇄** → Walk **left**
- **Cursor ↕** → Walk **down**
- **Shift + Cursor ↕** → Walk **up**

> **Note**: On a real C64, these are the **Crsr ⇄** and **Crsr ↕** keys with **Shift** for reverse direction. Most emulators map them to the PC arrow keys.

### Animations
Trigger animations with the following keys:
- **F** — Campfire (**F**ire)
- **S** — Shark swims by (**S**hark)
- **C** — Fishing (**C**atch)
- **B** — Binoculars (**B**inoculars)
- **P** — Pirouette
- **K** — Coconut falls from the palm (**K**okosnoot)
- **M** — Message in a bottle washes ashore (**M**essage)
- **J** — The big jump: climb the palm, look around, and back

> **Notes**:
> - **F** and **C** automatically pick the nearest shore and play facing left or right.
> - Hotkeys only work when no other animation is playing.

---

## 🛠️ Build Instructions

### Prerequisites
1. **`cc65`** (6502 C compiler) — [Download here](https://cc65.github.io/)
2. **VICE** (C64 emulator) — [Download here](https://vice-emu.sourceforge.io/)
3. **Windows** (for `build.bat`; adapt for other OSes if needed)

### Build Steps
1. Clone this repository or download the source files.
2. Open a terminal in the project directory.
3. Run the build script:
   ```batch
   build.bat
   ```
   - This compiles the source files and launches the program in VICE.
   - The output is [`cowboy.prg`](./cowboy.prg).

### Manual Build
If you prefer to build manually:
```batch
cl65 -O -t c64 main.c cowboy_frames.c island_gfx.c intro_gfx.c anims.c -o cowboy.prg
```

---

## ▶️ Running the Program

### Using VICE
1. Launch the compiled [`cowboy.prg`](./cowboy.prg) in VICE:
   ```batch
   x64sc -autostart cowboy.prg -pal
   ```
   - `-autostart` loads and runs the program automatically.
   - `-pal` ensures 50Hz timing (required for correct animation speed).

### On Real Hardware
Transfer [`cowboy.prg`](./cowboy.prg) to a real C64 using your preferred method (e.g., SD2IEC, 1541 Ultimate, or a floppy disk).

---

## 📁 Project Structure

| File/Directory       | Description                                                                 |
|----------------------|-----------------------------------------------------------------------------|
| [`main.c`](./main.c)          | Main program logic, input handling, and game loop.                          |
| [`anims.c`](./anims.c)        | Animation definitions and logic.                                            |
| [`anims.h`](./anims.h)        | Header file for animations.                                                 |
| [`intro_gfx.c`](./intro_gfx.c)| Graphics for the title screen.                                              |
| [`island_gfx.c`](./island_gfx.c)| Island graphics and collision data.                                         |
| [`cowboy_frames.c`](./cowboy_frames.c)| Sprite data for the cowboy.                                                 |
| [`build.bat`](./build.bat)    | Build script for Windows (compiles and launches in VICE).                  |
| [`controlls.md`](./controlls.md)| Detailed control scheme documentation.                                     |
| `sound/`               | Sound effects and music (if any).                                           |
| `sprites/`             | Sprite graphics (source files, e.g., PNGs).                                 |
| `BACKUP/`              | Automatically generated backups of source files.                            |
| [`cowboy.prg`](./cowboy.prg)  | Compiled executable (output of the build process).                          |

---

## 📜 License
This project is open-source. See [`LICENSE`](./LICENSE) for details. *(Add a license file if missing.)*

---

## 🙌 Credits
- Developed using [`cc65`](https://cc65.github.io/) and VICE.
- Inspired by classic Commodore 64 demos and games.
