import wasmbin from "../build/wasm/qrean.wasm.js";

const toCString = (s: string) => {
  return new TextEncoder().encode(s + "\0");
};

const fromCString = (memory: WebAssembly.Memory, idx: number) => {
  const mem = new Uint8ClampedArray(memory.buffer);
  for (let i = 0;; i++) {
    if (mem[idx + i] == 0) {
      const s = new TextDecoder().decode(mem.slice(idx, idx + i));
      return s;
    }
  }
};

type CreateOptions = {
  pageSize?: number;
  debug?: boolean;
};

type EncodeOptions = {
  codeType?: keyof typeof Qrean.CODE_TYPES;
  dataType?: keyof typeof Qrean.DATA_TYPES;
  qrVersion?: keyof typeof Qrean.QR_VERSIONS;
  qrMaskPattern?: keyof typeof Qrean.QR_MASKPATTERNS;
  qrErrorLevel?: keyof typeof Qrean.QR_ERRORLEVELS;
  scale?: number;
  padding?: number[];
};

type DetectOptions = {
  gamma?: number;
  eciCode?: 'UTF-8' | 'ShiftJIS' | 'Latin1';
  outbufSize?: number;
};

export type Image = {
  width: number;
  height: number;
  data: Uint8ClampedArray;
}

export type Detected = {
  type: keyof typeof Qrean.CODE_TYPES;
  text: string;

  points: number[][];

  qr?: {
    version: keyof typeof Qrean.QR_VERSIONS;
    mask: keyof typeof Qrean.QR_MASKPATTERNS;
    level: keyof typeof Qrean.QR_ERRORLEVELS;
  };
}
export class Qrean {
  private wasm: Promise<WebAssembly.WebAssemblyInstantiatedSource>;
  private on_found?: (obj: Detected) => void;

  private instance!: WebAssembly.Instance;
  private heap!: number;
  private memory!: WebAssembly.Memory;

  private detected: Detected[] = [];

  static async create(opts: CreateOptions = {}) {
    return new Qrean(opts);
  }

  constructor(opts: CreateOptions = {}) {
    const importObject = {
      env: {
        atan2: (y: number, x: number) => Math.atan2(y, x),
        pow: (n: number, m: number) => Math.pow(n, m),
        sin: (th: number) => Math.sin(th),
        cos: (th: number) => Math.cos(th),
        fmod: (n: number, m: number) => n % m,
        round: (f: number) => Math.round(f),
        roundf: (f: number) => Math.round(f),

        on_found: (type: number, ptr: number, version: number, level: number, mask: number, points: number) => {
          const text = fromCString(this.memory, ptr);
          const typestr = (Object.entries(Qrean.CODE_TYPES).find(([_, v]) => v == type)?.[0] ?? 'Unknown') as keyof typeof Qrean.CODE_TYPES;
          const qrVersion = (Object.entries(Qrean.QR_VERSIONS).find(([_, v]) => v == version)?.[0]) as keyof typeof Qrean.QR_VERSIONS | undefined;
          const qrLevel = (Object.entries(Qrean.QR_ERRORLEVELS).find(([_, v]) => v == level)?.[0]) as keyof typeof Qrean.QR_ERRORLEVELS | undefined;
          const qrMask = (Object.entries(Qrean.QR_MASKPATTERNS).find(([_, v]) => v == mask)?.[0]) as keyof typeof Qrean.QR_MASKPATTERNS | undefined;

          const pointsView = new Float32Array(this.memory.buffer, points, 4 * 2);

          const detected: Detected = {
            type: typestr,
            text,
            qr: version >= Qrean.QR_VERSIONS[Qrean.QR_VERSION_1] && qrVersion && qrLevel && qrMask
              ? { version: qrVersion, level: qrLevel, mask: qrMask } : undefined,
            points: [
              [pointsView[0], pointsView[1]],
              [pointsView[2], pointsView[3]],
              [pointsView[4], pointsView[5]],
              [pointsView[6], pointsView[7]],
            ]
          };
          this.detected.push(detected);
          if (this.on_found) this.on_found.call(this, detected);
        },

        debug: (ptr: number) => {
          const result = fromCString(this.memory, ptr);
          console.log("DEBUG:", result);
        },
      },
    };

    this.wasm = WebAssembly.instantiate(wasmbin, importObject).then(wasm => {
      this.instance = wasm.instance;
      this.memory = wasm.instance.exports.memory as WebAssembly.Memory;
      this.memory.grow(opts.pageSize ?? 100);
      this.heap = (wasm.instance.exports.__heap_base as WebAssembly.Global).value;

      if (opts.debug) {
        (wasm.instance.exports as any).enable_debug();
      }
      return wasm;
    });
  }

