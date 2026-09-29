// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { Predict, setStr, END } from "./predict.mjs";
import { LL1Table, compact } from "./ll1table.mjs";

const FILES = [
  "fixtures/expr.grammar",
  "fixtures/stmts.grammar",
  "fixtures/danglingelse.grammar",
];

let sb = "";
const p = (s = "") => { sb += s + "\n"; };
const d2 = n => String(n).padStart(2);
const padEnd = (s, w) => { s = String(s); while (s.length < w) s += " "; return s; };

p("=== Module 25 - The LL(1) Parsing Table ===");
p();
p("M[A][t] = the production to apply when expanding A with lookahead t.");
p("build: for each A -> alpha, put it in M[A][t] for every t in PREDICT(A -> alpha).");
p("a cell that gets two productions is a CONFLICT  <=>  grammar is not LL(1).");
p();

for (const file of FILES) {
  const g = Grammar.parse(readFileSync(file, "utf8"));
  const tbl = new LL1Table(g);

  p("########## " + file);
  p();
  p("productions:");
  for (const prod of g.productions) p("  " + compact(prod));
  p();

  p("PREDICT sets:");
  for (const prod of g.productions) {
    const lhsArrow = prod.lhs + " -> " + (prod.rhs.length === 0 ? "epsilon" : prod.rhs.join(" "));
    p("  (" + prod.index + ") " + padEnd(lhsArrow, 20) + " " + setStr(tbl.pr.predict(prod)));
  }
  p();

  p("table (cells = production index; . = blank, ! = conflict):");
  for (const line of tbl.render().split("\n").filter(l => l !== "")) p("  " + line);
  p();

  if (tbl.isLL1()) {
    p("VERDICT: LL(1) = YES   every cell has at most one production");
  } else {
    p("VERDICT: LL(1) = NO    " + tbl.conflicts().length + " conflicted cell(s):");
    for (const c of tbl.conflicts()) {
      p("  M[" + c.nt + "][" + c.token + "] wants:");
      for (const prod of c.ps) p("      " + compact(prod));
    }
  }
  p();
}

p("using the expr table to parse  id + id * id :");
traceParse(Grammar.parse(readFileSync("fixtures/expr.grammar", "utf8")), ["id", "+", "id", "*", "id"]);

process.stdout.write(sb);

function traceParse(g, input) {
  const tbl = new LL1Table(g);
  const stack = [END, g.start];
  const inp = [...input, END];
  let ip = 0;
  let step = 0;

  while (stack.length > 0) {
    const top = stack[stack.length - 1];
    const look = inp[ip];
    if (top === END && look === END) {
      p("  " + d2(step++) + "  stack top " + padEnd(top, 4) + "  look " + padEnd(look, 4) + "  ACCEPT");
      break;
    }
    if (!g.isNonterminal(top)) {
      p("  " + d2(step++) + "  match " + top);
      stack.pop(); ip++;
      continue;
    }
    const ps = tbl.cell(top, look);
    if (ps.length !== 1) {
      p("  " + d2(step++) + "  M[" + top + "][" + look + "] empty -> syntax error");
      break;
    }
    const prod = ps[0];
    p("  " + d2(step++) + "  expand " + compact(prod));
    stack.pop();
    for (let k = prod.rhs.length - 1; k >= 0; k--) stack.push(prod.rhs[k]);
  }
}
