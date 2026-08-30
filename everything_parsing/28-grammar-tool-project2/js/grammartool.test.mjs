// node js/grammartool.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { GrammarTool } from "./grammartool.mjs";
import { G, paull, hasLeftRecursion } from "./leftrec.mjs";
import { factor, needsFactoring } from "./leftfactor.mjs";
import { Predict } from "./predict.mjs";

console.log("grammartool.test.mjs");

const s = set => JSON.stringify([...set].sort());
const toGrammar = lg => Grammar.parse("%start " + lg.start + "\n" + lg.text());

// ── LL(1) statement grammar ──
const st = Grammar.parse(`
%start P
P -> L
L -> S L | epsilon
S -> id assign E semi
E -> id | num
`);
const t1 = new GrammarTool(st);
equals([...st.nonterminals].join(" "), "P L S E", "nonterminals in appearance order");
equals(s(t1.fs.nullable), s(new Set(["L", "P"])), "L and P nullable");
equals(s(t1.fs.firstOf("E")), s(new Set(["id", "num"])), "FIRST(E)");
equals(s(t1.fol.followOf("L")), s(new Set(["$"])), "FOLLOW(L)");
that(new Predict(st).isLL1(), "already LL(1)");
that(!hasLeftRecursion(paull(st)), "Paull leaves it LL-friendly");
that(!needsFactoring(factor(st)), "nothing to factor");

// ── natural expression grammar ──
const ex = Grammar.parse(`
%start E
E -> E + T | T
T -> T * F | F
F -> ( E ) | id | num
`);
that(!new Predict(ex).isLL1(), "expression grammar is not LL(1)");
that(hasLeftRecursion(G.from(ex)), "it is left-recursive");

const noLR = paull(ex);
equals(noLR.text().trim(), [
  "E -> T E'",
  "E' -> + T E' | epsilon",
  "T -> F T'",
  "T' -> * F T' | epsilon",
  "F -> ( E ) | id | num",
].join("\n"), "section 5 gives the Module 19 grammar");
that(new Predict(toGrammar(noLR)).isLL1(), "and that IS LL(1)");

// ── JSON grammar ──
const js = Grammar.parse(`
%start value
value    -> obj | arr | str | num | tru | fls | nul
obj      -> lbrace rbrace | lbrace members rbrace
members  -> pair | pair comma members
pair     -> str colon value
arr      -> lbrack rbrack | lbrack elements rbrack
elements -> value | value comma elements
`);
that(!new Predict(js).isLL1(), "JSON grammar as written is not LL(1)");
that(!hasLeftRecursion(G.from(js)), "but it has no left recursion");
that(paull(js).text().includes("elements -> value | value comma elements"),
  "Paull leaves 'elements' untouched");
that(!needsFactoring(factor(js)), "left factoring clears the shared prefixes");
that(new Predict(toGrammar(factor(toGrammar(paull(js))))).isLL1(), "transformed JSON grammar is LL(1)");

// ── report runs end to end ──
const rep = new GrammarTool(ex).report();
that(rep.includes("7. LL(1) VERDICT"), "report has all sections");
that(rep.includes("after left-recursion removal + left factoring: LL(1) = YES"),
  "report confirms the fix works");

summary();
