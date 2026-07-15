// Port of lib/effect/StopLightEffect.{hpp,cpp}.

import { hsv2rgbRainbow } from '../fastled.js';

const RED = { h: 0, s: 255, v: 255 };
const DIM_RED = { h: 0, s: 255, v: 32 };
const AMBER = { h: 32, s: 255, v: 255 };
const DIM_AMBER = { h: 32, s: 255, v: 32 };
const GREEN = { h: 102, s: 255, v: 255 };
const DIM_GREEN = { h: 102, s: 255, v: 32 };

function halve(rgb) {
  return {
    r: Math.trunc(rgb.r / 2),
    g: Math.trunc(rgb.g / 2),
    b: Math.trunc(rgb.b / 2),
  };
}

export function makeStopLightEffect() {
  return {
    name: 'Stop Light',
    getRGB(ledIndex, timeMs, strip) {
      const ledPos =
        (Math.abs((strip.ledCount >> 1) - ledIndex) << 8) & 0xffff;
      const t = timeMs >>> 11;

      const isRed = (t & 0b100) === 0 && (t & 0b11) > 0;
      const isAmber = (t & 0b100) === 0 && (t & 0b11) === 0;
      const isGreen = (t & 0b100) !== 0;

      if (strip.hasFlag('Tiny')) {
        if (isRed) {
          return hsv2rgbRainbow(RED);
        } else if (isAmber) {
          return hsv2rgbRainbow(AMBER);
        } else {
          return hsv2rgbRainbow(GREEN);
        }
      }

      if (strip.hasFlag('Controller')) {
        const segment = Math.trunc(strip.ledCount / 5);
        if (ledIndex <= segment) {
          return hsv2rgbRainbow(RED);
        }
        if (ledIndex > segment * 2 && ledIndex <= segment * 3) {
          return halve(hsv2rgbRainbow(AMBER));
        }
        if (ledIndex > segment * 4) {
          return hsv2rgbRainbow(GREEN);
        }
        return { r: 0, g: 0, b: 0 };
      }

      const segment = (strip.ledCount << (8 - 3)) & 0xffff;

      if (ledPos < segment) {
        return { r: 0, g: 0, b: 0 };
      } else if (ledPos > segment && ledPos < segment * 2) {
        return isRed ? hsv2rgbRainbow(RED) : hsv2rgbRainbow(DIM_RED);
      } else if (ledPos > segment * 2 && ledPos < segment * 3) {
        return isAmber ? hsv2rgbRainbow(AMBER) : hsv2rgbRainbow(DIM_AMBER);
      } else if (ledPos > segment * 3) {
        return isGreen ? hsv2rgbRainbow(GREEN) : hsv2rgbRainbow(DIM_GREEN);
      } else {
        return { r: 0, g: 0, b: 0 };
      }
    },
  };
}
