// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { FirstSets, setStr } from "./firstsets.mjs";
import { Predict } from "./predict.mjs";

const FILES = [
  "fixtures/indirect.grammar",
  "fixtures/cascade.grammar",
  "fixtures/expr.grammar",
];

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 21 - Nullable & FIRST Sets (least fixed points) ===");
p();
p("NULLABLE: start empty; add A when  A -> epsilon  or  A -> X1..Xk with every Xi");
p("          already nullable; repeat until a pass adds nothing.");
p("FIRST   : start empty; fold FIRST(rhs) into FIRST(lhs) for every production;");
p("          repeat to convergence. Circular by nature -> needs the iteration.");
p();

for (const file of FILES) {
  const g = Grammar.parse(readFileSync(file, "utf8"));
  const fs = new FirstSets(g);

  p("########## " + file);
  p();
  for (const prod of g.productions)
    p("  (" + prod.index + ") " + prod.lhs + " -> " + (prod.rhs.length === 0 ? "epsilon" : prod.rhs.join(" ")));
  p();

  p("NULLABLE iteration:");
  fs.nullableRounds.forEach((round, i) => {
    const last = i === fs.nullableRounds.length - 1;
    p("  round " + i + ": " + setStr(round) + (last ? "   (stable -- one more pass adds nothing)" : ""));
  });
  p("  => nullable nonterminals: " + setStr(fs.nullable));
  p();

  p("FIRST iteration:");
  fs.firstRounds.forEach((round, i) => {
    p("  round " + i + ":");
    for (const nt of g.nonterminals)
      p("      FIRST(" + nt.padEnd(2) + ") = " + setStr(round.get(nt)));
  });
  p("  one more pass changes nothing? " + fs.firstIsStable());
  p();

  const pr = new Predict(g);
  let agree = true;
  for (const nt of g.nonterminals) {
    const a = [...fs.firstOf(nt)].sort().join(",");
    const b = [...pr.firstOf(nt)].sort().join(",");
    if (a !== b) agree = false;
  }
  p("cross-check vs Module 20 Predict.firstOf: " + (agree ? "MATCH" : "MISMATCH"));
  p();
}

p("why the LEAST fixed point?");
p("  - the empty assignment is NOT a fixed point: the rules force elements in.");
p("  - our result IS a fixed point: one more pass adds nothing (checked above).");
p("  - we only ever ADD, starting from empty, so every element we put in was");
p("    forced by a rule. Any other fixed point must also contain those elements.");
p("    Therefore ours is contained in every fixed point = the least one.");
p("  - the LEAST one is the right answer: a bigger set would claim a token can");
p("    start a nonterminal when no derivation actually produces it there.");

process.stdout.write(sb);
