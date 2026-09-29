// node js/main.mjs
//
// No fixture file — this module's "input" is the algebra. Same output bytes as
// the Java build.

import { Language } from "./language.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 02 - Alphabets, Strings, Languages, Operations ===");
p();

// ── the alphabet, and Sigma* ──
const sigma = ["a", "b"];
p("alphabet Sigma = { a, b }");
const sigmaStar3 = Language.sigmaStar(sigma, 3);
p("Sigma* up to length 3  (bounded Kleene star of the alphabet):");
p(`  ${sigmaStar3.render()}   [${sigmaStar3.size} strings]`);
p();

// ── union / concat ──
const l1 = Language.of("a", "b");
const l2 = Language.of("c");
p("L1 = " + l1.render());
p("L2 = " + l2.render());
p("L1 union L2  = " + l1.union(l2).render());
p("L1 concat L2 = " + l1.concat(l2).render());
p("L2 concat L1 = " + l2.concat(l1).render());
p();

// ── closure: results of finite operands are finite ──
const p3 = l1.power(3);
p("closure of finite languages:");
p("  L1 union L2 : " + l1.union(l2).size + " strings");
p("  L1 concat L2: " + l1.concat(l2).size + " strings");
p(`  L1 power 3  : ${p3.render()}  (${p3.size})`);
p();

// ── {epsilon} is not {} ──
p("the empty string is not the empty language:");
p("  {}          size " + Language.EMPTY.size);
p("  { epsilon }  size " + Language.EPSILON.size);
p("  {} concat L1        = " + Language.EMPTY.concat(l1).render() + "   (annihilator)");
p("  { epsilon } concat L1 = " + Language.EPSILON.concat(l1).render() + "   (identity)");
p();

// ── Kleene star of a single-string language ──
const aStar = Language.of("a").star(4);
p(`Kleene star of { a } up to length 4:  ${aStar.render()}   [${aStar.size} strings]`);
p();

// ── a regular expression, rebuilt from set operations ──
const firstChar = Language.of("a").union(Language.of("b"));
const tail = Language.of("a").star(3);
const re4 = firstChar.concat(tail).intersect(Language.sigmaStar(sigma, 4));
p("regex as operations:  (a|b) a*   up to length 4");
p("  built as: ( {a} union {b} ) concat ( {a}.star(3) )");
p(`  = ${re4.render()}   [${re4.size} strings]`);
p("  membership:  " + q("b", re4) + "   " + q("ba", re4) + "   " + q("ab", re4) + "   " + q("", re4));
p();

p("summary: operations=union,concat,power,star  " +
  `epsilonLang!=emptyLang=${Language.EPSILON.size !== Language.EMPTY.size}  ` +
  `regexReproduced=${re4.contains("b") && re4.contains("ba") && !re4.contains("ab")}`);

process.stdout.write(sb);

function q(w, l) {
  const shown = w === "" ? '""' : `"${w}"`;
  return shown + (l.contains(w) ? " in L" : " not in L");
}
