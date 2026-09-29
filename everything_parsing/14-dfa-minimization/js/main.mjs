// node js/main.mjs   — identical output bytes to the Java build.

import { parse as parseRegex } from "./regex.mjs";
import { Dfa } from "./dfa.mjs";
import { build } from "./thompson.mjs";
import { determinize } from "./subset.mjs";
import { partitionRefinement, tableFilling, minimal, isomorphic, equivalent } from "./minimize.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 14 - DFA Minimization ===");
p();

const subset = determinize(build(parseRegex("(a|b)*abb")));
p("subset-construction DFA for /(a|b)*abb/:  " + subset.states.length + " states");
p();

const byRefine = partitionRefinement(subset);
const byTable = tableFilling(subset);
p("partition refinement -> " + byRefine.length + " equivalence classes:");
for (const c of byRefine) p("  { " + c.join(", ") + " }");
p();
p("table filling -> " + byTable.length + " classes;  same partition as refinement:  "
  + samePartition(byRefine, byTable));
p();

const min = minimal(subset);
p("minimal DFA:  " + min.states.length + " states");
p(min.transitionTable());
p();

const hand = Dfa.parse(
  "states: S A AB ABB\nalphabet: a b\nstart: S\naccept: ABB\n" +
  "S a A\nS b S\nA a A\nA b AB\nAB a A\nAB b ABB\nABB a A\nABB b S\n");
p("Module 10's hand-written DFA:  " + hand.states.length + " states (already minimal: "
  + (minimal(hand).states.length === hand.states.length) + ")");
p("minimal DFA is isomorphic to the hand-written DFA:  " + isomorphic(min, hand));
p();
p("Myhill-Nerode:  L(/(a|b)*abb/) has exactly " + min.states.length
  + " right-language classes  =  " + min.states.length + " states, and that DFA is unique.");
p();

p("--- regex equivalence (decided via minimal-DFA isomorphism) ---");
const pairs = [
  ["(a|b)*", "(a*b*)*", "both = Sigma*"],
  ["a**", "a*", ""],
  ["a(b|c)", "ab|ac", "concatenation distributes over |"],
  ["ab|ba", "(a|b)(a|b)", "RHS also matches aa, bb"],
  ["a*", "a+", "a+ excludes the empty string"],
  ["(a|b)*abb", "(a|b)*abb", ""],
];
for (const [x, y, note] of pairs) {
  const eq = equivalent(parseRegex(x), parseRegex(y));
  p("  " + pad("/" + x + "/", 14) + " == " + pad("/" + y + "/", 14) + " :  " + pad(String(eq), 6)
    + " " + (note === "" ? "" : "(" + note + ")"));
}
p();

p("summary: subset DFA " + subset.states.length + " -> minimal " + min.states.length
  + " | partition-refinement and table-filling agree | minimal DFA is unique (Myhill-Nerode)"
  + " -> regex equivalence is decidable");

process.stdout.write(sb);

function samePartition(a, b) {
  const norm = arr => new Set(arr.map(c => [...c].sort().join(",")));
  const sa = norm(a), sb2 = norm(b);
  if (sa.size !== sb2.size) return false;
  for (const x of sa) if (!sb2.has(x)) return false;
  return true;
}
function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
