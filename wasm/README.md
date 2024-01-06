# QR and Barcode library libqrean for web by WebAssembly

## demo

- https://kikuchan.github.io/libqrean/wasm/

## usage

```js
import { Qrean } from "https://kikuchan.github.io/dist/Qrean.js";

const qrean = await Qrean.create();
const imgd = qrean.make("test", Qrean.CODE_TYPE_MQR);
console.log(imgd);
```

## build

Setup clang and deno, then
```sh
make
```
