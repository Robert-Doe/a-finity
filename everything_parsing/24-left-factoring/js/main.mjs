// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { G } from "./leftrec.mjs";
import { factor, needsFactoring } from "./leftfactor.mjs";
import { upTo } from "./language.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 24 - Left Factoring ===");
p();
p("A -> a b1 | a b2 | rest      ==>      A  -> a A' | rest");
p("                                     A' -> b1 | b2      (empty bi -> A' -> epsilon)");
p("repeat until no nonterminal has two alternatives with the same first symbol.");
p();

section("fixtures/danglingelse.grammar", 8);
section("fixtures/nested.grammar", 4);
section("fixtures/decl.grammar", 6);

process.stdout.write(sb);

function eqSet(a, b) {
  if (a.size !== b.size) return false;
  for (const x of a) if (!b.has(x)) return false;
  return true;
}

function section(file, maxLen) {
  const src = Grammar.parse(readFileSync(file, "utf8"));
  const before = G.from(src);
  const after = factor(src);

  p("########## " + file);
  p();
  p("BEFORE:");
  for (const line of before.text().split("\n").filter(l => l !== "")) p("  " + line);
  p("  needs factoring? " + needsFactoring(before));
  p();

  p("AFTER:");
  for (const line of after.text().split("\n").filter(l => l !== "")) p("  " + line);
  p("  needs factoring? " + needsFactoring(after));
  p();

  const lb = upTo(before, maxLen);
  const la = upTo(after, maxLen);
  p("language check (all strings up to length " + maxLen + "):");
  p("  before: " + lb.size + "   after: " + la.size + "   identical? " + eqSet(lb, la));
  if (!eqSet(lb, la)) {
    p("  only before: [" + [...lb].filter(x => !la.has(x)).sort().join(", ") + "]");
    p("  only after : [" + [...la].filter(x => !lb.has(x)).sort().join(", ") + "]");
  }
  p();
}
