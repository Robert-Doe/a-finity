// Module 23 — Left Recursion Elimination (mirror of java/LeftRec.java).
//
// DIRECT:   A -> A a | b   ==>   A -> b A' ;  A' -> a A' | epsilon
// INDIRECT: Paull's algorithm — order A1..An; for i, substitute Aj (j<i) into
//           Ai -> Aj g, then eliminate direct left recursion in Ai.

export class G {
  constructor() {
    this.order = [];
    this.prods = new Map();       // nt -> [ [sym,...], ... ]   ([] = epsilon)
    this.start = null;
  }

  isNT(s) { return this.prods.has(s); }

  static from(src) {
    const g = new G();
    g.start = src.start;
    for (const nt of src.nonterminals) { g.order.push(nt); g.prods.set(nt, []); }
    for (const p of src.productions) g.prods.get(p.lhs).push([...p.rhs]);
    return g;
  }

  text() {
    let out = "";
    for (const nt of this.order) {
      const alts = this.prods.get(nt).map(r => r.length === 0 ? "epsilon" : r.join(" "));
      out += nt + " -> " + alts.join(" | ") + "\n";
    }
    return out;
  }
}

export function eliminateDirect(g, a) {
  const rules = g.prods.get(a);
  const alpha = [], beta = [];
  for (const r of rules) {
    if (r.length > 0 && r[0] === a) alpha.push(r.slice(1));
    else beta.push([...r]);
  }
  if (alpha.length === 0) return;

  const ap = fresh(g, a);
  const newA = beta.map(b => [...b, ap]);
  const newAp = alpha.map(al => [...al, ap]);
  newAp.push([]);                              // A' -> epsilon

  g.prods.set(a, dedupe(newA));
  g.prods.set(ap, dedupe(newAp));
  g.order.splice(g.order.indexOf(a) + 1, 0, ap);
}

export function paull(src) {
  const g = G.from(src);
  const a = [...g.order];                      // original nonterminals only
  for (let i = 0; i < a.length; i++) {
    const ai = a[i];
    for (let j = 0; j < i; j++) {
      const aj = a[j];
      // Only substitute Aj into Ai -> Aj g when Aj can leftmost-reach Ai —
      // i.e. when doing so exposes (indirect) left recursion. Otherwise the
      // rule is fine and substituting only bloats the grammar.
      if (!leftmostReaches(g, aj).has(ai)) continue;
      const replaced = [];
      for (const r of g.prods.get(ai)) {
        if (r.length > 0 && r[0] === aj) {
          const tail = r.slice(1);
          for (const d of g.prods.get(aj)) replaced.push([...d, ...tail]);
        } else {
          replaced.push([...r]);
        }
      }
      g.prods.set(ai, dedupe(replaced));
    }
    eliminateDirect(g, ai);
  }
  return g;
}

function leftmostReaches(g, from) {
  const seen = new Set();
  const stack = [];
  for (const r of g.prods.get(from)) if (r.length > 0 && g.isNT(r[0])) stack.push(r[0]);
  while (stack.length > 0) {
    const x = stack.pop();
    if (seen.has(x)) continue;
    seen.add(x);
    for (const r of g.prods.get(x)) if (r.length > 0 && g.isNT(r[0])) stack.push(r[0]);
  }
  return seen;
}

function fresh(g, base) {
  let name = base + "'";
  while (g.prods.has(name)) name += "'";
  return name;
}

function dedupe(inp) {
  const seen = new Set();
  const out = [];
  for (const r of inp) {
    const key = r.join("");
    if (!seen.has(key)) { seen.add(key); out.push(r); }
  }
  return out;
}

export function hasLeftRecursion(g) {
  for (const nt of g.order) {
    const seen = new Set();
    const stack = [];
    for (const r of g.prods.get(nt)) if (r.length > 0 && g.isNT(r[0])) stack.push(r[0]);
    while (stack.length > 0) {
      const x = stack.pop();
      if (x === nt) return true;
      if (seen.has(x)) continue;
      seen.add(x);
      for (const r of g.prods.get(x)) if (r.length > 0 && g.isNT(r[0])) stack.push(r[0]);
    }
  }
  return false;
}
