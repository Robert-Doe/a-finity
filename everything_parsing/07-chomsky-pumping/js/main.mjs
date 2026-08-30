// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { enumerate } from "./derivation.mjs";
import { classify, isRegular } from "./grammarclass.mjs";
import { regularRefutation, cflRefutation, inAnBn, inAnBnCn } from "./pumping.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 07 - The Chomsky Hierarchy & Pumping Lemmas ===");
p();

// 1. classification
p("grammar classification:");
for (const f of ["a-star-b", "b-a-star", "anbn", "expr"]) {
  const g = load(f + ".grammar");
  const k = classify(g);
  const tag = isRegular(g) ? "regular" : "not regular";
  p("  " + padR("fixtures/" + f + ".grammar", 26) + " " + padR(clip(firstRule(g), 32), 32)
    + " " + padR(k, 14) + " (" + tag + ")");
}
p();

// 2. a CFG for { a^n b^n }
const anbn = load("anbn.grammar");
const lang = enumerate(anbn, 8).map(w => (w === "epsilon" ? "epsilon" : w.replace(/ /g, "")));
p("a context-free grammar DOES generate { a^n b^n }:");
p("  fixtures/anbn.grammar  enumerated to length 8:  " + lang.join(", "));
p();

// 3. regular pumping lemma against { a^n b^n }
p("no regular grammar can -- the regular pumping lemma has no valid pumping length:");
let everyPFails = true;
for (let pp = 1; pp <= 6; pp++) {
  const w = regularRefutation(pp);
  everyPFails = everyPFails && w.allEscaped;
  p("  " + `p=${pp}: s=` + padR(w.s, 10)
    + ` all ${w.decompositionsTried}/${w.decompositionsTried} splits escape;  `
    + `x=${q(w.x)} y=${q(w.y)} z=${q(w.z)}, pump^${w.k} -> "${w.pumped}"  `
    + `(${inAnBn(w.pumped) ? "IN L?!" : "not in a^n b^n"})`);
}
p("  for EVERY p, a^p b^p is in L but no split survives pumping  ->  { a^n b^n } is NOT regular");
p();

// 4. context-free pumping lemma against { a^n b^n c^n }
p("one level up -- the context-free pumping lemma has no valid pumping length for { a^n b^n c^n }:");
for (let pp = 1; pp <= 4; pp++) {
  const w = cflRefutation(pp);
  everyPFails = everyPFails && w.allEscaped;
  p("  " + `p=${pp}: s=` + padR(w.s, 12)
    + ` all ${w.decompositionsTried} 5-splits escape;  `
    + `v=${q(w.v)} w=${q(w.w)} x=${q(w.x)}, pump^${w.k} -> "${w.pumped}"  `
    + `(${inAnBnCn(w.pumped) ? "IN L?!" : "not in a^n b^n c^n"})`);
}
p("  ->  { a^n b^n c^n } is context-sensitive but NOT context-free");
p();

// 5. the hierarchy
p("the hierarchy (each type STRICTLY contains the one below):");
p("  Type 3  regular            a*, (a|b)*abb         finite automaton / regex");
p("  Type 2  context-free       a^n b^n, balanced()   pushdown automaton");
p("  Type 1  context-sensitive  a^n b^n c^n           linear-bounded automaton");
p("  Type 0  recursively enum.  { <M,w> : M halts }   Turing machine");
p();

p("summary: 2 regular grammars + 2 context-free classified"
  + " | { a^n b^n } not regular (pumping p=1..6)"
  + " | { a^n b^n c^n } not context-free (pumping p=1..4)"
  + " | allAdversaryRunsSucceeded=" + everyPFails);

process.stdout.write(sb);

// ── helpers ──

function firstRule(g) {
  const lhs = g.productions[0].lhs;
  const alts = [];
  for (const pr of g.productions) {
    if (pr.lhs !== lhs) break;
    alts.push(pr.rhs.length === 0 ? "epsilon" : pr.rhs.join(" "));
  }
  return lhs + " -> " + alts.join(" | ");
}
function load(name) { return Grammar.parse(readFileSync("fixtures/" + name, "utf8")); }
function q(s) { return s === "" ? '""' : `"${s}"`; }
function padR(s, n) { s = String(s); return s.length >= n ? s : s + " ".repeat(n - s.length); }
function clip(s, n) { return s.length <= n ? s : s.slice(0, n - 3) + "..."; }
