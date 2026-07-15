// Port of lib/effect/ControlEffect.{hpp,cpp}.

export function makeControlEffect() {
  return {
    name: 'Control',
    getRGB(ledIndex, timeMs, strip, show) {
      const rgb = show.controlRgb;
      return { r: rgb.r, g: rgb.g, b: rgb.b };
    },
  };
}
