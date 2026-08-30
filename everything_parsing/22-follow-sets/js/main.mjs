// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { setStr as firstSetStr } from "./firstsets.mjs";
import { FollowSets, setStr } from "./followsets.mjs";
import { Predict } from "./predict.mjs";

const FILES = [
  "fixtures/abc.grammar",
  "fixtures/danglingelse.grammar",
  "fixtures/expr.grammar",
];

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 22 - FOLLOW Sets (least fixed point, given FIRST) ===");
p();
p("rule 1:  $ in FOLLOW(start)");
p("rule 2:  B -> a A b            =>  FIRST(b)\\{epsilon}  in FOLLOW(A)");
p("rule 3:  B -> a A b, b nullable =>  FOLLOW(B)          in FOLLOW(A)");
p("         (b nullable includes b empty, i.e. A is the last symbol)");
p();

for (const file of FILES) {
  const g = Grammar.parse(readFileSync(file, "utf8"));
  const fs = new FollowSets(g);

  p("########## " + file);
  p();
  for (const prod of g.productions)
    p("  (" + prod.index + ") " + prod.lhs + " -> " + (prod.rhs.length === 0 ? "epsilon" : prod.rhs.join(" ")));
  p();

  p("FIRST (from Module 21):");
  for (const nt of g.nonterminals)
    p("      FIRST(" + nt.padEnd(2) + ") = " + firstSetStr(fs.first.firstOf(nt)));
  p("  nullable: " + firstSetStr(fs.first.nullable));
  p();

  p("FOLLOW iteration:");
  fs.rounds.forEach((round, i) => {
    const last = i === fs.rounds.length - 1;
    p("  round " + i + (last ? "  (stable -- one more pass adds nothing):" : ":"));
    for (const nt of g.nonterminals)
      p("      FOLLOW(" + nt.padEnd(2) + ") = " + setStr(round.get(nt)));
  });
  p("  one more pass changes nothing? " + fs.isStable());
  p();

  const pr = new Predict(g);
  let agree = true;
  for (const nt of g.nonterminals) {
    const a = [...fs.followOf(nt)].sort().join(",");
    const b = [...pr.followOf(nt)].sort().join(",");
    if (a !== b) agree = false;
  }
  p("cross-check vs Module 20 Predict.followOf: " + (agree ? "MATCH" : "MISMATCH"));
  p();
}

p("why $ is not optional:");
p("  without $, FOLLOW(start) starts empty. A nullable start rule (or any");
p("  nullable rule that can sit at end of input) would then have an empty");
p("  PREDICT contribution from FOLLOW, and the parser could not tell that");
p("  'take the epsilon production and finish' is legal when the input runs out.");
p("  $ is a real terminal the driver appends to the token stream (Module 26).");

process.stdout.write(sb);
