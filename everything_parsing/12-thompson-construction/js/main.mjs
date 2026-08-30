// node js/main.mjs   — identical output bytes to the Java build.

import { parse as parseRegex, tree, matches } from "./regex.mjs";
import { Nfa } from "./nfa.mjs";
import { Dfa } from "./dfa.mjs";
import { build, stateCount, nodeCount } from "./thompson.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 12 - Thompson's Construction: RE -> epsilon-NFA ===");
p();

// ── one regex, gadget by gadget ──
const src = "a(b|c)";
const re = parseRegex(src);
const nfa = build(re);
p("regex /" + src + "/   tree: " + tree(re));
p("nodes: " + nodeCount(re) + "   |regex| (no parens): " + noParens(src));
p();
p("Thompson NFA:");
p("  states: " + nfa.states.join(" "));
p("  alphabet: " + nfa.alphabet.join(" "));
p("  start: " + nfa.start + "   accept: " + [...nfa.accept].join(" "));
for (const from of nfa.states)
  for (const [sym, tos] of (nfa.delta.get(from) ?? new Map()))
    for (const to of tos) p("  " + from + " " + sym + " " + to);
p("  -> " + nfa.states.length + " states");
p();
p("runs:  " + run(nfa, "ab") + "   " + run(nfa, "ac") + "   " + run(nfa, "a") + "   " + run(nfa, "abc"));
p();

// ── state count vs regex size ──
p("state count is always <= 2*nodes  (and <= 2*|regex|):");
p("  " + pad("regex", 14) + " " + padL("nodes", 6) + " " + padL("chars", 6) + " " + padL("states", 6)
  + "   " + pad("<=2*nodes", 10) + " " + pad("<=2*chars", 10));
for (const r of ["a", "ab", "a|b", "a*", "(a|b)*", "(a|b)*abb", "a(b|c)*d", "((a|b)(c|d))*"]) {
  const x = parseRegex(r);
  const nodes = nodeCount(x), chars = noParens(r), states = stateCount(x);
  p("  " + pad("/" + r + "/", 14) + " " + padL(nodes, 6) + " " + padL(chars, 6) + " " + padL(states, 6)
    + "   " + pad(states <= 2 * nodes ? "yes" : "NO", 10) + " " + pad(states <= 2 * chars ? "yes" : "NO", 10));
}
p();

// ── the full pipeline for (a|b)*abb ──
const big = "(a|b)*abb";
const bre = parseRegex(big);
const bnfa = build(bre);
const bfree = bnfa.removeEpsilon();
const dfa = Dfa.parse(
  "states: S A AB ABB\nalphabet: a b\nstart: S\naccept: ABB\n" +
  "S a A\nS b S\nA a A\nA b AB\nAB a A\nAB b ABB\nABB a A\nABB b S\n");
const bound = 6;
const lang = bnfa.language(bound);
const vsFree = JSON.stringify(lang) === JSON.stringify(bfree.language(bound));
const vsRe = lang.every(w => matches(bre, w === "epsilon" ? "" : w));
const vsDfa = lang.filter(w => w !== "epsilon").every(w => dfa.acceptsString(w));
p("full pipeline for /" + big + "/:");
p("  Thompson NFA:            " + bnfa.states.length + " states, has epsilon: " + bnfa.hasEpsilon());
p("  removeEpsilon():         " + bfree.states.length + " states, has epsilon: " + bfree.hasEpsilon());
p("  L(Thompson NFA) up to length " + bound + ":  " + lang.length + " strings");
p("    == L(epsilon-free NFA):        " + vsFree);
p("    == regex /(a|b)*abb/:          " + vsRe);
p("    == DFA(Module 10):             " + vsDfa);
p();

p("summary: every regex node -> a fixed-size NFA gadget | NFA states <= 2*nodes always"
  + " | L(Thompson NFA) == L(regex) == L(DFA)");

process.stdout.write(sb);

function run(n, w) { return `"${w}" -> ` + (n.acceptsString(w) ? "ACCEPT" : "reject"); }
function noParens(s) { return s.replace(/[()]/g, "").length; }
function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
function padL(s, w) { s = String(s); return s.length >= w ? s : " ".repeat(w - s.length) + s; }
