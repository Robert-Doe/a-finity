// node js/main.mjs   — identical output bytes to the Java build.

import { parse as parseRegex, matches } from "./regex.mjs";
import { Dfa } from "./dfa.mjs";
import { build } from "./thompson.mjs";
import { determinize, legend, kthFromEndNfa } from "./subset.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 13 - Subset Construction: NFA -> DFA ===");
p();

// ── 1. determinize the Thompson NFA ──
const re = parseRegex("(a|b)*abb");
const nfa = build(re);
const dfa = determinize(nfa);
const leg = legend(nfa);

p("regex /(a|b)*abb/  ->  Thompson NFA (" + nfa.states.length + " states, epsilon: "
  + nfa.hasEpsilon() + ")  ->  subset-construction DFA (" + dfa.states.length + " states)");
p();
p("each DFA state is the SET of NFA states a run could be in:");
let shown = 0;
for (const [name, set] of leg) {
  p("  " + pad(name, 4) + " = " + fmtSet(set) + (dfa.accept.has(name) ? "   (accepting)" : ""));
  if (++shown === 8) { p("  ... (" + leg.size + " DFA states total)"); break; }
}
p();

const bound = 7;
const nfaLang = nfa.language(bound);
const same = JSON.stringify(nfaLang) === JSON.stringify(dfa.language(bound));
const vsRe = nfaLang.every(w => matches(re, w === "epsilon" ? "" : w));
p("L(NFA) up to length " + bound + " == L(DFA):  " + same + "   (" + nfaLang.length + " strings)");
p("  ...and == regex /(a|b)*abb/:  " + vsRe);
p();

// ── 2. the exponential blow-up ──
p('exponential blow-up:  NFA "the k-th symbol from the end is \'a\'" (over {a,b})');
p("  " + padL("k", 3) + "  " + pad("NFA states", 12) + "  " + pad("DFA states", 12) + "  2^k");
for (let k = 1; k <= 5; k++) {
  const n = kthFromEndNfa(k);
  const d = determinize(n);
  const pow = 1 << k;
  p("  " + padL(String(k), 3) + "  " + pad(String(n.states.length), 12) + "  " + pad(String(d.states.length), 12)
    + "  " + pow + (d.states.length === pow ? "   (matches 2^k)" : ""));
}
const d3 = determinize(kthFromEndNfa(3));
p("  spot-check k=3:  \"abaa\" (3rd from end = 'b')  DFA: " + (d3.acceptsString("abaa") ? "ACCEPT" : "reject")
  + "   \"baab\" (3rd from end = 'a')  DFA: " + (d3.acceptsString("baab") ? "ACCEPT" : "reject"));
p();

// ── 3. the complete pipeline ──
const hand = Dfa.parse(
  "states: S A AB ABB\nalphabet: a b\nstart: S\naccept: ABB\n" +
  "S a A\nS b S\nA a A\nA b AB\nAB a A\nAB b ABB\nABB a A\nABB b S\n");
const vsHand = JSON.stringify(dfa.language(bound)) === JSON.stringify(hand.language(bound));
p("complete pipeline:  regex  ->  NFA (" + nfa.states.length + ")  ->  DFA (" + dfa.states.length
  + ")   -- and the DFA's language == the hand-written DFA from Module 10:  " + vsHand);
p();

p("summary: subset construction: DFA state = set of NFA states | L preserved (" + same + ")"
  + " | worst case 2^k states (k-th-from-end), verified for k=1..5");

process.stdout.write(sb);

function fmtSet(s) { return "{" + [...s].sort().join(",") + "}"; }
function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
function padL(s, w) { s = String(s); return s.length >= w ? s : " ".repeat(w - s.length) + s; }