  private async init() {
    await this.wasm;

    const exp: any = this.instance.exports;

    new Uint8ClampedArray(this.memory.buffer).fill(0, this.heap);
    exp.tinymm_init(this.heap, this.memory.buffer.byteLength - this.heap, 100);
  }

  // type
  static CODE_TYPE_QR = 'QR' as const;
  static CODE_TYPE_MQR = 'mQR' as const;
  static CODE_TYPE_RMQR = 'rMQR' as const;
  static CODE_TYPE_TQR = 'tQR' as const;
  static CODE_TYPE_EAN13 = 'EAN13' as const;
  static CODE_TYPE_EAN8 = 'EAN8' as const;
  static CODE_TYPE_UPCA = 'UPCA' as const;
  static CODE_TYPE_CODE39 = 'CODE39' as const;
  static CODE_TYPE_CODE93 = 'CODE93' as const;
  static CODE_TYPE_NW7 = 'NW7' as const;
  static CODE_TYPE_ITF = 'ITF' as const;
  static CODE_TYPES = {
    [Qrean.CODE_TYPE_QR]: 1 as const,
    [Qrean.CODE_TYPE_MQR]: 2 as const,
    [Qrean.CODE_TYPE_RMQR]: 3 as const,
    [Qrean.CODE_TYPE_TQR]: 4 as const,
    [Qrean.CODE_TYPE_EAN13]: 5 as const,
    [Qrean.CODE_TYPE_EAN8]: 6 as const,
    [Qrean.CODE_TYPE_UPCA]: 7 as const,
    [Qrean.CODE_TYPE_CODE39]: 8 as const,
    [Qrean.CODE_TYPE_CODE93]: 9 as const,
    [Qrean.CODE_TYPE_NW7]: 10 as const,
    [Qrean.CODE_TYPE_ITF]: 11 as const,
  };

  // datatype
  static DATA_TYPE_AUTO = 'AUTO' as const;
  static DATA_TYPE_NUMERIC = 'NUMERIC' as const;
  static DATA_TYPE_ALNUM = 'ALNUM' as const;
  static DATA_TYPE_8BIT = '8BIT' as const;
  static DATA_TYPE_KANJI = 'KANJI' as const;
  static DATA_TYPES = {
    [Qrean.DATA_TYPE_AUTO]: 0 as const,
    [Qrean.DATA_TYPE_NUMERIC]: 1 as const,
    [Qrean.DATA_TYPE_ALNUM]: 2 as const,
    [Qrean.DATA_TYPE_8BIT]: 3 as const,
    [Qrean.DATA_TYPE_KANJI]: 4 as const,
  };

