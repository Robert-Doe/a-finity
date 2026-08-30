// node js/firstsets.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { FirstSets } from "./firstsets.mjs";
import { Predict } from "./predict.mjs";

console.log("firstsets.test.mjs");

const load = g => new FirstSets(Grammar.parse(g));
const s = set => JSON.stringify([...set].sort());

// ── indirect nullability ──
const ind = load(`
%start X
X -> Y
Y -> Z
Z -> z | epsilon
`);
equals(s(ind.nullable), s(new Set(["X", "Y", "Z"])), "X, Y, Z all nullable (indirectly)");
equals(ind.nullableRounds.length, 5, "nullable: round 0 + 3 productive + 1 confirming");
equals(s(ind.nullableRounds[1]), s(new Set(["Z"])), "round 1 finds only Z");
equals(s(ind.nullableRounds[2]), s(new Set(["Y", "Z"])), "round 2 adds Y");
equals(s(ind.firstOf("X")), s(new Set(["epsilon", "z"])), "FIRST(X) = { z epsilon }");
that(ind.firstIsStable(), "the FIRST result is a fixed point");

// ── nullable cascade ──
const cas = load(`
%start S
S -> A B C d
A -> a | epsilon
B -> b | epsilon
C -> c | epsilon
`);
equals(s(cas.nullable), s(new Set(["A", "B", "C"])), "A B C nullable; S is not");
that(!cas.nullable.has("S"), "S not nullable (the d)");
equals(s(cas.firstOf("S")), s(new Set(["a", "b", "c", "d"])), "FIRST(S) collects a,b,c AND d");
that(!cas.firstOf("S").has("epsilon"), "epsilon NOT in FIRST(S)");

that(cas.nullableSeq(["A", "B", "C"]), "A B C is a nullable sequence");
that(!cas.nullableSeq(["A", "B", "C", "d"]), "A B C d is not");
equals(s(cas.firstOfSeq(["A", "B", "C"])), s(new Set(["a", "b", "c", "epsilon"])),
  "FIRST(A B C) includes epsilon");
equals(s(cas.firstOfSeq(["C", "d"])), s(new Set(["c", "d"])), "FIRST(C d): c, then d because C nullable");

// ── expression grammar ──
const e = load(`
%start E
E  -> T Ep
Ep -> + T Ep | epsilon
T  -> F Tp
Tp -> * F Tp | epsilon
F  -> ( E ) | num | id
`);
equals(s(e.nullable), s(new Set(["Ep", "Tp"])), "only Ep and Tp are nullable");
equals(s(e.firstOf("E")), s(new Set(["(", "id", "num"])), "FIRST(E) resolved through the cycle");
that(e.firstRounds.length >= 4, "FIRST(E) empty until round 3");
equals(s(e.firstRounds[1].get("E")), s(new Set()), "FIRST(E) still empty after round 1");

// ── cross-check against Module 20 ──
const pr = new Predict(e.g);
for (const nt of e.g.nonterminals)
  equals(s(e.firstOf(nt)), s(pr.firstOf(nt)), `FIRST(${nt}) agrees with Module 20`);

summary();
