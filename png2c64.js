const fs = require('fs');
const PNG = require('./node_modules/pngjs').PNG;

// --- configuration ---
const inFile = process.argv[2] || 'sprites/intro.png';
const outFile = process.argv[3] || 'intro_gfx.c';

// C64 palette (RGB tuples). Order matches C64 color index 0..15.
const pal = [
    [0,0,0],        // 0 black
    [255,255,255],  // 1 white
    [136,0,0],      // 2 red
    [170,255,238],  // 3 cyan
    [204,68,204],   // 4 purple
    [0,204,85],     // 5 green
    [0,0,170],      // 6 blue
    [238,238,119],  // 7 yellow
    [221,136,85],   // 8 orange
    [102,68,0],     // 9 brown
    [255,119,119],  // 10 light red
    [51,51,51],     // 11 dark grey
    [119,119,119],  // 12 grey
    [170,255,102],  // 13 light green
    [0,136,255],    // 14 light blue
    [187,187,187],  // 15 light grey
];

function nearestC64(r, g, b) {
    let best = 0;
    let bestDist = 1e9;
    for (let i = 0; i < pal.length; ++i) {
        const dr = r - pal[i][0];
        const dg = g - pal[i][1];
        const db = b - pal[i][2];
        const d = dr*dr + dg*dg + db*db;
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

function readPNG(path) {
    const data = fs.readFileSync(path);
    const png = PNG.sync.read(data);
    return { width: png.width, height: png.height, data: png.data };
}

// Encode a 320x200 multicolor bitmap screen.
// Output layout:
//   bitmap:  8000 bytes (C64 multicolor bitmap order)
//   screen:  1000 bytes (color0/1 per 4x8 cell)
//   color:   1000 bytes (color2/3 per 4x8 cell)
function encodeMulticolor(img) {
    if (img.width !== 320 || img.height !== 200) {
        throw new Error(`Image must be 320x200, got ${img.width}x${img.height}`);
    }

    const bitmap = Buffer.alloc(8000);
    const screen = Buffer.alloc(1000);
    const color  = Buffer.alloc(1000);
    let globalBgCount = new Array(16).fill(0);

    for (let cy = 0; cy < 25; ++cy) {
        for (let cx = 0; cx < 40; ++cx) {
            // Determine the 4 colors used in this 4x8 cell.
            const counts = new Array(16).fill(0);
            for (let y = 0; y < 8; ++y) {
                for (let x = 0; x < 4; ++x) {
                    const px = cx * 4 + x;
                    const py = cy * 8 + y;
                    const idx = (py * img.width + px) * 4;
                    const c = nearestC64(img.data[idx], img.data[idx+1], img.data[idx+2]);
                    counts[c]++;
                    globalBgCount[c]++;
                }
            }

            // Find most frequent color overall as background candidate.
            let bg = 0;
            for (let i = 1; i < 16; ++i) if (counts[i] > counts[bg]) bg = i;

            // Find top two remaining colors.
            const rest = [];
            for (let i = 0; i < 16; ++i) if (i !== bg) rest.push({c:i, n:counts[i]});
            rest.sort((a,b) => b.n - a.n);
            const c1 = rest[0]?.c || 0;
            const c2 = rest[1]?.c || 0;
            const c3 = rest[2]?.c || c2;

            screen[cy*40+cx] = (c1 << 4) | c2;
            /* Color RAM is only 4 bits wide; the VIC reads the lower nibble
               as the color for bit-pair %11. Keep the upper nibble zero. */
            color[cy*40+cx]  = c3 & 0x0F;

            // Encode bitmap bytes for this cell (8 bytes, one per scanline).
            for (let y = 0; y < 8; ++y) {
                let b = 0;
                for (let x = 0; x < 4; ++x) {
                    const px = cx * 4 + x;
                    const py = cy * 8 + y;
                    const idx = (py * img.width + px) * 4;
                    const c = nearestC64(img.data[idx], img.data[idx+1], img.data[idx+2]);
                    let pair;
                    if (c === bg) pair = 0;
                    else if (c === c1) pair = 1;
                    else if (c === c2) pair = 2;
                    else pair = 3;
                    b = (b << 2) | pair;
                }
                const byteOffset = cy*320 + cx*8 + y;
                bitmap[byteOffset] = b;
            }
        }
    }

    let bg = 0;
    for (let i = 1; i < 16; ++i) if (globalBgCount[i] > globalBgCount[bg]) bg = i;

    return { bitmap, screen, color, bg };
}

// RLE that matches the unpack() routine in main.c:
//   n < 128      -> literal: the next n+1 bytes follow literally
//   n >= 129     -> repeat: 257-n times the next byte (2..128 repeats)
function rle(buf) {
    const out = [];
    let i = 0;
    while (i < buf.length) {
        // Try to make a repeat run.
        let j = i + 1;
        while (j < buf.length && buf[j] === buf[i] && (j - i) < 128) ++j;
        const run = j - i;
        if (run >= 3) {
            out.push(257 - run, buf[i]);
            i = j;
            continue;
        }

        // Literal run: gather until we hit a worthwhile repeat or 128 bytes.
        let k = i;
        while (k < buf.length) {
            let r = k + 1;
            while (r < buf.length && buf[r] === buf[k] && (r - k) < 128) ++r;
            if ((r - k) >= 3) break;       // a repeat run starts here
            if ((k - i) >= 127) break;     // literal length would exceed 128
            k = r;
        }
        const lit = k - i;
        out.push(lit - 1);                 // 0..127 means 1..128 bytes
        for (let n = 0; n < lit; ++n) out.push(buf[i + n]);
        i = k;
    }
    return Buffer.from(out);
}

function bytesToC(name, buf) {
    const lines = [];
    for (let i = 0; i < buf.length; i += 16) {
        const chunk = [];
        for (let j = i; j < Math.min(i+16, buf.length); ++j) {
            chunk.push('0x' + buf[j].toString(16).padStart(2,'0').toUpperCase());
        }
        lines.push('    ' + chunk.join(', ') + ',');
    }
    return `const unsigned char ${name}[${buf.length}] = {\n${lines.join('\n')}\n};`;
}

const img = readPNG(inFile);
const enc = encodeMulticolor(img);
const rleBitmap = rle(enc.bitmap);
const rleScreen = rle(enc.screen);
const rleColor  = rle(enc.color);

console.log(`bitmap raw=${enc.bitmap.length} rle=${rleBitmap.length}`);
console.log(`screen raw=${enc.screen.length} rle=${rleScreen.length}`);
console.log(`color  raw=${enc.color.length} rle=${rleColor.length}`);
console.log(`dominant background color index = ${enc.bg}`);

const src = `/*
 * intro_gfx.c  -  Titelscherm gegenereerd uit sprites/intro.png
 * ---------------------------------------------------------------
 * Multicolor bitmap (320x200), ingepakt met RLE.
 */
#define INTRO_BGCOLOR      ${enc.bg}      /* $D021 tijdens het titelscherm */
#define INTRO_BORDERCOLOR  ${enc.bg}      /* rand */

${bytesToC('intro_bitmap_rle', rleBitmap)}

${bytesToC('intro_screen_rle', rleScreen)}

${bytesToC('intro_color_rle', rleColor)}
`;

fs.writeFileSync(outFile, src);
console.log(`Wrote ${outFile}`);
