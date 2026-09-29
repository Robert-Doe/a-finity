// node js/grammar.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar, productionToString, isEpsilon } from "./grammar.mjs";
import { leftmostDerivation, enumerate, derives } from "./derivation.mjs";

console.log("grammar.test.mjs");

// ── parsing a grammar file ──
const g = Grammar.parse("# a^n b^n\nS -> a S b | epsilon\n");
equals(g.start, "S", "start symbol is the first lhs");
equals([...g.nonterminals].join(","), "S", "one nonterminal");
equals([...g.terminals()].join(","), "a,b", "terminals are rhs symbols never on a lhs");
equals(g.productions.length, 2, "two productions (one per alternative)");
that(isEpsilon(g.productions[1]), "second production is epsilon");
equals(productionToString(g.productions[0]), "S -> a S b", "first production renders");

// ── %start override, multi-rule ──
const h = Grammar.parse("%start B\nA -> x\nB -> A A\n");
equals(h.start, "B", "%start overrides first-lhs");
equals([...h.terminals()].join(","), "x", "A is a nonterminal, not a terminal");

// ── malformed grammars are rejected ──
that(rejects("S -> a\nS b -> c"), "lhs must be a single symbol");
that(rejects("S -> -> a"), "two arrows rejected");
that(rejects("just some text"), "missing arrow rejected");
that(rejects("%start Z\nS -> a"), "start symbol must be a nonterminal");
that(rejects("S -> a epsilon b"), "epsilon cannot be mixed with symbols");

// ── leftmost derivation ──
const steps = leftmostDerivation(g, [0, 0, 1]);
equals(steps.length, 4, "3 steps => 4 sentential forms");
equals(steps[0].form.join(" "), "S", "form 0 is the start symbol");
equals(steps[1].form.join(" "), "a S b", "form 1");
equals(steps[2].form.join(" "), "a a S b b", "form 2");
equals(steps[3].form.join(" "), "a a b b", "form 3 is all terminals");
that(throwsOn(() => leftmostDerivation(h, [0])),
  "choosing A -> x when leftmost NT is B is rejected");

// ── enumerate: the language, and its infiniteness ──
const lang6 = enumerate(g, 6);
equals(lang6.join(" | "), "epsilon | a b | a a b b | a a a b b b",
  "a^n b^n up to length 6, in (terminal count, lexicographic) order");
that(enumerate(g, 8).length === lang6.length + 1,
  "raising the bound by one pair adds exactly one string -- infinite");

// ── derives: membership ──
that(derives(g, ["a", "a", "b", "b"]).derivable, "aabb in L(G)");
equals(derives(g, ["a", "a", "b", "b"]).steps, 3, "aabb in 3 steps");
that(!derives(g, ["a", "a", "b"]).derivable, "aab not in L(G)");
that(!derives(g, ["b", "a"]).derivable, "ba not in L(G)");
that(derives(g, []).derivable, "epsilon in L(G)");

// ── a second grammar: balanced parentheses ──
const bal = Grammar.parse("S -> ( S ) S | epsilon");
that(derives(bal, ["(", ")"]).derivable, "() balanced");
that(derives(bal, ["(", "(", ")", ")"]).derivable, "(()) balanced");
that(!derives(bal, ["(", ")", ")"]).derivable, "()) not balanced");
equals(enumerate(bal, 4).join(" | "), "epsilon | ( ) | ( ( ) ) | ( ) ( )",
  "Dyck language up to length 4 (symbols, then lexicographic)");

summary();

function rejects(text)   { try { Grammar.parse(text); return false; } catch { return true; } }
function throwsOn(fn)     { try { fn(); return false; } catch { return true; } }
