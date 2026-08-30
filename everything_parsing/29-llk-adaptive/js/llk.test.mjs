// node js/llk.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { G } from "./leftrec.mjs";
import { FirstK, show } from "./firstk.mjs";
import { Backtrack } from "./backtrack.mjs";

console.log("llk.test.mjs");

const lg = g => G.from(Grammar.parse(g));

const exprSrc = `
%start E
E  -> T Ep
Ep -> + T Ep | epsilon
T  -> F Tp
Tp -> * F Tp | epsilon
F  -> ( E ) | id | num
`;

const ek = new FirstK(lg(exprSrc));
equals(show(ek.firstKTable(1).get("F")), "{ ( , id , num }", "FIRST_1(F)");
equals(ek.minLL(4, Grammar.parse(exprSrc)), 1, "expression grammar is LL(1)");

// ── label grammar ──
const labelSrc = `
%start stmt
stmt  -> label colon stmt | expr semi
label -> id
expr  -> id | id lparen rparen
`;
const lk = new FirstK(lg(labelSrc));
const lt1 = lk.firstKTable(1);
equals(show(lk.firstKOfSeq(["label", "colon", "stmt"], 1, lt1)), "{ id }", "FIRST_1(label colon stmt)");
equals(show(lk.firstKOfSeq(["expr", "semi"], 1, lt1)), "{ id }", "FIRST_1(expr semi) -- collision");
const lt2 = lk.firstKTable(2);
equals(show(lk.firstKOfSeq(["label", "colon", "stmt"], 2, lt2)), "{ id colon }", "FIRST_2(label colon stmt)");
equals(show(lk.firstKOfSeq(["expr", "semi"], 2, lt2)), "{ id lparen , id semi }", "FIRST_2(expr semi)");
equals(lk.minLL(4, Grammar.parse(labelSrc)), 2, "label grammar's minimal k is 2");

// ── equiv grammar ──
const equivSrc = `
%start A
A -> x B | x C | y
B -> A
C -> A
`;
equals(new FirstK(lg(equivSrc)).minLL(4, Grammar.parse(equivSrc)), -1, "equiv grammar is not LL(k)");

// ── backtracking cost ──
const eG = lg(equivSrc);
const b3 = new Backtrack(eG).parse(["x", "x", "x", "y"]);
equals(b3.parseCount, 8, "x x x y has 8 parse trees");
const b5 = new Backtrack(eG).parse(["x", "x", "x", "x", "x", "y"]);
equals(b5.parseCount, 32, "x^5 y has 32 parse trees");
that(b5.entries > 2 * b3.entries, "work more than doubles from n=3 to n=5");

const exG = lg(exprSrc);
const shortIn = ["id", "+", "id"];
const longIn = ["id"];
for (let i = 0; i < 10; i++) { longIn.push("+"); longIn.push("id"); }
const e1 = new Backtrack(exG).parse(shortIn);
const e2 = new Backtrack(exG).parse(longIn);
equals(e1.parseCount, 1, "id + id : one parse tree");
equals(e2.parseCount, 1, "long chain : still one parse tree");
that(e2.entries < 10 * e1.entries, "expr grammar backtracking stays linear");

summary();
