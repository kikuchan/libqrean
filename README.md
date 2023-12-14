QR and Barcode library `libqrean`
----

# What's this?

A portable QR and Barcode manipulation library with a small footprint.

# Examples
```c
qrmatrix_t qr = create_qrmatrix_for_string(QR_VERSION_5, QR_ERRORLEVEL_M, QR_MASKPATTERN_0, "Hello, world");

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

# Why?

As you can see, you can take control.
I love QR and Barcodes.

It doesn't rely on `malloc()` for portability. It uses a stack instead of a heap, but you can still use `new_qrmatrix` for example if you prefer.

You can also configure a callback to draw a pixel directly on a screen for example.

# Supported codes

| type                | encode | decode | detection
|---------------------|--------|--------|-----------
| QR                  | ✓      | ✓      | - (not yet)
| UPCA / EAN13 / EAN8 | ✓      | -      | -
| CODE39              | ✓      | -      | -
| CODE93              | ✓      | -      | -
| NW7                 | ✓      | -      | -
| ITF                 | ✓      | -      | -

