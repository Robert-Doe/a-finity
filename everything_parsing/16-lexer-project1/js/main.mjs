// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { SpecLexer, LexicalError, tokenStr } from "./speclexer.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 16 - The Lexical Analyzer (Project 1) ===");
p();

const lex = SpecLexer.load(readFileSync("fixtures/mini.spec", "utf8"));

p("spec: fixtures/mini.spec");
for (const r of lex.rules)
  p("  (" + r.index + ") " + pad(r.name, 8) + " = " + squash(r.pattern)
    + (lex.skip.has(r.name) ? "   [skipped]" : ""));
p();
p("epsilon check:  no token can match the empty string  ->  OK");
p("combined DFA:  " + lex.combinedDfaStates + " states  (union of " + lex.rules.length
  + " pattern NFAs, tagged, determinized)");
p();

const lines = readFileSync("fixtures/inputs.txt", "utf8").split(/\r?\n/);
p("--- fixtures/inputs.txt ---");
for (const raw of lines) {
  const line = raw.trim();
  if (line === "" || line.startsWith("#")) continue;
  if (line.length < 2 || line[0] !== '"' || line[line.length - 1] !== '"') continue;
  const input = line.slice(1, -1);
  p();
  p('"' + input + '"');
  try {
    for (const t of lex.tokenize(input)) p("  " + tokenStr(t));
    const skipped = lex.countSkipped(input);
    if (skipped > 0) p("  (" + skipped + " WS token" + (skipped === 1 ? "" : "s") + " skipped)");
  } catch (e) {
    if (e instanceof LexicalError) p("  " + e.message);
    else throw e;
  }
}
p();

p("--- bad spec: a token that matches epsilon ---");
try {
  SpecLexer.load(readFileSync("fixtures/bad.spec", "utf8"));
  p("  fixtures/bad.spec  ->  (unexpectedly accepted?!)");
} catch (e) {
  p("  fixtures/bad.spec  ->  rejected:  " + e.message);
}
p();

p("summary: " + lex.rules.length + " rules -> 1 combined DFA (" + lex.combinedDfaStates
  + " states) | maximal munch + priority in one pass | WS skipped | epsilon-token rejected at load");

process.stdout.write(sb);

function squash(p) { return p.replaceAll("(0|1|2|3|4|5|6|7|8|9)", "(0..9)"); }
function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
