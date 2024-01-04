# QR and Barcode library libqrean for web by WebAssembly

## demo

- https://taisukef.github.io/libqrean/wasm/

## usage

```js
import { QRean } from "https://taisukef.github.io/libqrean/wasm/QRean.js";

const qrean = await QRean.create();
const imgd = qrean.make("test", QRean.TYPE_MQR, QRean.DATA_TYPE_AUTO);
console.log(imgd);
```

## build

setup clang
```sh
make
```
