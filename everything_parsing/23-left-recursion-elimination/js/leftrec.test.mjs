// node js/leftrec.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { G, paull, hasLeftRecursion } from "./leftrec.mjs";
import { upTo } from "./language.mjs";

console.log("leftrec.test.mjs");

const eqSet = (a, b) => a.size === b.size && [...a].every(x => b.has(x));

// ── direct: expression grammar ──
const expr = Grammar.parse(`
%start E
E -> E + T | T
T -> T * F | F
F -> ( E ) | id | num
`);
const before = G.from(expr);
that(hasLeftRecursion(before), "the natural expression grammar is left-recursive");

const after = paull(expr);
that(!hasLeftRecursion(after), "after elimination it is not");
equals(after.text().trim(), [
  "E -> T E'",
  "E' -> + T E' | epsilon",
  "T -> F T'",
  "T' -> * F T' | epsilon",
  "F -> ( E ) | id | num",
].join("\n"), "produces exactly the Module 19 grammar");
equals(after.start, "E", "start symbol preserved");

// ── language preserved (direct) ──
const lb = upTo(before, 7);
const la = upTo(after, 7);
that(eqSet(lb, la), "same language up to length 7");
that(la.has("id + id * id"), "id + id * id still derivable");

// ── indirect: Paull ──
const ind = Grammar.parse(`
%start S
S -> A a | b
A -> A c | S d | e
`);
const indBefore = G.from(ind);
const indAfter = paull(ind);
that(hasLeftRecursion(indBefore), "indirect grammar is (indirectly) left-recursive");
that(!hasLeftRecursion(indAfter), "Paull removes the indirect left recursion");
equals(indAfter.text().trim(), [
  "S -> A a | b",
  "A -> b d A' | e A'",
  "A' -> c A' | a d A' | epsilon",
].join("\n"), "matches the Dragon Book 4.18 result");

const ib = upTo(indBefore, 6);
const ia = upTo(indAfter, 6);
that(eqSet(ib, ia), "indirect: same language up to length 6");

// ── clean grammar untouched ──
const clean = Grammar.parse(`
%start S
S -> a S b | c
`);
equals(paull(clean).text().trim(), "S -> a S b | c", "non-left-recursive grammar unchanged");

summary();
