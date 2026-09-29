// node js/dfa.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Dfa } from "./dfa.mjs";
import { parse as parseRegex, matches, toLanguage } from "./regex.mjs";

console.log("dfa.test.mjs");

const abb = Dfa.parse(
  "states: S A AB ABB\nalphabet: a b\nstart: S\naccept: ABB\n" +
  "S a A\nS b S\nA a A\nA b AB\nAB a A\nAB b ABB\nABB a A\nABB b S\n");

// ── delta is total ──
for (const s of abb.states)
  for (const sym of abb.alphabet)
    that(abb.states.includes(abb.step(s, sym)), `delta(${s},${sym}) is defined`);

// ── acceptance ──
that(abb.acceptsString("abb"), "abb ends in abb");
that(abb.acceptsString("aabb"), "aabb ends in abb");
that(abb.acceptsString("babbabb"), "babbabb ends in abb");
that(!abb.acceptsString(""), "empty does not end in abb");
that(!abb.acceptsString("ab"), "ab does not end in abb");
that(!abb.acceptsString("abba"), "abba does not end in abb");
that(!abb.acceptsString("abc"), "symbol outside the alphabet -> reject");

equals(abb.trace(["a", "b", "b"]).join(" "), "S A AB ABB", "trace of abb");

// ── DFA language == regex language, up to a bound ──
const re = parseRegex("(a|b)*abb");
for (const w of abb.language(6))
  if (w !== "epsilon") that(matches(re, w), `DFA-accepted "${w}" is also regex-matched`);
for (const w of toLanguage(re, 6).list())
  if (w !== "") that(abb.acceptsString(w), `regex-matched "${w}" is also DFA-accepted`);

// ── missing transitions route to DEAD ──
const partial = Dfa.parse("states: X Y\nalphabet: a b\nstart: X\naccept: Y\nX a Y\n");
that(partial.states.includes("<dead>"), "a DEAD state was added");
equals(partial.step("X", "b"), "<dead>", "the missing (X,b) goes to DEAD");
that(partial.acceptsString("a") && !partial.acceptsString("ab") && !partial.acceptsString("b"),
  "DEAD is a trap");

// ── even number of a's ──
const even = Dfa.parse("states: E O\nalphabet: a b\nstart: E\naccept: E\nE a O\nE b E\nO a E\nO b O\n");
that(even.acceptsString("") && even.acceptsString("aa") && even.acceptsString("bbaabb"), "even # of a's accepted");
that(!even.acceptsString("a") && !even.acceptsString("baaba"), "odd # of a's rejected");

// ── the pigeonhole repeat ──
const asbs = Dfa.parse(
  "states: A B DEAD\nalphabet: a b\nstart: A\naccept: A B\n" +
  "A a A\nA b B\nB a DEAD\nB b B\nDEAD a DEAD\nDEAD b DEAD\n");
that(asbs.acceptsString("aaab"), "a*b* accepts aaab -- it is NOT a^n b^n");
that(asbs.acceptsString("abbb"), "a*b* accepts abbb");
that(!asbs.acceptsString("ba"), "a*b* rejects ba");
const r = asbs.repeatOn("a");
equals(r[0], 0, "state A after 0 a's");
equals(r[1], 1, "same state A after 1 a");

// ── malformed ──
that(rejects("alphabet: a\nstart: S\nS a S"), "missing states:");
that(rejects("states: S\nalphabet: a\nstart: Z\nS a S"), "start not among states");
that(rejects("states: S\nalphabet: a\nstart: S\naccept: Z\nS a S"), "accept not among states");
that(rejects("states: S\nalphabet: a\nstart: S\nS a Q"), "transition to unknown state");

summary();

function rejects(src) { try { Dfa.parse(src); return false; } catch { return true; } }
