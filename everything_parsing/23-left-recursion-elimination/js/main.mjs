// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { G, paull, hasLeftRecursion } from "./leftrec.mjs";
import { upTo } from "./language.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 23 - Left Recursion Elimination ===");
p();
p("direct:   A -> A a | b        ==>   A -> b A'    A' -> a A' | epsilon");
p("indirect: Paull's algorithm -- fix an order, substitute lower nonterminals");
p("          into higher ones, then remove direct left recursion.");
p();

section("fixtures/expr.grammar", 6);
section("fixtures/indirect.grammar", 6);

process.stdout.write(sb);

function eqSet(a, b) {
  if (a.size !== b.size) return false;
  for (const x of a) if (!b.has(x)) return false;
  return true;
}

function section(file, maxLen) {
  const src = Grammar.parse(readFileSync(file, "utf8"));
  const before = G.from(src);
  const after = paull(src);

  p("########## " + file);
  p();
  p("BEFORE:");
  for (const line of before.text().split("\n").filter(l => l !== "")) p("  " + line);
  p("  left-recursive? " + hasLeftRecursion(before));
  p();

  p("AFTER (Paull):");
  for (const line of after.text().split("\n").filter(l => l !== "")) p("  " + line);
  p("  left-recursive? " + hasLeftRecursion(after));
  p("  start symbol: " + after.start);
  p();

  const lb = upTo(before, maxLen);
  const la = upTo(after, maxLen);
  p("language check (all strings up to length " + maxLen + "):");
  p("  before: " + lb.size + " strings");
  p("  after : " + la.size + " strings");
  p("  identical? " + eqSet(lb, la));
  if (!eqSet(lb, la)) {
    const onlyBefore = [...lb].filter(x => !la.has(x)).sort();
    const onlyAfter = [...la].filter(x => !lb.has(x)).sort();
    p("  only before: [" + onlyBefore.join(", ") + "]");
    p("  only after : [" + onlyAfter.join(", ") + "]");
  } else {
    let line = "  sample: ";
    [...la].sort().slice(0, 8).forEach(s => { line += '"' + s + '"  '; });
    p(line);
  }
  p();
}
