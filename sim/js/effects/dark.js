// Port of lib/effect/DarkEffect.{hpp,cpp}.

export function makeDarkEffect() {
  return {
    name: 'Dark',
    getRGB() {
      return { r: 0, g: 0, b: 0 };
    },
  };
}
