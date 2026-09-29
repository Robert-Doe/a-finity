// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar, productionToString } from "./grammar.mjs";
import { Predict, setStr } from "./predict.mjs";

const FILES = [
  "fixtures/expr.grammar",
  "fixtures/stmts.grammar",
  "fixtures/prefix.grammar",
  "fixtures/danglingelse.grammar",
];

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 20 - The Predictive Parsing Condition ===");
p();
p("PREDICT(A -> a) = FIRST(a)                        if a is not nullable");
p("               = FIRST(a)\\{epsilon} U FOLLOW(A)   if a is nullable");
p("LL(1)  <=>  for every nonterminal, the PREDICT sets of its productions");
p("            are pairwise disjoint.");
p();

for (const file of FILES) {
  const g = Grammar.parse(readFileSync(file, "utf8"));
  const pr = new Predict(g);

  p("########## " + file);
  p();
  p("grammar:");
  for (const prod of g.productions) p("  (" + prod.index + ") " + productionToString(prod));
  p("  start: " + g.start);
  p("  terminals: " + setStr(g.terminals()));
  p();

  p("NT".padEnd(6) + " " + "FIRST".padEnd(26) + " " + "FOLLOW");
  for (const nt of g.nonterminals)
    p(nt.padEnd(6) + " " + setStr(pr.firstOf(nt)).padEnd(26) + " " + setStr(pr.followOf(nt)));
  p();

  p("PREDICT sets:");
  for (const prod of g.productions) {
    const pred = pr.predict(prod);
    const tag = pr.nullable(prod.rhs) ? "  (nullable: adds FOLLOW)" : "";
    p("  (" + prod.index + ") " + productionToString(prod).padEnd(22) + "  " + setStr(pred) + tag);
  }
  p();

  const cs = pr.conflicts();
  if (cs.length === 0) {
    p("VERDICT: LL(1) = YES   every nonterminal's PREDICT sets are disjoint");
    p("         -> predictive recursive descent works, no backtracking");
  } else {
    p("VERDICT: LL(1) = NO    " + cs.length + " conflict(s):");
    for (const c of cs) p("  - " + c.toString());
    sb += hintFor(file);
  }
  p();
}

function hintFor(file) {
  if (file.includes("prefix"))
    return "  -> the alternatives share a prefix; LEFT-FACTOR them (Module 24):\n" +
           "        S -> a b S'      S' -> c | d\n";
  if (file.includes("danglingelse"))
    return "  -> classic dangling-else. A predictive parser resolves it by always\n" +
           "     choosing  X -> else S  when the token is 'else' (else binds to the\n" +
           "     nearest if). Modules 25/35 formalise this as a resolved conflict.\n";
  return "";
}

process.stdout.write(sb);
