import * as t from "https://deno.land/std/testing/asserts.ts";
import QRCode from 'npm:qrcode';
import { Qrean } from "./Qrean.ts";

const imgdata2s = (imgd, step = 1) => {
  const ss = [];
  const width = imgd.width;
  const height = imgd.height;
  const data = imgd.data;
  for (let y = 0; y < height; y += step) {
    for (let x = 0; x < width; x += step) {
      const idx = (x + y * width) * 4;
      if (data[idx]) {
        ss.push("1");
      } else {
        ss.push("0");
      }
    }
    ss.push("\n");
  }
  return ss.join("").trim();
};

Deno.test("mQR", async () => {
  const qrean = new Qrean();
  const imgd = await qrean.encode("test", {
    codeType: Qrean.CODE_TYPE_MQR,
    dataType: Qrean.DATA_TYPE_8BIT,
    qrMaskPattern: Qrean.QR_MASKPATTERN_1,
    qrErrorLevel: Qrean.QR_ERRORLEVEL_L,
    scale: 1,
    padding: 2,
  });
  t.assertEquals(imgd.width, 19);
  t.assertEquals(imgd.height, 19);

  // based on `qrencode -s 1 -8 -M -l L -t XPM test | tr 'FB' '01'`
  t.assertEquals(imgdata2s(imgd), `
1111111111111111111
1111111111111111111
1100000001010101011
1101111101011010011
1101000101000110011
1101000101010100111
1101000101011001011
1101111101101101011
1100000001100011011
1111111111000010111
1100001100100100011
1111011011010101011
1101101011011010011
1110001011110100111
1101000011011110011
1111000001101011011
1101010001000011011
1111111111111111111
1111111111111111111
`.trim());
});

Deno.test("rMQR", async () => {
  const qrean = new Qrean();
  const imgd = await qrean.encode("test", {
    codeType: Qrean.CODE_TYPE_RMQR,
    dataType: Qrean.DATA_TYPE_8BIT,
    qrVersion: Qrean.QR_VERSION_AUTO_W,
  });
  t.assertEquals(imgd.width, 188);
  t.assertEquals(imgd.height, 44);
  t.assertEquals(imgdata2s(imgd, 4), `
11111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111111
11000000010101010101010001010101010101010100011
11011111011010111101110101000010110110011101011
11010001010100101011000001011011111110000000011
11010001011001001111001111101001101111110111011
11010001011100000001110001000101011000110101011
11011111010000111101100101000110110110010111011
11000000010101010101010001010101010101010000011
11111111111111111111111111111111111111111111111
11111111111111111111111111111111111111111111111
`.trim());
});
