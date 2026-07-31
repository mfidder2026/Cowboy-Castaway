// preprocess_intro.js - Voorbewerk een afbeelding voor C64 multicolor-modus
// Gebruik: node preprocess_intro.js <input.png> <output.png>

const fs = require('fs');
const PNG = require('./node_modules/pngjs').PNG;
const { createCanvas, loadImage } = require('./node_modules/canvas');

// C64-palet (RGB)
const c64Palette = [
    [0, 0, 0],        // 0: Zwart
    [255, 255, 255],  // 1: Wit
    [136, 0, 0],      // 2: Rood
    [170, 255, 238],  // 3: Cyaan
    [204, 68, 204],   // 4: Paars
    [0, 204, 85],     // 5: Groen
    [0, 0, 170],      // 6: Blauw
    [238, 238, 119],  // 7: Geel
    [221, 136, 85],   // 8: Oranje
    [102, 68, 0],     // 9: Bruin
    [255, 119, 119],  // 10: Lichtrood
    [51, 51, 51],     // 11: Donkergrijs
    [119, 119, 119],  // 12: Grijs
    [170, 255, 102],  // 13: Lichtgroen
    [0, 136, 255],    // 14: Lichtblauw
    [187, 187, 187]   // 15: Lichtgrijs
];

// Vind de dichtstbijzijnde kleur in het C64-palet
function nearestC64Color(r, g, b) {
    let best = 0;
    let bestDist = 1e9;
    for (let i = 0; i < c64Palette.length; ++i) {
        const dr = r - c64Palette[i][0];
        const dg = g - c64Palette[i][1];
        const db = b - c64Palette[i][2];
        const dist = dr * dr + dg * dg + db * db;
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

// Beperk het aantal kleuren per 4x8 cel tot maximaal 4 (inclusief achtergrond)
function limitColorsPerCell(img) {
    const width = img.width;
    const height = img.height;
    const newImg = new PNG({ width, height });
    const bgColor = [0, 0, 0]; // Zwart als standaard achtergrond

    // Bepaal de meest voorkomende kleur als achtergrondkleur
    const colorCounts = new Array(c64Palette.length).fill(0);
    for (let y = 0; y < height; ++y) {
        for (let x = 0; x < width; ++x) {
            const idx = (y * width + x) * 4;
            const r = img.data[idx];
            const g = img.data[idx + 1];
            const b = img.data[idx + 2];
            const c64Color = nearestC64Color(r, g, b);
            colorCounts[c64Color]++;
        }
    }
    const bgColorIdx = colorCounts.indexOf(Math.max(...colorCounts));
    const bgRgb = c64Palette[bgColorIdx];

    // Loop door elke 4x8 cel en beperk het aantal kleuren
    for (let cy = 0; cy < Math.ceil(height / 8); ++cy) {
        for (let cx = 0; cx < Math.ceil(width / 4); ++cx) {
            const colorsInCell = new Set();
            // Verzamel alle unieke kleuren in deze cel
            for (let y = cy * 8; y < Math.min((cy + 1) * 8, height); ++y) {
                for (let x = cx * 4; x < Math.min((cx + 1) * 4, width); ++x) {
                    const idx = (y * width + x) * 4;
                    const r = img.data[idx];
                    const g = img.data[idx + 1];
                    const b = img.data[idx + 2];
                    const c64Color = nearestC64Color(r, g, b);
                    colorsInCell.add(c64Color);
                }
            }
            // Als er meer dan 4 kleuren zijn, behoud alleen de 3 meest voorkomende (plus achtergrond)
            if (colorsInCell.size > 4) {
                const colorFreq = {};
                for (let y = cy * 8; y < Math.min((cy + 1) * 8, height); ++y) {
                    for (let x = cx * 4; x < Math.min((cx + 1) * 4, width); ++x) {
                        const idx = (y * width + x) * 4;
                        const r = img.data[idx];
                        const g = img.data[idx + 1];
                        const b = img.data[idx + 2];
                        const c64Color = nearestC64Color(r, g, b);
                        if (!colorFreq[c64Color]) colorFreq[c64Color] = 0;
                        colorFreq[c64Color]++;
                    }
                }
                // Sorteer kleuren op frequentie (afgezien van de achtergrond)
                const sortedColors = Object.keys(colorFreq)
                    .map(c => ({ color: parseInt(c), freq: colorFreq[c] }))
                    .filter(c => c.color !== bgColorIdx)
                    .sort((a, b) => b.freq - a.freq)
                    .slice(0, 3);
                const allowedColors = new Set([bgColorIdx, ...sortedColors.map(c => c.color)]);
                // Pas de cel aan
                for (let y = cy * 8; y < Math.min((cy + 1) * 8, height); ++y) {
                    for (let x = cx * 4; x < Math.min((cx + 1) * 4, width); ++x) {
                        const idx = (y * width + x) * 4;
                        const r = img.data[idx];
                        const g = img.data[idx + 1];
                        const b = img.data[idx + 2];
                        const c64Color = nearestC64Color(r, g, b);
                        if (!allowedColors.has(c64Color)) {
                            // Vervang door de dichtstbijzijnde toegestane kleur
                            const newColor = sortedColors.length > 0 ? sortedColors[0].color : bgColorIdx;
                            const newRgb = c64Palette[newColor];
                            newImg.data[idx] = newRgb[0];
                            newImg.data[idx + 1] = newRgb[1];
                            newImg.data[idx + 2] = newRgb[2];
                            newImg.data[idx + 3] = 255; // Alpha
                        } else {
                            newImg.data[idx] = r;
                            newImg.data[idx + 1] = g;
                            newImg.data[idx + 2] = b;
                            newImg.data[idx + 3] = 255;
                        }
                    }
                }
            } else {
                // Kopieer de cel ongewijzigd
                for (let y = cy * 8; y < Math.min((cy + 1) * 8, height); ++y) {
                    for (let x = cx * 4; x < Math.min((cx + 1) * 4, width); ++x) {
                        const idx = (y * width + x) * 4;
                        newImg.data[idx] = img.data[idx];
                        newImg.data[idx + 1] = img.data[idx + 1];
                        newImg.data[idx + 2] = img.data[idx + 2];
                        newImg.data[idx + 3] = 255;
                    }
                }
            }
        }
    }
    return newImg;
}

// Hoofdverwerking
async function main() {
    const inFile = process.argv[2] || 'sprites/intro1.png';
    const outFile = process.argv[3] || 'sprites/intro1_processed.png';

    const img = await loadImage(inFile);
    const canvas = createCanvas(img.width, img.height);
    const ctx = canvas.getContext('2d');
    ctx.drawImage(img, 0, 0);
    const imageData = ctx.getImageData(0, 0, img.width, img.height);
    const png = new PNG({ width: img.width, height: img.height });
    png.data = Buffer.from(imageData.data);

    // Beperk het aantal kleuren per cel
    const processedPng = limitColorsPerCell(png);

    // Sla de voorbewerkte afbeelding op
    const outStream = fs.createWriteStream(outFile);
    processedPng.pack().pipe(outStream);
    outStream.on('finish', () => {
        console.log(`Processed image saved to ${outFile}`);
    });
}

main().catch(err => console.error(err));