/**
 * Ported from everything_parsing/25-ll1-table/js/ll1table.mjs.
 *
 * M[A][t] = the production to apply when expanding A with lookahead t.
 * Build: for each A -> alpha, for each t in PREDICT(A -> alpha), set M[A][t].
 * A cell with two productions is a CONFLICT <=> the grammar is not LL(1).
 */
import { Grammar, Production, END } from "./grammar";
import { Predict } from "./predict";

export class LL1Table {
  pr: Predict;
  columns: string[];
  M = new Map<string, Map<string, Production[]>>();

  constructor(public g: Grammar) {
    this.pr = new Predict(g);

    const cols = new Set(g.terminals());
    cols.add(END);
    this.columns = [...cols];

    for (const nt of g.nonterminals) this.M.set(nt, new Map());
    for (const p of g.productions) {
      for (const t of this.pr.predict(p)) {
        const row = this.M.get(p.lhs)!;
        if (!row.has(t)) row.set(t, []);
        row.get(t)!.push(p);
      }
    }
  }

  cell(nt: string, t: string): Production[] {
    const row = this.M.get(nt);
    return row && row.has(t) ? row.get(t)! : [];
  }
}
