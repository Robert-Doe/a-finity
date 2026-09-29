// node js/ebnf.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { enumerate } from "./derivation.mjs";
import { parse, toBnfText, accepts, isoRule } from "./ebnf.mjs";

console.log("ebnf.test.mjs");

// ── parsing EBNF ──
equals(isoRule(parse("a = 'x' 'y' ;").rules.get("a")), "'x' 'y'", "sequence of two terminals");
equals(isoRule(parse("a = 'x' | 'y' ;").rules.get("a")), "'x' | 'y'", "alternation");
equals(isoRule(parse("a = 'x'? ;").rules.get("a")), "[ 'x' ]", "optional -> [ ]");
equals(isoRule(parse("a = 'x'* ;").rules.get("a")), "{ 'x' }", "star -> { }");
equals(isoRule(parse("a = 'x'+ ;").rules.get("a")), "'x' { 'x' }", "plus -> x { x }");
equals(isoRule(parse("a = ( 'x' | 'y' ) 'z' ;").rules.get("a")), "( 'x' | 'y' ) 'z'", "grouping");

// ── desugaring ──
let bnf = toBnfText(parse("a = 'x'? ;"));
that(bnf.includes("a -> opt_1") && bnf.includes("opt_1 -> x | epsilon"), "x? -> Opt -> x | epsilon");
bnf = toBnfText(parse("a = 'x'* ;"));
that(bnf.includes("a -> rep_1") && bnf.includes("rep_1 -> x rep_1 | epsilon"), "x* -> Rep -> x Rep | epsilon");
bnf = toBnfText(parse("a = 'x'+ ;"));
that(bnf.includes("plus_1 -> x plus_1 | x"), "x+ -> Plus -> x Plus | x");

// ── round trip ──
const expr = parse(
  "expr = term (('+' | '-') term)* ;\n" +
  "term = factor (('*' | '/') factor)* ;\n" +
  "factor = '(' expr ')' | NUM ;\n");
const be = Grammar.parse(toBnfText(expr));
const lang = enumerate(be, 7);
let allAgree = true;
for (const w of lang) {
  const toks = w === "epsilon" ? [] : w.split(" ");
  if (!accepts(expr, toks)) allAgree = false;
}
that(allAgree, "every string L(BNF) generates is accepted by the EBNF matcher");
that(lang.length > 50, `the enumeration is non-trivial (${lang.length} strings)`);

// ── specific strings ──
that(accepts(expr, ["NUM"]), "NUM");
that(accepts(expr, ["NUM", "+", "NUM", "*", "NUM"]), "NUM + NUM * NUM");
that(accepts(expr, ["(", "NUM", "+", "NUM", ")"]), "( NUM + NUM )");
that(!accepts(expr, ["NUM", "+"]), "NUM + is rejected");
that(!accepts(expr, ["+", "NUM"]), "+ NUM is rejected");
that(!accepts(expr, ["(", "NUM"]), "( NUM is rejected");

// ── signed ──
const num = parse("number = '-'? digit+ ; digit = '0' | '1' | '2' ;");
that(accepts(num, ["0"]), "0");
that(accepts(num, ["-", "1", "2"]), "-12");
that(accepts(num, ["1", "0", "1", "2"]), "1012");
that(!accepts(num, ["-"]), "- alone is rejected");
that(!accepts(num, []), "empty is rejected");
that(!accepts(num, ["-", "-", "1"]), "-- is rejected");

// ── malformed ──
that(rejects("a = 'x'"), "missing ';'");
that(rejects("a 'x' ;"), "missing '='");
that(rejects("a = ( 'x' ;"), "unbalanced (");

summary();

function rejects(src) { try { parse(src); return false; } catch { return true; } }
