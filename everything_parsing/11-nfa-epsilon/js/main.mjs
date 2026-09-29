// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Nfa, showSet } from "./nfa.mjs";
import { Dfa } from "./dfa.mjs";
import { parse as parseRegex, matches } from "./regex.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };
const pf = (label, set) => p("  after " + label.padEnd(6) + " " + showSet(set));

p("=== Module 11 - NFA and epsilon-NFA ===");
p();

// ── 1. the nondeterministic NFA ──
const abb = load("ends-abb.nfa");
p("--- fixtures/ends-abb.nfa   (nondeterministic, for (a|b)*abb) ---");
p("states: " + abb.states.join(" ") + "    alphabet: " + abb.alphabet.join(" "));
p("start: " + abb.start + "    accept: " + [...abb.accept].join(" "));
p("  S has TWO transitions on 'a':  S -a-> S   and   S -a-> S1   (the 'guess')");
p();
p('run "aabb"  (tracking the SET of possible states):');
const tr = abb.trace([..."aabb"]);
["start", "a", "a", "b", "b"].forEach((lab, i) => pf(lab, tr[i]));
p("  final set contains S3 (accepting)?  " + tr[tr.length - 1].has("S3") + "   -> ACCEPT");
p();

const dfa = Dfa.parse(readFileSync("../10-dfa/fixtures/ends-abb.dfa", "utf8"));
const re = parseRegex("(a|b)*abb");
const bound = 6;
const nfaLang = abb.language(bound).filter(s => s !== "epsilon");
const dfaLang = dfa.language(bound).filter(s => s !== "epsilon");
const vsDfa = eqSet(nfaLang, dfaLang);
const vsRe = nfaLang.every(s => matches(re, s));
p("cross-check, all up to length " + bound + ":");
p("  NFA language == DFA language (Module 10):  " + vsDfa + "   (" + nfaLang.length + " strings)");
p("  every NFA-accepted string is regex-matched:  " + vsRe);
p();

// ── 2. the epsilon-NFA ──
const eps = load("ab-star.nfa");
p("--- fixtures/ab-star.nfa   (epsilon-NFA for (a|b)*) ---");
p("has epsilon transitions?  " + eps.hasEpsilon());
p();
p("epsilon-closure of {I} (the start):  " + showSet(eps.epsilonClosure(new Set(["I"]))));
p("  -> before reading any input, the NFA is 'in' all of these at once,");
p("     including F, so the empty string is accepted (zero repetitions).");
p();
p('run "ab":');
const etr = eps.trace([..."ab"]);
["start", "a", "b"].forEach((lab, i) => pf(lab, etr[i]));
p();

// ── 3. epsilon elimination ──
const noEps = eps.removeEpsilon();
p("removeEpsilon():  new NFA has epsilon transitions?  " + noEps.hasEpsilon());
p("  new accepting states: " + [...noEps.accept].join(" ")
  + "   (every state whose epsilon-closure reached the old F)");
const lb = 5;
const sameLang = JSON.stringify(eps.language(lb)) === JSON.stringify(noEps.language(lb));
const abStar = parseRegex("(a|b)*");
const vsReStar = noEps.language(lb).every(w => matches(abStar, w === "epsilon" ? "" : w));
p("  L(epsilon-NFA) == L(epsilon-free NFA), up to length " + lb + ":  " + sameLang);
p("  ...and both == regex (a|b)*:  " + vsReStar);
const nl = noEps.language(lb);
p("  language: " + nl.slice(0, 10).join(", ") + (nl.length > 10 ? ", ..." : ""));
p();

p("summary: NFA(a|b)*abb == DFA == regex (" + vsDfa + ") | epsilon-closure computed"
  + " | epsilon removed, language unchanged (" + sameLang + ") -> epsilon and nondeterminism add NO power");

process.stdout.write(sb);

function load(name) { return Nfa.parse(readFileSync("fixtures/" + name, "utf8")); }
function eqSet(a, b) { return a.length === b.length && new Set([...a, ...b]).size === a.length; }
