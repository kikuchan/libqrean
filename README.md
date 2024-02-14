QR and Barcode library `libqrean`
----

# What's this?

A portable QR and Barcode generation / manipulation library written in **C**.

![animation](https://github.com/kikuchan/libqrean/assets/445223/1fdf9b2f-df63-4c0e-8cd6-a72633f2abc3)
![rmqr-animation](https://github.com/kikuchan/libqrean/assets/445223/e56d97b3-24a3-44d2-8ee3-d0f4339c6157)

# DEMO

[Wasm](https://kikuchan.github.io/libqrean/wasm/) (thanks @taisukef)


# CLI Usage
## Generation
```
% qrean -t rmqr Hello
███████████████████████████████
██ ▄▄▄▄▄ █ ▀ █ █ █▄█ █▄█▄█▄▄ ██
██ █   █ ██▄ ▄█▀▄▄  ▄▄   ▀█ ▄██
██ █▄▄▄█ ██▀▀▀▄█▄▄█▀█▄   █▀▀▄██
██▄▄▄▄▄▄▄█▄██ ▄ ▀▄█▀▀█▀▄ ▄▄▄ ██
██ ▄▀▀ ▀▄ ▄█▄███▀▀▄▄▀▄▀▄ █▄█ ██
██▄▄▄█▄█▄█▄█▄█▄█▄█▄█▄█▄█▄▄▄▄▄██
▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
% qrean -t mQR Hello > mqr.png     # CAVEAT: Outputs PNG stream for not a tty
% qrean -o qr.png Hello            # You can also specify output filename
% qrean -h
Usage: qrean [OPTION]... [STRING]
Generate QR/Barcode image

  General options:
    -h                Display this help
    -o FILENAME       Image save as FILENAME
    -i FILENAME       Data read from FILENAME
    -f FORMAT         Output format, one of the following:
                          TXT    (default for tty)
                          PNG    (default for non tty)
                          PPM
    -t CODE           Output CODE, one of the following:
                          QR, mQR, rMQR, tQR
                          EAN13, EAN8, UPCA
                          CODE39, CODE93
                          ITF, NW7
    -p PADDING        Comma-separated PADDING for the code

  QR family specific options:
    -v VERSION        Use VERSION, one of the following:
                          1 ... 40           (for QR)
                          M1 ... M4          (for mQR)
                          R7x43 ... R17x139  (for rMQR)
                            R{H}x{W}
                              H: 7, 9, 11, 13, 15, 17
                              W: 43, 59, 77, 99, 139
                          tQR                (for tQR)
    -m MASK           Use MASK pattern, one of the following:
                          0 ... 7            (for QR)
                          0 ... 4            (for mQR)
    -l LEVEL          Use ecc LEVEL, one of the following:
                          L, M, Q, H         (for QR)
                          L, M, Q            (for mQR)
                             M,    H         (for rMQR)
```
## Detection
```
% qrean-detect qr.png   # The source file must be a PNG file so far
Hello
%
```

# Library Usage
```c
qrean_t qrean = create_qrean(QREAN_CODE_TYPE_QR);
qrean_write_string(&qrean, "Hello, world", QREAN_DATA_TYPE_AUTO);

size_t len = qrean_read_bitmap(&qrean, buffer, sizeof(buffer), 8);
fwrite(buffer, 1, len, stdout);
```

```c
qrean_write_pixel(&qrean, qrean.canvas.symbol_width - 1, qrean.canvas.symbol_height - 1, 0);
qrean_fix_errors(&qrean);

qrean_read_string(&qrean, buffer, sizeof(buffer));
printf("%s\n", buffer);
```

```c
qrean_t *qrean = new_qrean(QREAN_CODE_TYPE_QR);
qrean_set_qr_version(qrean, QR_VERSION_3);
qrean_set_qr_errorlevel(qrean, QR_ERRORLEVEL_M);
qrean_set_qr_maskpattern(qrean, QR_MASKPATTERN_7);

qrean_write_qr_finder_pattern(qrean);
qrean_write_qr_alignment_pattern(qrean);
qrean_write_qr_timing_pattern(qrean);
qrean_write_qr_format_info(qrean);
qrean_write_qr_version_info(qrean);
```

See [examples/](examples/) directory for working examples.


# JavaScript / TypeScript Usage (Powered by Wasm)

```js
import { Qrean } from 'qrean';

const qrean = new Qrean();
const img = await qrean.encode('Hello, world');
console.log(img);
// {
//   width: 116,
//   height: 116,
//   data: Uint8ClampedArray(53824) [
//    :
```

It requires Wasm to run, but don't worry, most engines support it; Node, Deno, Bun and browsers.

# Install

## As a CLI

* Windows binaries are directly placed on [dist/](dist/) (for now)
* Just `make install` for UNIX-like systems (Linux / macOS / *BSD)

## As a JavaScript / TypeScript library

```sh
% npm install qrean
```

## As a C library for embedded systems

Take a look at [src/](src/) and include (or copy) them!

# Why?

As you can see, you can take control.
I love QR and Barcodes.

The core (encode/decode) doesn't rely on `malloc()` for portability. It uses a stack instead of a heap, but you can still use `new_qrean` for example if you prefer.

You can also configure a callback to draw a pixel directly on a screen for example.

# Supported codes

| type                | encode | decode | detect
|---------------------|--------|--------|-----------
| QR                  | ✓      | ✓      | ✓
| mQR                 | ✓      | ✓      | ✓
| rMQR                | ✓      | ✓      | ✓
| tQR                 | ✓      | ✓      | ✓
| UPCA / EAN13 / EAN8 | ✓      | ✓      | ✓
| CODE39              | ✓      | ✓      | ✓
| CODE93              | ✓      | ✓      | ✓
| NW7                 | ✓      | ✓      | ✓
| ITF                 | ✓      | ✓      | ✓

