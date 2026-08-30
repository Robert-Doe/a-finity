// node js/followsets.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { FollowSets } from "./followsets.mjs";
import { Predict } from "./predict.mjs";

console.log("followsets.test.mjs");

const load = g => new FollowSets(Grammar.parse(g));
const s = set => JSON.stringify([...set].sort());

// ── FOLLOW flows through a nullable tail ──
const abc = load(`
%start S
S -> A B c
A -> a | epsilon
B -> b | epsilon
`);
equals(s(abc.followOf("S")), s(new Set(["$"])), "FOLLOW(S) = { $ }");
equals(s(abc.followOf("A")), s(new Set(["b", "c"])), "FOLLOW(A) = { b c }");
equals(s(abc.followOf("B")), s(new Set(["c"])), "FOLLOW(B) = { c }");
that(abc.isStable(), "FOLLOW result is a fixed point");

// ── dangling else: mutually-dependent FOLLOW ──
const de = load(`
%start S
S -> if b then S X | other
X -> else S | epsilon
`);
equals(s(de.followOf("S")), s(new Set(["$", "else"])), "FOLLOW(S) = { $ else }");
equals(s(de.followOf("X")), s(new Set(["$", "else"])), "FOLLOW(X) = { $ else }");
equals(de.rounds.length, 3, "converges in 1 productive round + confirm + round 0");

// ── expression grammar ──
const e = load(`
%start E
E  -> T Ep
Ep -> + T Ep | epsilon
T  -> F Tp
Tp -> * F Tp | epsilon
F  -> ( E ) | num | id
`);
equals(s(e.followOf("E")), s(new Set(["$", ")"])), "FOLLOW(E) = { $ ) }");
equals(s(e.followOf("Ep")), s(new Set(["$", ")"])), "FOLLOW(Ep) = FOLLOW(E)");
equals(s(e.followOf("T")), s(new Set(["$", ")", "+"])), "FOLLOW(T) picks up +");
equals(s(e.followOf("F")), s(new Set(["$", ")", "*", "+"])), "FOLLOW(F) picks up *");
that(!e.followOf("F").has("epsilon"), "epsilon is never in a FOLLOW set");
that(e.followOf("E").has("$"), "$ seeded into FOLLOW(start)");

// ── cross-check vs Module 20 ──
const pr = new Predict(e.g);
for (const nt of e.g.nonterminals)
  equals(s(e.followOf(nt)), s(pr.followOf(nt)), `FOLLOW(${nt}) agrees with Module 20`);

summary();
