# Cowboy Castaway — Controls

## Startup (title screen)

Press **Space** or the **fire button** (joystick in port 1 or 2) to unpack the
island over the title screen and start the program.

## Movement

The cowboy walks with the **cursor (arrow) keys**:

- **Cursor ⇄** → walk **right**
- **Shift + Cursor ⇄** → walk **left**
- **Cursor ↕** → walk **down**
- **Shift + Cursor ↕** → walk **up**

On a diagonal, horizontal wins. Touching an arrow key hands control back to you:
wandering stops, a running animation is cut short, and any dizziness clears.
Release everything and he freezes on the last frame; after ~3 seconds he starts
wandering on his own and occasionally plays a random animation.

> On a real C64 these are the two cursor keys — **Crsr ⇄** and **Crsr ↕** — with
> **Shift** for the reverse direction. Most emulators map them to the PC arrow keys.

## Start animations

- **F** — campfire (**F**ire)
- **S** — shark swims by (**S**hark)
- **C** — fishing (**C**atch)
- **B** — binoculars (**B**inoculars)
- **P** — pirouette
- **K** — coconut falls from the palm — bonks him if he's underneath (**K**okosnoot)
- **M** — message in a bottle washes ashore (**M**essage)
- **J** — the big jump: climb the palm, look around, and back

Two things to know: **F** and **C** automatically pick the nearest shore and play
facing left or right accordingly, which is why the mirrored variants have no key
of their own. And a hotkey only works when nothing else is playing (no running
animation, no jump, not on the way to the water), so a held key doesn't keep
restarting.
