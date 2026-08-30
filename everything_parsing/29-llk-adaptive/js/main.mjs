// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { G } from "./leftrec.mjs";
import { FirstK, show } from "./firstk.mjs";
import { Backtrack } from "./backtrack.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };
const padEnd = (s, w) => { s = String(s); while (s.length < w) s += " "; return s; };
const rhsText = prod => (prod.rhs.length === 0 ? "epsilon" : prod.rhs.join(" "));

p("=== Module 29 - LL(k), LL(*), and Backtracking RD ===");
p();
p("### PART 1 - one token isn't always enough");
p();

for (const file of ["fixtures/expr.grammar", "fixtures/label.grammar", "fixtures/equiv.grammar"]) {
  const g = Grammar.parse(readFileSync(file, "utf8"));
  const lg = G.from(g);
  const fk = new FirstK(lg);

  p("########## " + file);
  for (const prod of g.productions) p("  " + prod.lhs + " -> " + rhsText(prod));

  for (let k = 1; k <= 2; k++) {
    p("  FIRST_" + k + " per production:");
    const t = fk.firstKTable(k);
    for (const prod of g.productions)
      p("    " + padEnd(prod.lhs + " -> " + rhsText(prod), 22) + " " + show(fk.firstKOfSeq(prod.rhs, k, t)));
  }
  const m = fk.minLL(4, g);
  p("  minimal k: " + (m < 0 ? "none in 1..4 (grammar is ambiguous / not LL(k))" : String(m)));
  p();
}

p("### PART 2 - backtracking recursive descent: the cost of no lookahead");
p();

const equiv = Grammar.parse(readFileSync("fixtures/equiv.grammar", "utf8"));
const equivG = G.from(equiv);
p("equiv.grammar  (A -> x B | x C | y ;  B -> A ;  C -> A)  on  x^n y :");
p("    " + padEnd("n", 6) + " " + padEnd("parse trees", 14) + " " + padEnd("production tries", 16));
for (let n = 1; n <= 7; n++) {
  const inp = [];
  for (let i = 0; i < n; i++) inp.push("x");
  inp.push("y");
  const r = new Backtrack(equivG).parse(inp);
  p("    " + padEnd(n, 6) + " " +
    padEnd(r.capped ? "(capped)" : String(r.parseCount), 14) + " " +
    padEnd(r.capped ? ">cap" : String(r.entries), 16));
}
p("    parse trees double each step (2^n) and so does the work -- no finite k helps.");
p("    packrat parsing (Appendix X2) memoizes (nonterminal, position) -> linear.");
p();

const exprG = Grammar.parse(readFileSync("fixtures/expr.grammar", "utf8"));
const exprLG = G.from(exprG);
p("expr.grammar  (LL(1))  on  id (+ id)^n :");
p("    " + padEnd("tokens", 6) + " " + padEnd("parse trees", 14) + " " + padEnd("production tries", 16));
for (let n = 0; n <= 6; n++) {
  const inp = ["id"];
  for (let i = 0; i < n; i++) { inp.push("+"); inp.push("id"); }
  const r = new Backtrack(exprLG).parse(inp);
  p("    " + padEnd(inp.length, 6) + " " + padEnd(String(r.parseCount), 14) + " " + padEnd(String(r.entries), 16));
}
p("    exactly one parse tree; work grows LINEARLY -- disjoint FIRST sets mean each");
p("    nonterminal's wrong alternatives fail on the first token.");
p();

p("### LL(*) in one line");
p("ANTLR's adaptive LL(*): instead of a fixed k, build a small DFA over the");
p("lookahead that reads as many tokens as needed to pick an alternative --");
p("regular (not bounded) lookahead. Falls back to backtracking only when the");
p("DFA can't decide. For label.grammar the DFA reads id then peeks one more:");
p("  'colon' -> alternative 1 (label) ;  otherwise -> alternative 2 (expr).");

process.stdout.write(sb);
