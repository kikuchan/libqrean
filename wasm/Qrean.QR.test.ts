import * as t from "https://deno.land/std/testing/asserts.ts";
import QRCode from 'npm:qrcode';
import { Qrean } from "./Qrean.ts";

Deno.test("QR", async () => {
  async function qrean(v: number, e: 'L' | 'M' | 'Q' | 'H', m: number) {
    return await Qrean.encode('Hello', {
      scale: 1,
      eciCode: 'Latin1',
      dataType: '8BIT',
      qrVersion: String(v) as any,
      qrErrorLevel: e,
      qrMaskPattern: String(m) as any,
    }).then(obj => {
      if (!obj) return false;
      const w = obj.width;
      const stride = 4;

      const lines = [];
      for (let y = 0; y < obj.height; y++) {
        const line = [];
        for (let x = 0; x < obj.width; x++) {
          line.push(obj.data[(y * w + x) * stride + 0] == 0 ? '1' : '0');
        }
        lines.push(line.join(''));
      }
      return lines.join('\n');
    });
  }

  async function reference(v: number, e: string, m: number) {
    return await QRCode.toString(
      [{ data: 'Hello', mode: 'Byte' }],
      {
        version: v,
        errorCorrectionLevel: e,
        maskPattern: m,
      }
    ).then((s: string) => s.split('\n').flatMap((line: string) => {
      const odd = (x: string) => Math.floor(" ▄▀█".indexOf(x) / 2);
      const even = (x: string) => Math.floor(" ▄▀█".indexOf(x) % 2);
      const l = line.split('');
      return [l.map(odd).join(''), l.map(even).join('')];
    }).slice(0, -1).join('\n'));
  }

  for (let v = 1; v <= 40; v++) {
    for (const e of ['L', 'M', 'Q', 'H'] as ('L' | 'M' | 'Q' | 'H')[]) {
      for (let m = 0; m < 8; m++) {
        const a = await qrean(v, e, m);
        const b = await reference(v, e, m);

        t.assertEquals(a, b);
      }
    }
  }
});
