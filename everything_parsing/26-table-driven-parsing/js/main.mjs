// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { TableParser, compact, render } from "./tableparser.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 26 - Table-Driven Predictive Parsing ===");
p();
p("stack + LL(1) table + a loop. no recursion, no per-grammar code.");
p("stack top = nonterminal -> M[top][lookahead] gives the production; pop, push RHS reversed.");
p("stack top = terminal    -> must equal lookahead; pop and advance.");
p("stack top = $ and lookahead = $ -> ACCEPT.");
p();

const g = Grammar.parse(readFileSync("fixtures/expr.grammar", "utf8"));
p("grammar:");
for (const prod of g.productions) p("  " + compact(prod));
p();

const parser = new TableParser(g);

for (const line of readFileSync("fixtures/inputs.txt", "utf8").split(/\r?\n/)) {
  if (line.trim() === "" || line.startsWith("#")) continue;
  const toks = line.trim().split(/\s+/);

  p("--- " + line + " ---");
  const r = parser.parse(toks);
  for (const t of r.trace) p(t);

  if (r.ok) {
    p("  productions (a leftmost derivation):");
    for (const prod of r.productions) p("      " + prod);
    const sform = parser.replay(r.productions);
    p("  replay -> " + sform.join(" "));
    p("  equals input? " + (sform.length === toks.length && sform.every((x, i) => x === toks[i])));
    p("  parse tree:");
    for (const tl of render(r.tree).split("\n")) p("    " + tl);
  } else {
    p("  REJECTED: " + r.error);
  }
  p();
}

p("table-driven == recursive descent: the production sequence above is the");
p("same leftmost derivation Module 19's call stack produces for the same input.");
p("the explicit stack here IS that call stack, made into data.");

process.stdout.write(sb);
