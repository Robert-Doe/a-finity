// Module 29 — a backtracking recursive-descent recognizer that COUNTS
// (mirror of java/Backtrack.java).
//
// deriveSym tries every production, follows all of them (no lookahead), and
// returns per-end-position the number of ways to derive the consumed span.
// `entries` = production attempts = the raw work. No memoization -> exponential
// when alternatives overlap. Packrat (Appendix X2) memoizes (nt, pos) -> linear.
// Grammar must be free of left recursion.

export class Backtrack {
  constructor(g, cap = 20_000_000) { this.g = g; this.cap = cap; this.entries = 0; }

  parse(input) {
    this.entries = 0;
    try {
      const ends = this.deriveSym(this.g.start, 0, input);
      return { parseCount: ends.get(input.length) ?? 0, entries: this.entries, capped: false };
    } catch (e) {
      if (e === "capped") return { parseCount: -1, entries: this.entries, capped: true };
      throw e;
    }
  }

  deriveSym(sym, pos, input) {
    if (!this.g.isNT(sym)) {
      const out = new Map();
      if (pos < input.length && input[pos] === sym) out.set(pos + 1, 1);
      return out;
    }
    const out = new Map();
    for (const rhs of this.g.prods.get(sym)) {
      if (++this.entries > this.cap) throw "capped";
      const r = this.deriveSeq(rhs, pos, input);
      for (const [e, w] of r) out.set(e, (out.get(e) ?? 0) + w);
    }
    return out;
  }

  deriveSeq(seq, pos, input) {
    let cur = new Map([[pos, 1]]);
    for (const sym of seq) {
      const nxt = new Map();
      for (const [p, ways] of cur) {
        const r = this.deriveSym(sym, p, input);
        for (const [e, w] of r) nxt.set(e, (nxt.get(e) ?? 0) + ways * w);
      }
      cur = nxt;
      if (cur.size === 0) break;
    }
    return cur;
  }
}
