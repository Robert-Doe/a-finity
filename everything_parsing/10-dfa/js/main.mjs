// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Dfa } from "./dfa.mjs";
import { parse as parseRegex, toLanguage } from "./regex.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 10 - DFA: Deterministic Finite Automata ===");
p();

section("ends-abb.dfa", "the regex (a|b)*abb", ["abb", "aabb", "ab", "abba", ""]);

// cross-check against Module 3's regex engine
const d = load("ends-abb.dfa");
const re = parseRegex("(a|b)*abb");
const bound = 6;
const dfaLang = d.language(bound).filter(s => s !== "epsilon");
const reLang = toLanguage(re, bound).list().filter(s => s !== "");
const same = eqSet(dfaLang, reLang);
p(`cross-check vs regex (a|b)*abb, both up to length ${bound}:`);
p(`  DFA language == regex language:  ${same}   (${dfaLang.length} strings)`);
p();

section("even-a.dfa", "even number of a's", ["", "a", "aa", "bab", "abba"]);

section("a-star-b-star.dfa", "a*b*, NOT a^n b^n", ["aaab", "abb", "ba", "aabb"]);
const g = load("a-star-b-star.dfa");
const rep = g.repeatOn("a");
p("why a*b* is not { a^n b^n }:");
p(`  accepts "aaab"? ${g.acceptsString("aaab")}    accepts "abb"? ${g.acceptsString("abb")}    accepts "ba"? ${g.acceptsString("ba")}`);
p(`  repeatOn('a'): the DFA is in the SAME state after ${plur(rep[0])} and after ${plur(rep[1])}`);
p("  -> it keeps no count of a's. Module 7 proved NO dfa can accept { a^n b^n }.");
p("     a*b* is the closest regular over-approximation.");
p();

p(`summary: 3 DFAs simulated | DFA(ends-abb) language == regex (a|b)*abb == ${same}`
  + ` | a*b* accepts "aaab" (so it is not { a^n b^n })`);

process.stdout.write(sb);

function section(file, desc, tests) {
  const dd = load(file);
  p(`--- fixtures/${file}  (${desc}) ---`);
  p("states: " + dd.states.join(" ") + "    alphabet: " + dd.alphabet.join(" "));
  p("start: " + dd.start + "    accept: " + [...dd.accept].join(" "));
  p();
  p("transition table  ( -> start,  * accepting ):");
  p(dd.transitionTable());
  p();
  p("runs:");
  for (const t of tests) {
    const trace = dd.trace([...t]);
    const shown = t === "" ? '""' : `"${t}"`;
    p("  " + pad(shown, 8) + " -> " + pad(trace.join(" "), 20) + " " + (dd.acceptsString(t) ? "ACCEPT" : "reject"));
  }
  p();
  const lang = dd.language(5);
  const shown = lang.length <= 15 ? lang.join(", ")
    : lang.slice(0, 15).join(", ") + ", ... (" + lang.length + " total)";
  p("language (length <= 5): " + shown);
  p();
}

function load(name) { return Dfa.parse(readFileSync("fixtures/" + name, "utf8")); }
function eqSet(a, b) { return a.length === b.length && new Set(a).size === new Set([...a, ...b]).size; }
function plur(n) { return n === 1 ? "1 'a'" : n + " 'a's"; }
function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
