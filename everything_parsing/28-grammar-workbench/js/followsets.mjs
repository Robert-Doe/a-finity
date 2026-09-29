// Module 22 — FOLLOW as a LEAST FIXED POINT, given FIRST (mirror of java/FollowSets.java).
//
//   1.  $ in FOLLOW(start)
//   2.  B -> a A b            =>  FIRST(b)\{epsilon}  subset of FOLLOW(A)
//   3.  B -> a A b, b nullable =>  FOLLOW(B)          subset of FOLLOW(A)
//
// Rule 3 makes FOLLOW circular; start empty, add only, stop when a pass changes
// nothing. FIRST comes from Module 21's FirstSets, used as a fixed input.

import { FirstSets } from "./firstsets.mjs";

export const EPS = "epsilon";
export const END = "$";

export class FollowSets {
  constructor(g) {
    this.g = g;
    this.first = new FirstSets(g);
    this.rounds = [];
    this.follow = this._compute();
  }

  _compute() {
    const fol = new Map();
    for (const nt of this.g.nonterminals) fol.set(nt, new Set());
    fol.get(this.g.start).add(END);
    this.rounds.push(snapshot(fol));

    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        const rhs = p.rhs;
        for (let i = 0; i < rhs.length; i++) {
          const a = rhs[i];
          if (!this.g.isNonterminal(a)) continue;
          const beta = rhs.slice(i + 1);
          const fb = this.first.firstOfSeq(beta);

          const target = fol.get(a);
          const before = target.size;
          for (const x of fb) if (x !== EPS) target.add(x);
          if (fb.has(EPS)) for (const x of fol.get(p.lhs)) target.add(x);
          if (target.size !== before) changed = true;
        }
      }
      this.rounds.push(snapshot(fol));
    }
    return fol;
  }

  followOf(nt) { return new Set(this.follow.get(nt) ?? []); }

  isStable() {
    const f = snapshot(this.follow);
    for (const p of this.g.productions) {
      const rhs = p.rhs;
      for (let i = 0; i < rhs.length; i++) {
        const a = rhs[i];
        if (!this.g.isNonterminal(a)) continue;
        const fb = this.first.firstOfSeq(rhs.slice(i + 1));
        for (const x of fb) if (x !== EPS) f.get(a).add(x);
        if (fb.has(EPS)) for (const x of f.get(p.lhs)) f.get(a).add(x);
      }
    }
    for (const nt of this.g.nonterminals)
      if (!eqSet(f.get(nt), this.follow.get(nt))) return false;
    return true;
  }
}

function snapshot(f) {
  const s = new Map();
  for (const [k, v] of f) s.set(k, new Set(v));
  return s;
}
function eqSet(a, b) {
  if (a.size !== b.size) return false;
  for (const x of a) if (!b.has(x)) return false;
  return true;
}

export function setStr(s) {
  const items = [...s].sort((a, b) => {
    const ae = a === END, be = b === END;
    if (ae !== be) return ae ? 1 : -1;
    return a < b ? -1 : a > b ? 1 : 0;
  });
  return "{ " + items.join(" ") + " }";
}
