// Module 29 — FIRST_k sets and the minimal-k question (mirror of java/FirstK.java).
//
// A k-token prefix is stored as a space-joined string ("" = the empty prefix).
// FIRST_1 is the ordinary FIRST set. A grammar is "LL(k) by this test" when every
// multi-production nonterminal has pairwise-disjoint FIRST_k sets on its
// alternatives (k=1 is delegated to Module 20's exact PREDICT check; the
// nullable-alternative case needs FOLLOW_k and is out of scope here).

import { Predict } from "./predict.mjs";

export class FirstK {
  constructor(g) { this.g = g; }

  firstKTable(k) {
    const t = new Map();
    for (const nt of this.g.order) t.set(nt, new Set());
    let changed = true;
    while (changed) {
      changed = false;
      for (const nt of this.g.order) {
        const tgt = t.get(nt);
        const before = tgt.size;
        for (const rhs of this.g.prods.get(nt))
          for (const x of this.firstKOfSeq(rhs, k, t)) tgt.add(x);
        if (tgt.size !== before) changed = true;
      }
    }
    return t;
  }

  firstKOfSeq(seq, k, t) {
    let acc = new Set([""]);
    for (const sym of seq) {
      const symSet = this.g.isNT(sym) ? (t.get(sym) ?? new Set()) : new Set([sym]);
      const next = new Set();
      for (const pre of acc) {
        const preTok = pre === "" ? [] : pre.split(" ");
        if (preTok.length === k) { next.add(pre); continue; }
        for (const s of symSet) {
          const sTok = s === "" ? [] : s.split(" ");
          next.add([...preTok, ...sTok].slice(0, k).join(" "));
        }
      }
      acc = next;
    }
    return acc;
  }

  minLL(maxK, original) {
    if (new Predict(original).isLL1()) return 1;
    for (let k = 2; k <= maxK; k++) if (this.isLLk(k)) return k;
    return -1;
  }

  isLLk(k) {
    const t = this.firstKTable(k);
    for (const nt of this.g.order) {
      const prods = this.g.prods.get(nt);
      if (prods.length < 2) continue;
      const sets = [];
      for (const rhs of prods) {
        if (rhs.length === 0) return false;          // nullable -> FOLLOW_k needed
        sets.push(this.firstKOfSeq(rhs, k, t));
      }
      for (let i = 0; i < sets.length; i++)
        for (let j = i + 1; j < sets.length; j++)
          for (const x of sets[i]) if (sets[j].has(x)) return false;
    }
    return true;
  }
}

export function show(set) {
  const items = [...set].map(s => s === "" ? "()" : s).sort();
  return "{ " + items.join(" , ") + " }";
}
