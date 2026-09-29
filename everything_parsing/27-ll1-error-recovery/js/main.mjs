// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { RecoveringParser, compact, setStr } from "./recoveringparser.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 27 - LL(1) Error Recovery: Panic Mode & Phrase-Level ===");
p();
p("terminal mismatch on top   -> phrase-level: insert the expected token, keep the input.");
p("M[A][t] blank/conflicted   -> panic mode:");
p("    t in FOLLOW(A) (or $)   -> pop A (skip the nonterminal)");
p("    otherwise              -> discard t, retry");
p("one run, every error. every branch pops the stack or advances the input -> it stops.");
p();

const g = Grammar.parse(readFileSync("fixtures/expr.grammar", "utf8"));
const parser = new RecoveringParser(g);

p("grammar:");
for (const prod of g.productions) p("  " + compact(prod));
p();
p("synchronizing sets (= FOLLOW):");
for (const nt of g.nonterminals) p("  FOLLOW(" + nt + ") = " + setStr(parser.syncSet(nt)));
p();

for (const line of readFileSync("fixtures/inputs.txt", "utf8").split(/\r?\n/)) {
  if (line.trim() === "" || line.startsWith("#")) continue;
  const toks = line.trim().split(/\s+/);

  p("--- " + line + " ---");
  const r = parser.parse(toks);
  for (const t of r.trace) p(t);

  if (r.clean()) {
    p("  RESULT: accepted, no errors");
  } else {
    p("  RESULT: " + r.errors.length + " error(s), " +
      (r.accepted ? "recovered to end of input" : "gave up") + ":");
    for (const e of r.errors) p("    @" + e.pos + "  " + e.message);
  }
  p();
}

process.stdout.write(sb);