  // qr version
  static QR_VERSION_AUTO = 'AUTO' as const;
  static QR_VERSION_AUTO_W = 'AUTO-W' as const;
  static QR_VERSION_AUTO_H = 'AUTO-H' as const;
  static QR_VERSION_1 = '1' as const;
  static QR_VERSION_2 = '2' as const;
  static QR_VERSION_3 = '3' as const;
  static QR_VERSION_4 = '4' as const;
  static QR_VERSION_5 = '5' as const;
  static QR_VERSION_6 = '6' as const;
  static QR_VERSION_7 = '7' as const;
  static QR_VERSION_8 = '8' as const;
  static QR_VERSION_9 = '9' as const;
  static QR_VERSION_10 = '10' as const;
  static QR_VERSION_11 = '11' as const;
  static QR_VERSION_12 = '12' as const;
  static QR_VERSION_13 = '13' as const;
  static QR_VERSION_14 = '14' as const;
  static QR_VERSION_15 = '15' as const;
  static QR_VERSION_16 = '16' as const;
  static QR_VERSION_17 = '17' as const;
  static QR_VERSION_18 = '18' as const;
  static QR_VERSION_19 = '19' as const;
  static QR_VERSION_20 = '20' as const;
  static QR_VERSION_21 = '21' as const;
  static QR_VERSION_22 = '22' as const;
  static QR_VERSION_23 = '23' as const;
  static QR_VERSION_24 = '24' as const;
  static QR_VERSION_25 = '25' as const;
  static QR_VERSION_26 = '26' as const;
  static QR_VERSION_27 = '27' as const;
  static QR_VERSION_28 = '28' as const;
  static QR_VERSION_29 = '29' as const;
  static QR_VERSION_30 = '30' as const;
  static QR_VERSION_31 = '31' as const;
  static QR_VERSION_32 = '32' as const;
  static QR_VERSION_33 = '33' as const;
  static QR_VERSION_34 = '34' as const;
  static QR_VERSION_35 = '35' as const;
  static QR_VERSION_36 = '36' as const;
  static QR_VERSION_37 = '37' as const;
  static QR_VERSION_38 = '38' as const;
  static QR_VERSION_39 = '39' as const;
  static QR_VERSION_40 = '40' as const;
  static QR_VERSION_M1 = 'M1' as const;
  static QR_VERSION_M2 = 'M2' as const;
  static QR_VERSION_M3 = 'M3' as const;
  static QR_VERSION_M4 = 'M4' as const;
  static QR_VERSION_R7x43 = 'R7x43' as const;
  static QR_VERSION_R7x59 = 'R7x59' as const;
  static QR_VERSION_R7x77 = 'R7x77' as const;
  static QR_VERSION_R7x99 = 'R7x99' as const;
  static QR_VERSION_R7x139 = 'R7x139' as const;
  static QR_VERSION_R9x43 = 'R9x43' as const;
  static QR_VERSION_R9x59 = 'R9x59' as const;
  static QR_VERSION_R9x77 = 'R9x77' as const;
  static QR_VERSION_R9x99 = 'R9x99' as const;
  static QR_VERSION_R9x139 = 'R9x139' as const;
  static QR_VERSION_R11x27 = 'R11x27' as const;
  static QR_VERSION_R11x43 = 'R11x43' as const;
  static QR_VERSION_R11x59 = 'R11x59' as const;
  static QR_VERSION_R11x77 = 'R11x77' as const;
  static QR_VERSION_R11x99 = 'R11x99' as const;
  static QR_VERSION_R11x139 = 'R11x139' as const;
  static QR_VERSION_R13x27 = 'R13x27' as const;
  static QR_VERSION_R13x43 = 'R13x43' as const;
  static QR_VERSION_R13x59 = 'R13x59' as const;
  static QR_VERSION_R13x77 = 'R13x77' as const;
  static QR_VERSION_R13x99 = 'R13x99' as const;
  static QR_VERSION_R13x139 = 'R13x139' as const;
  static QR_VERSION_R15x43 = 'R15x43' as const;
  static QR_VERSION_R15x59 = 'R15x59' as const;
  static QR_VERSION_R15x77 = 'R15x77' as const;
  static QR_VERSION_R15x99 = 'R15x99' as const;
  static QR_VERSION_R15x139 = 'R15x139' as const;
  static QR_VERSION_R17x43 = 'R17x43' as const;
  static QR_VERSION_R17x59 = 'R17x59' as const;
  static QR_VERSION_R17x77 = 'R17x77' as const;
  static QR_VERSION_R17x99 = 'R17x99' as const;
  static QR_VERSION_R17x139 = 'R17x139' as const;
  static QR_VERSION_TQR = 'TQR' as const;
  static QR_VERSIONS = {
    [Qrean.QR_VERSION_AUTO]:     0 as const,
    [Qrean.QR_VERSION_AUTO_W]:   1 as const,
    [Qrean.QR_VERSION_AUTO_H]:   2 as const,
    [Qrean.QR_VERSION_1]:        3 as const,
    [Qrean.QR_VERSION_2]:        4 as const,
    [Qrean.QR_VERSION_3]:        5 as const,
    [Qrean.QR_VERSION_4]:        6 as const,
    [Qrean.QR_VERSION_5]:        7 as const,
    [Qrean.QR_VERSION_6]:        8 as const,
    [Qrean.QR_VERSION_7]:        9 as const,
    [Qrean.QR_VERSION_8]:       10 as const,
    [Qrean.QR_VERSION_9]:       11 as const,
    [Qrean.QR_VERSION_10]:      12 as const,
    [Qrean.QR_VERSION_11]:      13 as const,
    [Qrean.QR_VERSION_12]:      14 as const,
    [Qrean.QR_VERSION_13]:      15 as const,
    [Qrean.QR_VERSION_14]:      16 as const,
    [Qrean.QR_VERSION_15]:      17 as const,
    [Qrean.QR_VERSION_16]:      18 as const,
    [Qrean.QR_VERSION_17]:      19 as const,
    [Qrean.QR_VERSION_18]:      20 as const,
    [Qrean.QR_VERSION_19]:      21 as const,
    [Qrean.QR_VERSION_20]:      22 as const,
    [Qrean.QR_VERSION_21]:      23 as const,
    [Qrean.QR_VERSION_22]:      24 as const,
    [Qrean.QR_VERSION_23]:      25 as const,
    [Qrean.QR_VERSION_24]:      26 as const,
    [Qrean.QR_VERSION_25]:      27 as const,
    [Qrean.QR_VERSION_26]:      28 as const,
    [Qrean.QR_VERSION_27]:      29 as const,
    [Qrean.QR_VERSION_28]:      30 as const,
    [Qrean.QR_VERSION_29]:      31 as const,
    [Qrean.QR_VERSION_30]:      32 as const,
    [Qrean.QR_VERSION_31]:      33 as const,
    [Qrean.QR_VERSION_32]:      34 as const,
    [Qrean.QR_VERSION_33]:      35 as const,
    [Qrean.QR_VERSION_34]:      36 as const,
    [Qrean.QR_VERSION_35]:      37 as const,
    [Qrean.QR_VERSION_36]:      38 as const,
    [Qrean.QR_VERSION_37]:      39 as const,
    [Qrean.QR_VERSION_38]:      40 as const,
    [Qrean.QR_VERSION_39]:      41 as const,
    [Qrean.QR_VERSION_40]:      42 as const,
    [Qrean.QR_VERSION_M1]:      43 as const,
    [Qrean.QR_VERSION_M2]:      44 as const,
    [Qrean.QR_VERSION_M3]:      45 as const,
    [Qrean.QR_VERSION_M4]:      46 as const,
    [Qrean.QR_VERSION_R7x43]:   47 as const,
    [Qrean.QR_VERSION_R7x59]:   48 as const,
    [Qrean.QR_VERSION_R7x77]:   49 as const,
    [Qrean.QR_VERSION_R7x99]:   50 as const,
    [Qrean.QR_VERSION_R7x139]:  51 as const,
    [Qrean.QR_VERSION_R9x43]:   52 as const,
    [Qrean.QR_VERSION_R9x59]:   53 as const,
    [Qrean.QR_VERSION_R9x77]:   54 as const,
    [Qrean.QR_VERSION_R9x99]:   55 as const,
    [Qrean.QR_VERSION_R9x139]:  56 as const,
    [Qrean.QR_VERSION_R11x27]:  57 as const,
    [Qrean.QR_VERSION_R11x43]:  58 as const,
    [Qrean.QR_VERSION_R11x59]:  59 as const,
    [Qrean.QR_VERSION_R11x77]:  60 as const,
    [Qrean.QR_VERSION_R11x99]:  61 as const,
    [Qrean.QR_VERSION_R11x139]: 62 as const,
    [Qrean.QR_VERSION_R13x27]:  63 as const,
    [Qrean.QR_VERSION_R13x43]:  64 as const,
    [Qrean.QR_VERSION_R13x59]:  65 as const,
    [Qrean.QR_VERSION_R13x77]:  66 as const,
    [Qrean.QR_VERSION_R13x99]:  67 as const,
    [Qrean.QR_VERSION_R13x139]: 68 as const,
    [Qrean.QR_VERSION_R15x43]:  69 as const,
    [Qrean.QR_VERSION_R15x59]:  70 as const,
    [Qrean.QR_VERSION_R15x77]:  71 as const,
    [Qrean.QR_VERSION_R15x99]:  72 as const,
    [Qrean.QR_VERSION_R15x139]: 73 as const,
    [Qrean.QR_VERSION_R17x43]:  74 as const,
    [Qrean.QR_VERSION_R17x59]:  75 as const,
    [Qrean.QR_VERSION_R17x77]:  76 as const,
    [Qrean.QR_VERSION_R17x99]:  77 as const,
    [Qrean.QR_VERSION_R17x139]: 78 as const,
    [Qrean.QR_VERSION_TQR]:     79 as const,
  };

