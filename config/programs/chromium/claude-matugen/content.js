// Claude Matugen
//
// NOTES (for future maintenance):
// - colors.json is written by matugen (templates/websites/claude.json.template)
//   and only contains hex values (stable across matugen versions).
// - claude.ai's semantic tokens (--bg-*, --cds-surface-*, --df-*) live partly in
//   @layer blocks and change often. Instead of chasing them we recolor the
//   *primitive palettes* everything is derived from:
//     --cds-gray-N      (hex)          + --cds-hsl-gray-N  ("H S% L%")
//     --cds-blue-N      (hex, accents) + --cds-hsl-blue-N
//     --cds-hsl-clay(-emphasized)      (brand orange)
//     --df-z0..z6       (hardcoded greys of the dframe shell)
//   The ORIGINAL lightness of every step is read at runtime and kept; only hue
//   and saturation are replaced. So contrast stays exactly as Anthropic designed
//   it, and lightness tweaks on their side don't break us.
// - These primitives are re-declared on .cds-root / [data-mode] / .dframe-root
//   elements, so the override targets those too, with !important.
// - Unpacked extension files are read from disk: after a wallpaper change a
//   page reload (F5) is enough.

// Tint strength for the grey ramp, in % saturation. 0 = original grey.
const GREY_SATURATION = 14;

const STEPS = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200, 250, 300,
  350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 810, 820, 830, 840, 850,
  860, 870, 880, 890, 900];
const DF_Z = { 0: 3.9, 1: 10.2, 2: 14.9, 3: 20, 4: 23.9, 5: 32.2, 6: 65.1 };

const hexToHsl = (hex) => {
  const m = /^#?([0-9a-f]{3}|[0-9a-f]{6})\b/i.exec(hex.trim());
  if (!m) return null;
  let h6 = m[1].length === 3 ? [...m[1]].map((c) => c + c).join("") : m[1];
  const [r, g, b] = [0, 2, 4].map((i) => parseInt(h6.slice(i, i + 2), 16) / 255);
  const max = Math.max(r, g, b), min = Math.min(r, g, b);
  const l = (max + min) / 2;
  let h = 0, s = 0;
  if (max !== min) {
    const d = max - min;
    s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
    if (max === r) h = (g - b) / d + (g < b ? 6 : 0);
    else if (max === g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h *= 60;
  }
  return { h, s: s * 100, l: l * 100 };
};

const hslToHex = ({ h, s, l }) => {
  s /= 100; l /= 100;
  const k = (n) => (n + h / 30) % 12;
  const a = s * Math.min(l, 1 - l);
  const f = (n) => l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1)));
  return "#" + [0, 8, 4].map((n) => Math.round(f(n) * 255).toString(16).padStart(2, "0")).join("");
};

const r2 = (x) => Math.round(x * 100) / 100;
const triplet = ({ h, s, l }) => `${r2(h)} ${r2(s)}% ${r2(l)}%`;

// Read the original value of a primitive (before our override exists).
const readVar = (name) =>
  getComputedStyle(document.documentElement).getPropertyValue(name).trim();

const waitForPalette = () =>
  new Promise((resolve) => {
    const check = () => (readVar("--cds-gray-800") ? resolve() : requestAnimationFrame(check));
    check();
  });

(async () => {
  let colors;
  try {
    const res = await fetch(chrome.runtime.getURL("colors.json"), { cache: "no-store" });
    colors = await res.json();
  } catch (e) {
    console.warn("[claude-matugen] could not load colors.json", e);
    return;
  }
  const primary = hexToHsl(colors.primary);
  if (!primary) return console.warn("[claude-matugen] invalid primary", colors.primary);

  await waitForPalette();

  const out = [];
  const recolorRamp = (ramp, hue, sat) => {
    for (const step of STEPS) {
      const orig = hexToHsl(readVar(`--cds-${ramp}-${step}`) || "");
      if (!orig) continue;
      const c = { h: hue, s: sat, l: orig.l };
      out.push(`--cds-${ramp}-${step}: ${hslToHex(c)}`);
      out.push(`--cds-hsl-${ramp}-${step}: ${triplet(c)}`);
    }
  };

  recolorRamp("gray", primary.h, GREY_SATURATION);
  recolorRamp("blue", primary.h, primary.s);

  for (const [z, l] of Object.entries(DF_Z))
    out.push(`--df-z${z}: ${triplet({ h: primary.h, s: GREY_SATURATION, l })}`);

  out.push(`--cds-clay: ${colors.primary}`);
  out.push(`--cds-hsl-clay: ${triplet(primary)}`);
  out.push(`--cds-hsl-clay-emphasized: ${triplet({ ...primary, l: Math.max(0, primary.l - 8) })}`);

  const style = document.createElement("style");
  style.id = "claude-matugen";
  style.textContent =
    `:root, [data-mode], .cds-root, .dframe-root {\n` +
    out.map((d) => `  ${d} !important;`).join("\n") + `\n}`;
  (document.head || document.documentElement).appendChild(style);
})();
