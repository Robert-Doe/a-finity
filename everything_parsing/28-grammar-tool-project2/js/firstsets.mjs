// Module 21 — Nullable and FIRST as LEAST FIXED POINTS (mirror of java/FirstSets.java).
//
// NULLABLE = smallest set N with:  A in N if A -> epsilon;  A in N if
// A -> X1..Xk and every Xi in N.  Start empty, apply until a pass adds nothing.
// FIRST is the same shape over sets of terminals. Both record every round.

export const EPS = "epsilon";

export class FirstSets {
  constructor(g) {
    this.g = g;
    this.nullableRounds = [];
    this.firstRounds = [];
    this.nullable = this._computeNullable();
    this.first = this._computeFirst();
  }

  _computeNullable() {
    const n = new Set();
    this.nullableRounds.push(new Set(n));
    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        if (n.has(p.lhs)) continue;
        let allNull = true;
        for (const s of p.rhs) { if (!n.has(s)) { allNull = false; break; } }
        if (allNull) { n.add(p.lhs); changed = true; }
      }
      this.nullableRounds.push(new Set(n));
    }
    return n;
  }

  nullableSeq(seq) {
    for (const s of seq) if (!this.nullable.has(s)) return false;
    return true;
  }

  _computeFirst() {
    const f = new Map();
    for (const nt of this.g.nonterminals) f.set(nt, new Set());
    this.firstRounds.push(snapshot(f));

    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        const target = f.get(p.lhs);
        const before = target.size;
        for (const x of this._firstOfSeqWith(f, p.rhs)) target.add(x);
        if (target.size !== before) changed = true;
      }
      this.firstRounds.push(snapshot(f));
    }
    return f;
  }

  _firstOfSeqWith(f, seq) {
    const out = new Set();
    let allNullable = true;
    for (const sym of seq) {
      if (!this.g.isNonterminal(sym)) { out.add(sym); allNullable = false; break; }
      for (const x of f.get(sym)) if (x !== EPS) out.add(x);
      if (!this.nullable.has(sym)) { allNullable = false; break; }
    }
    if (allNullable) out.add(EPS);
    return out;
  }

  firstOfSeq(seq) { return this._firstOfSeqWith(this.first, seq); }

  firstOf(sym) {
    if (!this.g.isNonterminal(sym)) return new Set([sym]);
    return new Set(this.first.get(sym));
  }

  firstIsStable() {
    const f = snapshot(this.first);
    for (const p of this.g.productions)
      for (const x of this._firstOfSeqWith(f, p.rhs)) f.get(p.lhs).add(x);
    for (const nt of this.g.nonterminals)
      if (!eqSet(f.get(nt), this.first.get(nt))) return false;
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
    const ae = a === EPS, be = b === EPS;
    if (ae !== be) return ae ? 1 : -1;
    return a < b ? -1 : a > b ? 1 : 0;
  });
  return "{ " + items.join(" ") + " }";
}
