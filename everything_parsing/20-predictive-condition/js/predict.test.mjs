// node js/predict.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { Predict } from "./predict.mjs";

console.log("predict.test.mjs");

const load = g => new Predict(Grammar.parse(g));
const s = set => JSON.stringify([...set].sort());

const e = load(`
%start E
E  -> T Ep
Ep -> + T Ep | epsilon
T  -> F Tp
Tp -> * F Tp | epsilon
F  -> ( E ) | num | id
`);

// ── FIRST ──
equals(s(e.firstOf("E")), s(new Set(["(", "id", "num"])), "FIRST(E) = { ( id num }");
equals(s(e.firstOf("Ep")), s(new Set(["+", "epsilon"])), "FIRST(Ep) = { + epsilon }");
equals(s(e.firstOf("Tp")), s(new Set(["*", "epsilon"])), "FIRST(Tp) = { * epsilon }");
that(e.nullable(["Ep"]), "Ep is nullable");
that(!e.nullable(["F"]), "F is not nullable");

// ── FOLLOW ──
equals(s(e.followOf("E")), s(new Set(["$", ")"])), "FOLLOW(E) = { $ ) }");
equals(s(e.followOf("T")), s(new Set(["$", ")", "+"])), "FOLLOW(T) = { $ ) + }");
equals(s(e.followOf("F")), s(new Set(["$", ")", "*", "+"])), "FOLLOW(F) = { $ ) * + }");

// ── verdict ──
that(e.isLL1(), "the Module 19 expression grammar IS LL(1)");
equals(e.conflicts().length, 0, "no conflicts in the expression grammar");

// ── nullable list rule stays LL(1) ──
const st = load(`
%start P
P -> L
L -> S L | epsilon
S -> id assign E semi
E -> id | num
`);
that(st.isLL1(), "statement-list grammar with a nullable L is LL(1)");
equals(s(st.followOf("L")), s(new Set(["$"])), "FOLLOW(L) = { $ }");
equals(s(st.predict(st.g.productionsFor("L")[0])), s(new Set(["id"])), "PREDICT(L -> S L) = { id }");

// ── common prefix: NOT LL(1) ──
const pf = load("S -> a b c | a b d\n");
that(!pf.isLL1(), "S -> a b c | a b d is not LL(1)");
equals(pf.conflicts().length, 1, "exactly one conflict");
equals(pf.conflicts()[0].token, "a", "the conflict token is 'a'");

// ── dangling else ──
const de = load(`
%start S
S -> if b then S X | other
X -> else S | epsilon
`);
equals(s(de.followOf("X")), s(new Set(["$", "else"])), "FOLLOW(X) = { $ else }");
that(!de.isLL1(), "dangling-else grammar is not LL(1)");
equals(de.conflicts()[0].token, "else", "the conflict is on 'else'");
equals(de.conflicts()[0].nonterminal, "X", "the conflicted nonterminal is X");

// ── left recursion ──
const lr = load("A -> A x | y\n");
equals(s(lr.firstOf("A")), s(new Set(["y"])), "FIRST(A) for A -> A x | y is { y }");
that(!lr.isLL1(), "a left-recursive grammar is not LL(1)");

summary();
