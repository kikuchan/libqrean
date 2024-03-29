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
    eciCode?: 'UTF-8' | 'ShiftJIS' | 'Latin1';
    scale?: number;
    padding?: number[] | number;
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
};
export type Detected = {
    type: keyof typeof Qrean.CODE_TYPES;
    text: string;
    points: number[][];
    qr?: {
        version: keyof typeof Qrean.QR_VERSIONS;
        mask: keyof typeof Qrean.QR_MASKPATTERNS;
        level: keyof typeof Qrean.QR_ERRORLEVELS;
    };
};
export declare class Qrean {
    private wasm;
    private on_found?;
    private instance;
    private heap;
    private memory;
    private detected;
    static create(opts?: CreateOptions): Promise<Qrean>;
    constructor(opts?: CreateOptions);
    private init;
    static CODE_TYPE_QR: "QR";
    static CODE_TYPE_MQR: "mQR";
    static CODE_TYPE_RMQR: "rMQR";
    static CODE_TYPE_TQR: "tQR";
    static CODE_TYPE_EAN13: "EAN13";
    static CODE_TYPE_EAN8: "EAN8";
    static CODE_TYPE_UPCA: "UPCA";
    static CODE_TYPE_CODE39: "CODE39";
    static CODE_TYPE_CODE93: "CODE93";
    static CODE_TYPE_NW7: "NW7";
    static CODE_TYPE_ITF: "ITF";
    static CODE_TYPES: {
        QR: 1;
        mQR: 2;
        rMQR: 3;
        tQR: 4;
        EAN13: 5;
        EAN8: 6;
        UPCA: 7;
        CODE39: 8;
        CODE93: 9;
        NW7: 10;
        ITF: 11;
    };
    static DATA_TYPE_AUTO: "AUTO";
    static DATA_TYPE_NUMERIC: "NUMERIC";
    static DATA_TYPE_ALNUM: "ALNUM";
    static DATA_TYPE_8BIT: "8BIT";
    static DATA_TYPE_KANJI: "KANJI";
    static DATA_TYPES: {
        AUTO: 0;
        NUMERIC: 1;
        ALNUM: 2;
        "8BIT": 3;
        KANJI: 4;
    };
    static QR_VERSION_AUTO: "AUTO";
    static QR_VERSION_AUTO_W: "AUTO-W";
    static QR_VERSION_AUTO_H: "AUTO-H";
    static QR_VERSION_1: "1";
    static QR_VERSION_2: "2";
    static QR_VERSION_3: "3";
    static QR_VERSION_4: "4";
    static QR_VERSION_5: "5";
    static QR_VERSION_6: "6";
    static QR_VERSION_7: "7";
    static QR_VERSION_8: "8";
    static QR_VERSION_9: "9";
    static QR_VERSION_10: "10";
    static QR_VERSION_11: "11";
    static QR_VERSION_12: "12";
    static QR_VERSION_13: "13";
    static QR_VERSION_14: "14";
    static QR_VERSION_15: "15";
    static QR_VERSION_16: "16";
    static QR_VERSION_17: "17";
    static QR_VERSION_18: "18";
    static QR_VERSION_19: "19";
    static QR_VERSION_20: "20";
    static QR_VERSION_21: "21";
    static QR_VERSION_22: "22";
    static QR_VERSION_23: "23";
    static QR_VERSION_24: "24";
    static QR_VERSION_25: "25";
    static QR_VERSION_26: "26";
    static QR_VERSION_27: "27";
    static QR_VERSION_28: "28";
    static QR_VERSION_29: "29";
    static QR_VERSION_30: "30";
    static QR_VERSION_31: "31";
    static QR_VERSION_32: "32";
    static QR_VERSION_33: "33";
    static QR_VERSION_34: "34";
    static QR_VERSION_35: "35";
    static QR_VERSION_36: "36";
    static QR_VERSION_37: "37";
    static QR_VERSION_38: "38";
    static QR_VERSION_39: "39";
    static QR_VERSION_40: "40";
    static QR_VERSION_M1: "M1";
    static QR_VERSION_M2: "M2";
    static QR_VERSION_M3: "M3";
    static QR_VERSION_M4: "M4";
    static QR_VERSION_R7x43: "R7x43";
    static QR_VERSION_R7x59: "R7x59";
    static QR_VERSION_R7x77: "R7x77";
    static QR_VERSION_R7x99: "R7x99";
    static QR_VERSION_R7x139: "R7x139";
    static QR_VERSION_R9x43: "R9x43";
    static QR_VERSION_R9x59: "R9x59";
    static QR_VERSION_R9x77: "R9x77";
    static QR_VERSION_R9x99: "R9x99";
    static QR_VERSION_R9x139: "R9x139";
    static QR_VERSION_R11x27: "R11x27";
    static QR_VERSION_R11x43: "R11x43";
    static QR_VERSION_R11x59: "R11x59";
    static QR_VERSION_R11x77: "R11x77";
    static QR_VERSION_R11x99: "R11x99";
    static QR_VERSION_R11x139: "R11x139";
    static QR_VERSION_R13x27: "R13x27";
    static QR_VERSION_R13x43: "R13x43";
    static QR_VERSION_R13x59: "R13x59";
    static QR_VERSION_R13x77: "R13x77";
    static QR_VERSION_R13x99: "R13x99";
    static QR_VERSION_R13x139: "R13x139";
    static QR_VERSION_R15x43: "R15x43";
    static QR_VERSION_R15x59: "R15x59";
    static QR_VERSION_R15x77: "R15x77";
    static QR_VERSION_R15x99: "R15x99";
    static QR_VERSION_R15x139: "R15x139";
    static QR_VERSION_R17x43: "R17x43";
    static QR_VERSION_R17x59: "R17x59";
    static QR_VERSION_R17x77: "R17x77";
    static QR_VERSION_R17x99: "R17x99";
    static QR_VERSION_R17x139: "R17x139";
    static QR_VERSION_TQR: "TQR";
    static QR_VERSIONS: {
        AUTO: 0;
        "AUTO-W": 1;
        "AUTO-H": 2;
        "1": 3;
        "2": 4;
        "3": 5;
        "4": 6;
        "5": 7;
        "6": 8;
        "7": 9;
        "8": 10;
        "9": 11;
        "10": 12;
        "11": 13;
        "12": 14;
        "13": 15;
        "14": 16;
        "15": 17;
        "16": 18;
        "17": 19;
        "18": 20;
        "19": 21;
        "20": 22;
        "21": 23;
        "22": 24;
        "23": 25;
        "24": 26;
        "25": 27;
        "26": 28;
        "27": 29;
        "28": 30;
        "29": 31;
        "30": 32;
        "31": 33;
        "32": 34;
        "33": 35;
        "34": 36;
        "35": 37;
        "36": 38;
        "37": 39;
        "38": 40;
        "39": 41;
        "40": 42;
        M1: 43;
        M2: 44;
        M3: 45;
        M4: 46;
        R7x43: 47;
        R7x59: 48;
        R7x77: 49;
        R7x99: 50;
        R7x139: 51;
        R9x43: 52;
        R9x59: 53;
        R9x77: 54;
        R9x99: 55;
        R9x139: 56;
        R11x27: 57;
        R11x43: 58;
        R11x59: 59;
        R11x77: 60;
        R11x99: 61;
        R11x139: 62;
        R13x27: 63;
        R13x43: 64;
        R13x59: 65;
        R13x77: 66;
        R13x99: 67;
        R13x139: 68;
        R15x43: 69;
        R15x59: 70;
        R15x77: 71;
        R15x99: 72;
        R15x139: 73;
        R17x43: 74;
        R17x59: 75;
        R17x77: 76;
        R17x99: 77;
        R17x139: 78;
        TQR: 79;
    };
    static QR_ERRORLEVEL_L: "L";
    static QR_ERRORLEVEL_M: "M";
    static QR_ERRORLEVEL_Q: "Q";
    static QR_ERRORLEVEL_H: "H";
    static QR_ERRORLEVELS: {
        L: 0;
        M: 1;
        Q: 2;
        H: 3;
    };
    static QR_MASKPATTERN_0: "0";
    static QR_MASKPATTERN_1: "1";
    static QR_MASKPATTERN_2: "2";
    static QR_MASKPATTERN_3: "3";
    static QR_MASKPATTERN_4: "4";
    static QR_MASKPATTERN_5: "5";
    static QR_MASKPATTERN_6: "6";
    static QR_MASKPATTERN_7: "7";
    static QR_MASKPATTERN_AUTO: "AUTO";
    static QR_MASKPATTERNS: {
        "0": 0;
        "1": 1;
        "2": 2;
        "3": 3;
        "4": 4;
        "5": 5;
        "6": 6;
        "7": 7;
        AUTO: 10;
    };
    static QR_ECI_CODE_LATIN1: "Latin1";
    static QR_ECI_CODE_SJIS: "ShiftJIS";
    static QR_ECI_CODE_UTF8: "UTF-8";
    static QR_ECI_CODES: {
        Latin1: 3;
        ShiftJIS: 20;
        "UTF-8": 26;
    };
    static encode(text: string, opts?: EncodeOptions | keyof typeof Qrean.CODE_TYPES): Promise<false | Image>;
    encode(text: string, opts?: EncodeOptions | keyof typeof Qrean.CODE_TYPES): Promise<false | Image>;
    private allocImage;
    private readImage;
    static detect(imgdata: Image, opts?: DetectOptions, callback?: (obj: Detected) => void): Promise<{
        detected: Detected[];
        digitized: Image;
    }>;
    detect(imgdata: Image, opts?: DetectOptions, callback?: (obj: Detected) => void): Promise<{
        detected: Detected[];
        digitized: Image;
    }>;
}
export {};
