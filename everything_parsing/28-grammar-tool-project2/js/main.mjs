// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { GrammarTool } from "./grammartool.mjs";

const FILES = [
  "fixtures/stmts.grammar",
  "fixtures/expr.grammar",
  "fixtures/json.grammar",
];

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 28 - Grammar Analysis Tool (CSE 340 Project 2) ===");
p();
p("one program, seven sections, each a component from Modules 20-27:");
p("symbols | nullable | FIRST | FOLLOW | left-recursion-free | left-factored | LL(1)");
p();

for (const file of FILES) {
  const g = Grammar.parse(readFileSync(file, "utf8"));
  const tool = new GrammarTool(g);

  p("##################### " + file + " #####################");
  p();
  p("input grammar:");
  for (const prod of g.productions)
    p("  " + prod.lhs + " -> " + (prod.rhs.length === 0 ? "epsilon" : prod.rhs.join(" ")));
  p();
  sb += tool.report();
  p();
}

process.stdout.write(sb);
