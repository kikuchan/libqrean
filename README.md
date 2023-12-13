QR and Barcode library `libqrean`
----

# What's this?

A portable QR encoder / decoder and Barcode encoder with a small footprint.

# Examples
```c
qrmatrix_t qr = create_qrmatrix(QR_VERSION_5, QR_ERRORLEVEL_M, QR_MASKPATTERN_0);
qrmatrix_put_string(&qr, "Hello, world");

size_t len = qrmatrix_get_bitmap(&qr, buffer, sizeof(buffer), 8);
fwrite(buffer, 1, len, stdout);
```

```c
qrmatrix_put_pixel(&qr, qr.symbol_size - 1, qr.symbol_size - 1, 0);
qrmatrix_fix_errors(&qr);

qrmatrix_get_string(&qr, buffer, sizeof(buffer));
printf("%s\n", buffer);
```

```c
qrmatrix_put_finder_pattern(qr);
qrmatrix_put_alignment_pattern(qr);
qrmatrix_put_timing_pattern(qr);
qrmatrix_put_format_info(qr);
qrmatrix_put_version_info(qr);
```

# Why?

As you can see, you can take control.
I love QR and Barcodes.

It doesn't rely on `malloc()` for portability. It uses a stack instead of a heap, but you can still use `new_qrmatrix`, for example, if you prefer.

You can also configure a callback to draw a pixel directly on a screen for example.
