// node js/minimize.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { parse as parseRegex } from "./regex.mjs";
import { Dfa } from "./dfa.mjs";
import { build } from "./thompson.mjs";
import { determinize } from "./subset.mjs";
import { trim, partitionRefinement, tableFilling, minimal, isomorphic, equivalent } from "./minimize.mjs";

console.log("minimize.test.mjs");

// ── a DFA with a redundant state ──
const redundant = Dfa.parse("states: S A B\nalphabet: a\nstart: S\naccept: A B\nS a A\nA a B\nB a A\n");
const m = minimal(redundant);
equals(m.states.length, 2, "S | {A,B} -> 2 states");
equals(JSON.stringify(m.language(6)), JSON.stringify(redundant.language(6)), "language preserved");

// ── the two algorithms agree ──
for (const r of ["(a|b)*abb", "a*", "(a|b)*", "a(b|c)*d", "(ab|ba)*", "a?b?c?"]) {
  const d = trim(determinize(build(parseRegex(r))));
  that(samePartition(partitionRefinement(d), tableFilling(d)),
    `/${r}/: refinement and table filling agree`);
}

// ── minimal is idempotent + language-preserving ──
for (const r of ["(a|b)*abb", "(a|b)*", "a(b|c)", "(ab)*"]) {
  const full = determinize(build(parseRegex(r)));
  const min1 = minimal(full);
  const min2 = minimal(min1);
  equals(min1.states.length, min2.states.length, `/${r}/: idempotent`);
  equals(JSON.stringify(full.language(6)), JSON.stringify(min1.language(6)), `/${r}/: language preserved`);
  that(isomorphic(min1, min2), `/${r}/: isomorphic`);
}

// ── (a|b)*abb -> 4 states, == Module 10's DFA ──
const min = minimal(determinize(build(parseRegex("(a|b)*abb"))));
equals(min.states.length, 4, "minimal DFA for (a|b)*abb has 4 states");
const hand = Dfa.parse(
  "states: S A AB ABB\nalphabet: a b\nstart: S\naccept: ABB\n" +
  "S a A\nS b S\nA a A\nA b AB\nAB a A\nAB b ABB\nABB a A\nABB b S\n");
that(isomorphic(min, hand), "minimal DFA == hand-written DFA");
equals(minimal(hand).states.length, 4, "hand-written DFA was already minimal");

// ── regex equivalence ──
that(eq("(a|b)*", "(a*b*)*"), "(a|b)* == (a*b*)*");
that(eq("a**", "a*"), "a** == a*");
that(eq("a(b|c)", "ab|ac"), "a(b|c) == ab|ac");
that(eq("(ab)*", "(ab)*ab|()"), "(ab)* == (ab)*ab | epsilon");
that(!eq("a*", "a+"), "a* != a+");
that(!eq("ab|ba", "(a|b)(a|b)"), "ab|ba != (a|b)(a|b)");
that(!eq("(a|b)*abb", "(a|b)*ab"), "(a|b)*abb != (a|b)*ab");
that(eq("(a|b)*abb", "(a|b)*abb"), "a regex equals itself");

summary();

function eq(x, y) { return equivalent(parseRegex(x), parseRegex(y)); }
function samePartition(a, b) {
  const norm = arr => new Set(arr.map(c => [...c].sort().join(",")));
  const sa = norm(a), sbb = norm(b);
  if (sa.size !== sbb.size) return false;
  for (const x of sa) if (!sbb.has(x)) return false;
  return true;
}
