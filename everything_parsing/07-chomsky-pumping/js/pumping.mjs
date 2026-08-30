// Module 07 — the pumping lemmas, run as an adversary.
//
// Regular: if L is regular, some length p makes every s in L (|s| >= p) split
// as x y z with |y| >= 1, |xy| <= p and x y^k z in L for ALL k.
// We show { a^n b^n } cannot satisfy this for ANY p, and { a^n b^n c^n } cannot
// satisfy the 5-part context-free version for any p.

export function inAnBn(s) {
  let i = 0;
  while (i < s.length && s[i] === "a") i++;
  const a = i;
  while (i < s.length && s[i] === "b") i++;
  return i === s.length && a === s.length - a;
}

export function inAnBnCn(s) {
  let i = 0;
  while (i < s.length && s[i] === "a") i++;
  const a = i;
  while (i < s.length && s[i] === "b") i++;
  const b = i - a;
  while (i < s.length && s[i] === "c") i++;
  const c = i - a - b;
  return i === s.length && a === b && b === c;
}

const rep = (ch, n) => ch.repeat(n);

// ── regular refutation for { a^n b^n } ──

export function regularRefutation(p) {
  const s = rep("a", p) + rep("b", p);
  let tried = 0, allEscaped = true;
  let repX = "", repY = "", repZ = "", repPumped = "", repK = 0;

  for (let b = 1; b <= p; b++) {
    for (let a = 0; a < b; a++) {
      tried++;
      const x = s.slice(0, a), y = s.slice(a, b), z = s.slice(b);
      const k = firstEscape(kk => x + rep(y, kk) + z, inAnBn);
      if (k < 0) allEscaped = false;
      else if (repPumped === "") { repX = x; repY = y; repZ = z; repK = k; repPumped = x + rep(y, k) + z; }
    }
  }
  return { s, x: repX, y: repY, z: repZ, k: repK, pumped: repPumped, decompositionsTried: tried, allEscaped };
}

// ── context-free refutation for { a^n b^n c^n } ──

export function cflRefutation(p) {
  const s = rep("a", p) + rep("b", p) + rep("c", p);
  const n = s.length;
  let tried = 0, allEscaped = true;
  let rU = "", rV = "", rW = "", rX = "", rY = "", rP = "", rK = 0;

  for (let i = 0; i <= n; i++) {
    for (let j = i; j <= Math.min(n, i + p); j++) {
      for (let l = j; l <= Math.min(n, i + p); l++) {
        for (let m = l; m <= Math.min(n, i + p); m++) {
          const vLen = j - i, xLen = m - l;
          if (vLen + xLen < 1) continue;
          if (m - i > p) continue;
          tried++;
          const u = s.slice(0, i), v = s.slice(i, j), w = s.slice(j, l),
                x = s.slice(l, m), y = s.slice(m);
          const k = firstEscape(kk => u + rep(v, kk) + w + rep(x, kk) + y, inAnBnCn);
          if (k < 0) allEscaped = false;
          else if (rP === "") {
            rU = u; rV = v; rW = w; rX = x; rY = y; rK = k;
            rP = u + rep(v, k) + w + rep(x, k) + y;
          }
        }
      }
    }
  }
  return { s, u: rU, v: rV, w: rW, x: rX, y: rY, k: rK, pumped: rP, decompositionsTried: tried, allEscaped };
}

// smallest k in {2, 0, 3} that lands the pumped string outside L
function firstEscape(build, inL) {
  for (const k of [2, 0, 3]) if (!inL(build(k))) return k;
  return -1;
}
