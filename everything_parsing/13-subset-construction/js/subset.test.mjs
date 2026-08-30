// node js/subset.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { parse as parseRegex, matches, toLanguage, empty } from "./regex.mjs";
import { Nfa } from "./nfa.mjs";
import { build } from "./thompson.mjs";
import { determinize, legend, kthFromEndNfa } from "./subset.mjs";

console.log("subset.test.mjs");

// ── determinize a Thompson NFA and check the language ──
const re = parseRegex("(a|b)*abb");
const nfa = build(re);
const dfa = determinize(nfa);

equals(JSON.stringify(nfa.language(7)), JSON.stringify(dfa.language(7)), "L(NFA) == L(DFA) up to length 7");
for (const w of toLanguage(re, 7).list())
  if (w !== "") that(dfa.acceptsString(w), `regex-matched "${w}" is DFA-accepted`);
for (const w of dfa.language(7))
  if (w !== "epsilon") that(matches(re, w), `DFA-accepted "${w}" is regex-matched`);

// ── D0 is the epsilon-closure of the NFA start ──
const leg = legend(nfa);
equals(JSON.stringify([...leg.get("D0")].sort()),
  JSON.stringify([...nfa.epsilonClosure(new Set([nfa.start]))].sort()),
  "D0 is epsilonClosure({nfa.start})");

// ── accepting DFA state <=> its set meets the NFA accept set ──
for (const [name, set] of leg) {
  const meets = [...set].some(s => nfa.accept.has(s));
  equals(dfa.accept.has(name), meets, `${name} accepting iff its set meets NFA accept`);
}

// ── the exponential blow-up ──
for (let k = 1; k <= 6; k++) {
  const n = kthFromEndNfa(k);
  const d = determinize(n);
  equals(n.states.length, k + 1, `k=${k}: NFA has k+1 states`);
  equals(d.states.length, 1 << k, `k=${k}: DFA has exactly 2^k states`);
}

// ── the k-th-from-end DFA decides correctly ──
const d3 = determinize(kthFromEndNfa(3));
that(d3.acceptsString("baab"), "3rd from end of baab is 'a'");
that(!d3.acceptsString("abaa"), "3rd from end of abaa is 'b'");
that(d3.acceptsString("aaa"), "3rd from end of aaa is 'a'");
that(!d3.acceptsString("ab"), "too short");

// ── empty NFA ──
const noneD = determinize(build(empty()));
that(noneD.accept.size === 0, "L(∅) DFA has no accepting state");
that(!noneD.acceptsString("") && !noneD.acceptsString("a"), "and accepts nothing");

// ── even-a ──
const evenA = Nfa.parse("states: E O\nalphabet: a b\nstart: E\naccept: E\nE a O\nE b E\nO a E\nO b O\n");
equals(JSON.stringify(evenA.language(6)), JSON.stringify(determinize(evenA).language(6)),
  "even-a: NFA and its DFA agree");

summary();
