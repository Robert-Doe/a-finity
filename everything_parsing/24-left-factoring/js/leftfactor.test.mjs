// node js/leftfactor.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { G } from "./leftrec.mjs";
import { factor, needsFactoring, longestSharedPrefix } from "./leftfactor.mjs";
import { upTo } from "./language.mjs";

console.log("leftfactor.test.mjs");

const eqSet = (a, b) => a.size === b.size && [...a].every(x => b.has(x));

// ── dangling else ──
const de = Grammar.parse(`
%start S
S -> if b then S | if b then S else S | other
`);
const deBefore = G.from(de);
that(needsFactoring(deBefore), "before: two alts share 'if'");

const deAfter = factor(de);
that(!needsFactoring(deAfter), "after: no shared first symbol");
equals(deAfter.text().trim(), [
  "S -> other | if b then S S'",
  "S' -> epsilon | else S",
].join("\n"), "produces the dangling-else grammar Modules 20/22 analysed");
that(eqSet(upTo(deBefore, 9), upTo(deAfter, 9)), "same language up to length 9");

// ── nested prefixes ──
const ne = Grammar.parse(`
%start A
A -> a b c | a b d | a e
`);
const neBefore = G.from(ne);
const neAfter = factor(ne);
that(!needsFactoring(neAfter), "nested: fully factored");
equals(neAfter.text().trim(), [
  "A -> a A''",
  "A'' -> e | b A'",
  "A' -> c | d",
].join("\n"), "a factored out first, then b");
that(eqSet(upTo(neBefore, 4), upTo(neAfter, 4)), "nested: language preserved");

// ── longestSharedPrefix ──
equals(JSON.stringify(longestSharedPrefix([["a", "b", "c"], ["a", "b", "d"], ["a", "e"]])),
  JSON.stringify(["a", "b"]), "longest shared prefix is [a, b]");

// ── realistic decl ──
const decl = Grammar.parse(`
%start stmt
stmt -> id lp args rp semi | id assign expr semi | id colon type semi
args -> expr | epsilon
expr -> id | num
type -> id
`);
const declAfter = factor(decl);
that(declAfter.text().includes("stmt -> id stmt'"), "id factored out of stmt");
that(!needsFactoring(declAfter), "decl fully factored");
that(eqSet(upTo(G.from(decl), 6), upTo(declAfter, 6)), "decl: language preserved");

// ── nothing to do ──
const clean = Grammar.parse(`
%start S
S -> a X | b Y
X -> x
Y -> y
`);
equals(factor(clean).text().trim(), ["S -> a X | b Y", "X -> x", "Y -> y"].join("\n"),
  "already-factored grammar is unchanged");

summary();