  // qr errorlevel
  static QR_ERRORLEVEL_L = 'L' as const;
  static QR_ERRORLEVEL_M = 'M' as const;
  static QR_ERRORLEVEL_Q = 'Q' as const;
  static QR_ERRORLEVEL_H = 'H' as const;
  static QR_ERRORLEVELS = {
    [Qrean.QR_ERRORLEVEL_L]: 0 as const,
    [Qrean.QR_ERRORLEVEL_M]: 1 as const,
    [Qrean.QR_ERRORLEVEL_Q]: 2 as const,
    [Qrean.QR_ERRORLEVEL_H]: 3 as const,
  };

  // qr maskpattern
  static QR_MASKPATTERN_0 = '0' as const;
  static QR_MASKPATTERN_1 = '1' as const;
  static QR_MASKPATTERN_2 = '2' as const;
  static QR_MASKPATTERN_3 = '3' as const;
  static QR_MASKPATTERN_4 = '4' as const;
  static QR_MASKPATTERN_5 = '5' as const;
  static QR_MASKPATTERN_6 = '6' as const;
  static QR_MASKPATTERN_7 = '7' as const;
  static QR_MASKPATTERN_AUTO = 'AUTO' as const;
  static QR_MASKPATTERNS = {
    [Qrean.QR_MASKPATTERN_0]: 0 as const,
    [Qrean.QR_MASKPATTERN_1]: 1 as const,
    [Qrean.QR_MASKPATTERN_2]: 2 as const,
    [Qrean.QR_MASKPATTERN_3]: 3 as const,
    [Qrean.QR_MASKPATTERN_4]: 4 as const,
    [Qrean.QR_MASKPATTERN_5]: 5 as const,
    [Qrean.QR_MASKPATTERN_6]: 6 as const,
    [Qrean.QR_MASKPATTERN_7]: 7 as const,
    [Qrean.QR_MASKPATTERN_AUTO]: 10 as const,
  };

