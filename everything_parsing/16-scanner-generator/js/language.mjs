// Module 02 — a language is a (here: finite) set of strings; the operations on
// languages are set operations plus string glue.
//
//   Sigma    an alphabet — a finite set of symbols (characters)
//   ""       the empty string, epsilon; length 0; a real string
//   Language any set of strings over Sigma
//
// Strings are kept in a canonical order — shorter first, then lexicographic —
// so every rendering is deterministic and matches the Java build byte for byte.

export const ORDER = (x, y) =>
  x.length !== y.length ? x.length - y.length : (x < y ? -1 : x > y ? 1 : 0);

export class Language {
  constructor(iterable = []) {
    this._set = new Set(iterable);
  }

  static of(...ws) { return new Language(ws); }

  get size() { return this._set.size; }
  contains(w) { return this._set.has(w); }
  list() { return [...this._set].sort(ORDER); }

  // ── the operations ──

  union(o) {
    return new Language([...this._set, ...o._set]);
  }

  // { xy : x in this, y in o }
  concat(o) {
    const out = new Set();
    for (const x of this._set) for (const y of o._set) out.add(x + y);
    return new Language(out);
  }

  // this concatenated with itself n times; power(0) = { epsilon }
  power(n) {
    if (n < 0) throw new Error("power exponent must be >= 0");
    let result = Language.EPSILON;
    for (let i = 0; i < n; i++) result = result.concat(this);
    return result;
  }

  // Kleene star, bounded: every string of L* of length <= maxLen.
  // Breadth-first closure from { epsilon }; the seen-set makes it terminate
  // even when this language itself contains epsilon.
  star(maxLen) {
    const acc = new Set([""]);
    const work = [""];
    while (work.length > 0) {
      const prefix = work.shift();
      for (const s of this._set) {
        const next = prefix + s;
        if (next.length > maxLen) continue;
        if (!acc.has(next)) { acc.add(next); work.push(next); }
      }
    }
    return new Language(acc);
  }

  intersect(o) {
    return new Language([...this._set].filter(w => o._set.has(w)));
  }

  minus(o) {
    return new Language([...this._set].filter(w => !o._set.has(w)));
  }

  // Keep only strings of length <= maxLen.
  truncate(maxLen) {
    return new Language([...this._set].filter(w => w.length <= maxLen));
  }

  // Sigma* up to a length bound: the bounded Kleene star of the alphabet.
  static sigmaStar(alphabet, maxLen) {
    return new Language([...alphabet].map(String)).star(maxLen);
  }

  // "{ }" for empty; "{ epsilon, a, ab }" otherwise
  render() {
    if (this._set.size === 0) return "{ }";
    return "{ " + this.list().map(w => (w === "" ? "epsilon" : w)).join(", ") + " }";
  }
}

Language.EMPTY = new Language([]);
Language.EPSILON = new Language([""]);
