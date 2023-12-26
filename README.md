QR and Barcode library `libqrean`
----

# What's this?

A portable QR and Barcode generation / manipulation library written in C.

![animation](https://github.com/kikuchan/libqrean/assets/445223/1fdf9b2f-df63-4c0e-8cd6-a72633f2abc3)
![rmqr-animation](https://github.com/kikuchan/libqrean/assets/445223/e56d97b3-24a3-44d2-8ee3-d0f4339c6157)

# CLI Usage
```sh
% qrean Hello
...
% qrean -t mQR Hello > mqr.png
% qrean -o rmqr.png -t rMQR Hello 
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

# Why?

As you can see, you can take control.
I love QR and Barcodes.

The core doesn't rely on `malloc()` for portability. It uses a stack instead of a heap, but you can still use `new_qrean` for example if you prefer.

You can also configure a callback to draw a pixel directly on a screen for example.

# Supported codes

| type                | encode | decode | detect
|---------------------|--------|--------|-----------
| QR                  | ✓      | ✓      | ✓ (See [examples/detect.c](examples/detect.c))
| mQR                 | ✓      | ✓      | ✓ (See [examples/detect-mqr.c](examples/detect-mqr.c))
| rMQR                | ✓      | ✓      | ✓ (See [examples/detect-rmqr.c](examples/detect-rmqr.c))
| UPCA / EAN13 / EAN8 | ✓      | -      | -
| CODE39              | ✓      | -      | -
| CODE93              | ✓      | -      | -
| NW7                 | ✓      | -      | -
| ITF                 | ✓      | -      | -

