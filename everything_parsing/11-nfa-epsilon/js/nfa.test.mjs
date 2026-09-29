// node js/nfa.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Nfa, showSet } from "./nfa.mjs";
import { Dfa } from "./dfa.mjs";
import { parse as parseRegex, matches } from "./regex.mjs";

console.log("nfa.test.mjs");

// ── nondeterminism ──
const abb = Nfa.parse(
  "states: S S1 S2 S3\nalphabet: a b\nstart: S\naccept: S3\n" +
  "S a S\nS b S\nS a S1\nS1 b S2\nS2 b S3\n");
equals(abb.move("S", "a").size, 2, "S has two targets on 'a'");
that(abb.acceptsString("abb"), "abb");
that(abb.acceptsString("aabb"), "aabb");
that(abb.acceptsString("bababb"), "bababb");
that(!abb.acceptsString("ab"), "ab is not accepted");
that(!abb.acceptsString("abba"), "abba is not accepted");
that(!abb.acceptsString(""), "empty is not accepted");

const tr = abb.trace([..."aabb"]);
equals(showSet(tr[0]), "{S}", "start set");
equals(showSet(tr[1]), "{S,S1}", "after first a");
equals(showSet(tr[4]), "{S,S3}", "after aabb -- S3 reached");

// ── epsilon closure ──
const eps = Nfa.parse(
  "states: I U A1 A2 B1 B2 W F\nalphabet: a b\nstart: I\naccept: F\n" +
  "I epsilon U\nI epsilon F\nU epsilon A1\nU epsilon B1\nA1 a A2\nB1 b B2\n" +
  "A2 epsilon W\nB2 epsilon W\nW epsilon U\nW epsilon F\n");
that(eps.hasEpsilon(), "the NFA has epsilon transitions");
equals(showSet(eps.epsilonClosure(new Set(["I"]))), "{A1,B1,F,I,U}",
  "epsilon-closure of the start includes F");
that(eps.acceptsString(""), "(a|b)* accepts the empty string");
that(eps.acceptsString("ab") && eps.acceptsString("ba") && eps.acceptsString("abbaab"),
  "(a|b)* accepts mixed strings");

// ── epsilon elimination preserves the language ──
const noEps = eps.removeEpsilon();
that(!noEps.hasEpsilon(), "removeEpsilon() leaves no epsilon transitions");
equals(JSON.stringify(eps.language(6)), JSON.stringify(noEps.language(6)),
  "same language up to length 6");
that(noEps.accept.has("I"), "I is accepting now");

// ── NFA == DFA == regex ──
const dfa = Dfa.parse(
  "states: S A AB ABB\nalphabet: a b\nstart: S\naccept: ABB\n" +
  "S a A\nS b S\nA a A\nA b AB\nAB a A\nAB b ABB\nABB a A\nABB b S\n");
const re = parseRegex("(a|b)*abb");
for (const w of abb.language(7)) {
  if (w === "epsilon") continue;
  that(dfa.acceptsString(w), `DFA agrees on "${w}"`);
  that(matches(re, w), `regex agrees on "${w}"`);
}
for (const w of dfa.language(7))
  if (w !== "epsilon") that(abb.acceptsString(w), `NFA agrees on "${w}"`);

// ── malformed ──
that(rejects("states: S\nstart: S\nS a S"), "missing alphabet:");
that(rejects("states: S\nalphabet: a\nstart: S\nS a Q"), "transition to unknown state");

summary();

function rejects(s) { try { Nfa.parse(s); return false; } catch { return true; } }
