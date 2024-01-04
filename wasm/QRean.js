import wasmbin from "./qrean.wasm.js";

export const setText = (mem, idx, s) => {
  const sbin = new TextEncoder().encode(s + "\0");
  for (let i = 0; i < sbin.length; i++) {
    mem[idx + i] = sbin[i];
  }
};
export const getText = (mem, idx) => {
  for (let i = 0;; i++) {
    if (mem[idx + i] == 0) {
      const bin = new Uint8Array(i);
      for (let j = 0; j < i; j++) {
        bin[j] = mem[idx + j];
      }
      const s = new TextDecoder().decode(bin);
      return s;
    }
  }
};

export class QRean {
  static async create() {
    const memory = new WebAssembly.Memory({
      initial: 256, // 256 pages (64KiB per page)
      maximum: 512, // 512 pages
    });
    const importObject = {
      env: {
        memory,
        atan2: (y, x) => Math.atan2(y, x),
        pow: (n, m) => Math.pow(n, m),
        sin: (th) => Math.sin(th),
        cos: (th) => Math.cos(th),
        fmod: (n, m) => n % m,
        round: (f) => Math.round(f),
        roundf: (f) => Math.round(f),
      },
    };
    const wasm = await WebAssembly.instantiate(wasmbin, importObject);
    return new QRean(wasm);
  }
  constructor(wasm) {
    this.wasm = wasm;
  }
  // type
  static TYPE_QR = 1; // ok
  static TYPE_MQR = 2; // ok
  static TYPE_RMQR = 3; // ok
  static TYPE_TQR = 4;
  static TYPE_EAN13 = 5;
  static TYPE_EAN8 = 6;
  static TYPE_UPCA = 7;
  static TYPE_CODE39 = 8; // ok
  static TYPE_CODE93 = 9; // ok
  static TYPE_NW7 = 10; // ok
  static TYPE_ITF = 11;
  // datatype
  static DATA_TYPE_AUTO = 0;
  static DATA_TYPE_NUMERIC = 1;
  static DATA_TYPE_ALNUM = 2;
  static DATA_TYPE_8BIT = 3;
  static DATA_TYPE_KANJI = 4;

  static TYPES = {
    "QR": QRean.TYPE_QR,
    "mQR": QRean.TYPE_MQR,
    "rMQR": QRean.TYPE_RMQR,
    "tQR": QRean.TYPE_TQR,
    "EAN13": QRean.TYPE_EAN13,
    "EAN8": QRean.TYPE_EAN8,
    "UPCA": QRean.TYPE_UPCA,
    "CODE39": QRean.TYPE_CODE39,
    "CODE93": QRean.TYPE_CODE93,
    "NW7": QRean.TYPE_NW7,
    "ITF": QRean.TYPE_ITF,
  };
  static DATA_TYPES = {
    "auto": QRean.DATA_TYPE_AUTO,
    "numeric": QRean.DATA_TYPE_NUMERIC,
    "alphabate & numeric": QRean.DATA_TYPE_ALNUM,
    "8bit binary": QRean.DATA_TYPE_8BIT,
    "kanji character": QRean.DATA_TYPE_KNAJI,
  };
  make(text, type, datatype) {
    const ex = this.wasm.instance.exports;
    ex.memreset();
    const mem = new Uint8Array(ex.memory.buffer);
    const pinput = ex.getIndexInput();
    setText(mem, pinput, text);
    const pimage = ex.make(type, datatype);
    if (!pimage) {
      return null;
      //throw new Exception(res);
    }
    const pbuf = (mem[pimage + 0] & 0xff) | ((mem[pimage + 1] & 0xff) << 8);
    const width = mem[pimage + 4];
    const height = mem[pimage + 8];
    const data = new Uint8ClampedArray(width * height * 4);
    for (let i = 0; i < data.length; i++) {
      data[i] = mem[pbuf + i];
    }
    for (let i = 0; i < data.length; i += 4) {
      data[i + 3] = 255;
    }
    const imgdata = {
      width,
      height,
      data,
    };
    return imgdata;
  }
}

/*
log("detect: " + ex.detect());

const poutput = ex.getIndexOutput();
console.log(poutput);
const s2 = getText(poutput);
log("detected: " + s2);
*/
