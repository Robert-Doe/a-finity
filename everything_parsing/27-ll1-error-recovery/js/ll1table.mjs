// Module 25 — The LL(1) Parsing Table (mirror of java/LL1Table.java).
//
// M[A][t] = the production to apply when expanding A with lookahead t.
// Build: for each A -> alpha, for each t in PREDICT(A -> alpha), set M[A][t].
// A cell with two productions is a CONFLICT <=> grammar is not LL(1).

import { Predict, setStr, END } from "./predict.mjs";

export class LL1Table {
  constructor(g) {
    this.g = g;
    this.pr = new Predict(g);

    const cols = new Set(g.terminals());
    cols.add(END);
    this.columns = [...cols];

    this.M = new Map();
    for (const nt of g.nonterminals) this.M.set(nt, new Map());
    for (const p of g.productions)
      for (const t of this.pr.predict(p)) {
        const row = this.M.get(p.lhs);
        if (!row.has(t)) row.set(t, []);
        row.get(t).push(p);
      }
  }

  cell(nt, t) {
    const row = this.M.get(nt);
    return row && row.has(t) ? row.get(t) : [];
  }

  conflicts() {
    const out = [];
    for (const nt of this.g.nonterminals)
      for (const [tok, ps] of this.M.get(nt))
        if (ps.length > 1) out.push({ nt, token: tok, ps });
    return out;
  }

  isLL1() { return this.conflicts().length === 0; }

  render() {
    let w = 4;
    for (const c of this.columns) w = Math.max(w, c.length + 1);

    let sb = pad("", 6);
    for (const c of this.columns) sb += pad(c, w);
    sb += "\n";
    for (const nt of this.g.nonterminals) {
      sb += pad(nt, 6);
      for (const c of this.columns) {
        const ps = this.cell(nt, c);
        const txt = ps.length === 0 ? "." : ps.length === 1 ? String(ps[0].index) : "!";
        sb += pad(txt, w);
      }
      sb += "\n";
    }
    return sb;
  }
}

export function compact(p) {
  return "(" + p.index + ") " + p.lhs + " -> " + (p.rhs.length === 0 ? "epsilon" : p.rhs.join(" "));
}

function pad(s, w) {
  while (s.length < w) s += " ";
  return s;
}
