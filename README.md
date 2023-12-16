QR and Barcode library `libqrean`
----

# What's this?

A portable QR and Barcode manipulation library with a small footprint.

![animation](https://github.com/kikuchan/libqrean/assets/445223/1fdf9b2f-df63-4c0e-8cd6-a72633f2abc3)

# Examples
```c
qrmatrix_t qr = create_qrmatrix();
qrmatrix_write_string(&qr, "Hello, world");

size_t len = qrmatrix_read_bitmap(&qr, buffer, sizeof(buffer), 8);
fwrite(buffer, 1, len, stdout);
```

```c
qrmatrix_write_pixel(&qr, qr.symbol_size - 1, qr.symbol_size - 1, 0);
qrmatrix_fix_errors(&qr);

qrmatrix_read_string(&qr, buffer, sizeof(buffer));
printf("%s\n", buffer);
```

```c
qrmatrix_t *qr = new_qrmatrix();
qrmatrix_set_version(qr, QR_VERSION_3);
qrmatrix_set_errorlevel(qr, QR_ERRORLEVEL_M);
qrmatrix_set_maskpattern(qr, QR_MASKPATTERN_7);

qrmatrix_write_finder_pattern(qr);
qrmatrix_write_alignment_pattern(qr);
qrmatrix_write_timing_pattern(qr);
qrmatrix_write_format_info(qr);
qrmatrix_write_version_info(qr);
```

See [examples/](examples/) directory for working examples.

# Why?

As you can see, you can take control.
I love QR and Barcodes.

It doesn't rely on `malloc()` for portability. It uses a stack instead of a heap, but you can still use `new_qrmatrix` for example if you prefer.

You can also configure a callback to draw a pixel directly on a screen for example.

# Supported codes

| type                | encode | decode | detect
|---------------------|--------|--------|-----------
| QR                  | ✓      | ✓      | 🚧 (See [examples/detect.c](examples/detect.c))
| UPCA / EAN13 / EAN8 | ✓      | -      | -
| CODE39              | ✓      | -      | -
| CODE93              | ✓      | -      | -
| NW7                 | ✓      | -      | -
| ITF                 | ✓      | -      | -

