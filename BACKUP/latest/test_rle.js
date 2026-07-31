const fs = require('fs');

function rle(buf) {
    const out = [];
    let i = 0;
    while (i < buf.length) {
        let j = i + 1;
        while (j < buf.length && buf[j] === buf[i] && (j - i) < 128) ++j;
        const run = j - i;
        if (run >= 3) {
            out.push(257 - run, buf[i]);
            i = j;
            continue;
        }

        let k = i;
        while (k < buf.length) {
            let r = k + 1;
            while (r < buf.length && buf[r] === buf[k] && (r - k) < 128) ++r;
            if ((r - k) >= 3) break;
            if ((k - i) >= 127) break;
            k = r;
        }
        const lit = k - i;
        out.push(lit - 1);
        for (let n = 0; n < lit; ++n) out.push(buf[i + n]);
        i = k;
    }
    return Buffer.from(out);
}

function unpack(src, len) {
    const dst = Buffer.alloc(len);
    let si = 0;
    let di = 0;
    while (di < len) {
        let n = src[si++];
        if (n < 128) {
            ++n;
            while (n--) dst[di++] = src[si++];
        } else {
            n = (257 - n) & 0xFF;
            const v = src[si++];
            while (n--) dst[di++] = v;
        }
    }
    return dst;
}

function testBuf(name, buf) {
    const packed = rle(buf);
    const unpacked = unpack(packed, buf.length);
    if (Buffer.compare(buf, unpacked) === 0) {
        console.log(`${name}: OK  raw=${buf.length} rle=${packed.length}`);
    } else {
        console.log(`${name}: FAIL raw=${buf.length} rle=${packed.length}`);
        process.exit(1);
    }
}

// Random-ish buffers
for (let t = 0; t < 10; ++t) {
    const b = Buffer.alloc(1000);
    for (let i = 0; i < b.length; ++i) b[i] = (i * 17 + t * 31) % 256;
    testBuf(`random-${t}`, b);
}

// Solid buffers
for (let v = 0; v < 256; v += 51) {
    testBuf(`solid-${v}`, Buffer.alloc(1000, v));
}

// Small buffers
for (let len = 1; len < 300; ++len) {
    const b = Buffer.alloc(len);
    for (let i = 0; i < len; ++i) b[i] = (i % 5) * 50;
    testBuf(`small-${len}`, b);
}

console.log('all tests passed');
