// node js/workbench.test.mjs

import { readFileSync } from "node:fs";
import { that, equals, summary } from "./assert.mjs";
import { desugar, EbnfError } from "./ebnf.mjs";
import { conflicts, explain, parse, readTokens, fromG } from "./workbench.mjs";
import { Predict } from "./predict.mjs";
import { LL1Table } from "./ll1table.mjs";
import { G, paull, hasLeftRecursion } from "./leftrec.mjs";

console.log("workbench.test.mjs");

const rhs = (g, nt) => g.productionsFor(nt).map(p => p.rhs.length ? p.rhs.join(" ") : "epsilon").join(" | ");

// ── desugaring ──
const d1 = desugar('A ::= "x" { B } [ "y" ]\nB ::= "b"');
equals(rhs(d1.grammar, "A"), '"x" A_rep1 A_opt2', "brackets become named helpers");
equals(rhs(d1.grammar, "A_rep1"), "B A_rep1 | epsilon", "{ X } -> X rep | epsilon");
equals(rhs(d1.grammar, "A_opt2"), '"y" | epsilon', "[ X ] -> X | epsilon");
equals(rhs(desugar('A ::= ( "a" "b" ) "c"').grammar, "A"), '"a" "b" "c"', "a one-branch group is inlined");
equals(d1.origin.get("A_rep1"), "A", "helpers remember their rule");

// ── names ──
that(throwsEbnf('A ::= Bee'), "an undefined rule name is an error");
that(!throwsEbnf('A ::= IDENT'), "an UPPER_CASE name is a token class");
that(throwsEbnf('A ::= "x"\nA ::= "y"'), "a rule defined twice is an error");
that(throwsEbnf('A ::= { "x"'), "an unclosed bracket is an error");

// ── Ajoda as written: exactly one conflict, explained ──
const raw = desugar(readFileSync("fixtures/ajoda.ebnf", "utf8"));
const prRaw = new Predict(raw.grammar);
const cs = conflicts(raw.grammar, prRaw);
equals(cs.length, 1, "one conflict");
equals(cs[0].nt + " " + cs[0].token, "Statement IDENT", "Statement on IDENT");
that(explain(raw.grammar, prRaw, cs[0].prods[1], "IDENT").endsWith("Call -> Primary -> IDENT"),
  "the ExprStmt path is traced down to Primary");

// ── the repaired grammar is LL(1) and parses real tokens ──
const fixed = desugar(readFileSync("fixtures/ajoda-fixed.ebnf", "utf8"));
const table = new LL1Table(fixed.grammar);
equals(conflicts(fixed.grammar, new Predict(fixed.grammar)).length, 0, "the repair removes the conflict");
const ok = parse(fixed.grammar, table, fixed.origin, readTokens(readFileSync("fixtures/factorial.tokens", "utf8")), 0);
that(ok.ok, "factorial.ajoda parses");
const bad = parse(fixed.grammar, table, fixed.origin, readTokens(readFileSync("fixtures/missing-semi.tokens", "utf8")), 0);
that(!bad.ok && bad.error.startsWith("3:5:"), "a missing ';' is caught at the next token, 3:5");

// ── left recursion ──
const arith = desugar(readFileSync("fixtures/arith-leftrec.ebnf", "utf8"));
that(hasLeftRecursion(G.from(arith.grammar)), "Expr ::= Expr ... is left-recursive");
const noLR = fromG(paull(arith.grammar));
that(!hasLeftRecursion(G.from(noLR)), "Paull removes it");
equals(conflicts(noLR, new Predict(noLR)).length, 0, "and the result is LL(1)");

summary();

function throwsEbnf(text) {
  try { desugar(text); return false; } catch (e) { return e instanceof EbnfError; }
}
