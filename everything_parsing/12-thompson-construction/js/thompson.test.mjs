// node js/thompson.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { parse as parseRegex, matches, toLanguage, empty, epsilon } from "./regex.mjs";
import { build, stateCount, nodeCount } from "./thompson.mjs";

console.log("thompson.test.mjs");

// ── gadget sizes ──
equals(stateCount(parseRegex("a")), 2, "Char -> 2 states");
equals(stateCount(parseRegex("ab")), 4, "Concat adds 0 states");
equals(stateCount(parseRegex("a|b")), 6, "Union adds 2 states");
equals(stateCount(parseRegex("a*")), 4, "Star adds 2 states");
equals(stateCount(parseRegex("()")), 2, "Epsilon -> 2 states");

// ── the bound ──
for (const r of ["a", "ab", "a|b", "a*", "(a|b)*", "(a|b)*abb", "a(b|c)*d", "((a|b)(c|d))*", "a**", "(a|b|c|d)*"]) {
  const x = parseRegex(r);
  const states = stateCount(x), nodes = nodeCount(x);
  that(states <= 2 * nodes, `/${r}/: ${states} states <= 2*${nodes} nodes`);
  const chars = r.replace(/[()]/g, "").length;
  that(states <= 2 * chars, `/${r}/: ${states} states <= 2*${chars} chars`);
}

// ── the built NFA matches the regex ──
const re = parseRegex("(a|b)*abb");
const nfa = build(re);
for (const w of nfa.language(7)) {
  if (w === "epsilon") continue;
  that(matches(re, w), `Thompson NFA & regex agree on "${w}"`);
}
for (const w of toLanguage(re, 7).list())
  if (w !== "") that(nfa.acceptsString(w), `regex & Thompson NFA agree on "${w}"`);

// ── specific runs ──
const abc = build(parseRegex("a(b|c)"));
that(abc.acceptsString("ab") && abc.acceptsString("ac"), "a(b|c) accepts ab, ac");
that(!abc.acceptsString("a") && !abc.acceptsString("abc"), "a(b|c) rejects a, abc");

const star = build(parseRegex("(ab)*"));
that(star.acceptsString("") && star.acceptsString("ab") && star.acceptsString("abab"), "(ab)* even reps");
that(!star.acceptsString("a") && !star.acceptsString("aba"), "(ab)* rejects partials");

// ── Empty / Epsilon ──
const emptyNfa = build(empty());
that(!emptyNfa.acceptsString("") && !emptyNfa.acceptsString("a"), "∅ accepts nothing");
const epsNfa = build(epsilon());
that(epsNfa.acceptsString("") && !epsNfa.acceptsString("a"), "ε accepts only the empty string");

// ── nested stars ──
const nested = build(parseRegex("(a*)*"));
that(nested.acceptsString("") && nested.acceptsString("aaa"), "(a*)* accepts a-strings");
that(!nested.acceptsString("b"), "(a*)* rejects b");

// ── epsilon-free preserves the language ──
const free = nfa.removeEpsilon();
equals(JSON.stringify(nfa.language(6)), JSON.stringify(free.language(6)), "removeEpsilon preserves the language");
that(!free.hasEpsilon(), "and leaves no epsilon transitions");

summary();
