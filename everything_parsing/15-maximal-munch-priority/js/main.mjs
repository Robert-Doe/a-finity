// node js/main.mjs   — identical output bytes to the Java build.

import { makeRule, longestAccept, nextToken, tokenize, matchLengths, tokenStr } from "./lexer.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

const rules = [
  makeRule("KW_IF", "if"),
  makeRule("ID", "(i|f|x|y)(i|f|x|y)*"),
  makeRule("ASSIGN", "=="),
  makeRule("EQ", "="),
];

p("=== Module 15 - Maximal Munch & Token Priority ===");
p();
p("token rules  (priority = declaration order):");
rules.forEach((r, i) => p("  (" + i + ") " + pad(r.name, 8) + " = " + r.pattern));
p();

p("RULE 1 -- longest match wins:");
for (const s of ["iffy", "ifx", "=="]) {
  const t = nextToken(rules, s, 0);
  p('  "' + s + '"' + spaces(9 - s.length - 2) + "-> " + tokenStr(t)
    + "   (KW_IF/EQ would have matched a shorter prefix)");
}
p();

p("RULE 2 -- on a length tie, the EARLIER rule wins:");
const ifIf = longestAccept(rules[0].dfa, "if", 0);
const ifId = longestAccept(rules[1].dfa, "if", 0);
p('  "if"   -> KW_IF matches len ' + ifIf + ",  ID matches len " + ifId
  + "  -- tie  ->  " + tokenStr(nextToken(rules, "if", 0)) + "   (rule 0 < rule 1)");
p();

p("the mechanism -- run the DFA, keep the LAST-ACCEPT position, back up to it:");
traceOne(rules[0], "iffy");
traceOne(rules[1], "iffy");
p("  -> ID's last-accept (4) beats KW_IF's (2)  ->  the token is ID \"iffy\"");
p();

p("full tokenization:");
for (const s of ["iffy==x=y", "if==x", "x=y"]) {
  p('  input "' + s + '":');
  for (const t of tokenize(rules, s)) p("    " + tokenStr(t));
}
p();

try {
  tokenize(rules, "if z");
} catch (e) {
  p('lexical error demo:  tokenize("if z")  ->  ' + e.message);
}
p();

p("summary: 4 rules | longest match wins | ties -> first declared | mechanism = DFA + last-accept mark");

process.stdout.write(sb);

function traceOne(r, s) {
  const hits = matchLengths(r.dfa, s, 0);
  let marks = "";
  for (let L = 1; L <= s.length; L++) {
    const hit = hits.includes(L);
    marks += s.slice(0, L) + (hit ? "(ACCEPT@" + L + ") " : "(-) ");
  }
  p("  " + pad(r.name, 8) + ' on "' + s + '":  ' + marks.trim()
    + "   -> last accept = " + (hits.length === 0 ? "none" : hits[hits.length - 1]));
}
function spaces(n) { return n <= 0 ? "" : " ".repeat(n); }
function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