  static QR_ECI_CODE_LATIN1 = 'Latin1' as const;
  static QR_ECI_CODE_SJIS = 'ShiftJIS' as const;
  static QR_ECI_CODE_UTF8 = 'UTF-8' as const;
  static QR_ECI_CODES = {
    [Qrean.QR_ECI_CODE_LATIN1]: 3 as const,
    [Qrean.QR_ECI_CODE_SJIS]: 20 as const,
    [Qrean.QR_ECI_CODE_UTF8]: 26 as const,
  };

  async encode(text: string, opts: EncodeOptions | keyof typeof Qrean.CODE_TYPES = {}) {
    await this.init();

    if (typeof opts == 'string') {
      opts = { codeType: opts };
    }

    const exp: any = this.instance.exports;
    const view = new DataView(this.memory.buffer);
    const mem = new Uint8ClampedArray(this.memory.buffer);

    const cstr = toCString(text);
    const text_ptr = exp.malloc(cstr.byteLength);
    mem.set(cstr, text_ptr);

    const opts_ptr = exp.malloc(16 * 4);

    const optsbuf = [
      Qrean.CODE_TYPES[opts.codeType ?? Qrean.CODE_TYPE_QR],
      Qrean.DATA_TYPES[opts.dataType ?? Qrean.DATA_TYPE_AUTO],
      Qrean.QR_ERRORLEVELS[opts.qrErrorLevel ?? Qrean.QR_ERRORLEVEL_M],
      Qrean.QR_VERSIONS[opts.qrVersion ?? Qrean.QR_VERSION_AUTO],
      Qrean.QR_MASKPATTERNS[opts.qrMaskPattern ?? Qrean.QR_MASKPATTERN_AUTO],
      opts.padding?.[0] ?? -1,
      opts.padding?.[1] ?? -1,
      opts.padding?.[2] ?? -1,
      opts.padding?.[3] ?? -1,
      opts.scale || 4,
    ];
    for (let i = 0; i < optsbuf.length; i++) {
      view.setUint32(opts_ptr + i * 4, optsbuf[i], true);
    }

    const result = exp.encode(
      text_ptr,
      opts_ptr,
    );

    if (!result) {
      return false;
    }

    const imgdata = this.readImage(result);

    exp.free(result);

    return imgdata;
  }

  private allocImage(img: Image): number {
    const exp: any = this.instance.exports;
    const view = new DataView(this.memory.buffer);
    const mem = new Uint8Array(this.memory.buffer);

    const ptr = exp.malloc(12 + img.width * img.height * 4);
    const imgbuf = ptr + 12;
    view.setUint32(ptr + 0, img.width, true);
    view.setUint32(ptr + 4, img.height, true);
    view.setUint32(ptr + 8, imgbuf, true);
    mem.set(img.data, imgbuf);

    return ptr;
  }

  private readImage(ptr: number): Image {
    const view = new DataView(this.memory.buffer);
    const mem = new Uint8ClampedArray(this.memory.buffer);

    const width = view.getUint32(ptr + 0, true);
    const height = view.getUint32(ptr + 4, true);
    const imgbuf = view.getUint32(ptr + 8, true);

    return {
      width,
      height,
      data: mem.slice(imgbuf, imgbuf + width * height * 4),
    };
  }

  async detect(imgdata: Image, opts: DetectOptions = {}, callback?: (obj: Detected) => void) {
    await this.init();

    const gamma_value = opts.gamma ?? 1.0;
    const exp: any = this.instance.exports;

    const outbuf_size = opts.outbufSize ?? 1024;
    const outbuf_ptr = exp.malloc(outbuf_size);

    const image_ptr = this.allocImage(imgdata);

    this.on_found = callback;
    this.detected = [];

    exp.detect(
      outbuf_ptr,
      outbuf_size,
      image_ptr,
      gamma_value,
      Qrean.QR_ECI_CODES[opts.eciCode ?? Qrean.QR_ECI_CODE_LATIN1],
    );

    this.on_found = undefined;

    const digitized = this.readImage(image_ptr);

    exp.free(image_ptr);
    exp.free(outbuf_ptr);

    return {
      detected: this.detected,
      digitized,
    }
  }
}
