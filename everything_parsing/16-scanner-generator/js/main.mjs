// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Scanner, ScanError, SpecError, tokenLine } from "./scanner.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 16 - Scanner Generator: a token spec in, a working lexer out ===");
p();

const sc = Scanner.fromSpec(readFileSync("fixtures/ajoda.tokens", "utf8"));

p("spec: fixtures/ajoda.tokens");
p("  #   action   name       pattern");
for (const r of sc.rules)
  p("  " + pad(String(r.index), 3) + " " + pad(r.action, 8) + " " + pad(r.name, 10) + " " + r.pattern);
p("  keywords (from " + sc.keywords.from + "): " + sc.keywords.words.join(" "));
p();
p("load checks: " + sc.rules.length + " patterns parsed, none accepts the empty string");
p("combined DFA: " + sc.dfaStates + " states over " + sc.alphabetSize + " input symbols");
p();

p("--- fixtures/factorial.ajoda ---");
const prog = readFileSync("fixtures/factorial.ajoda", "utf8").replace(/\r\n/g, "\n");
for (const t of sc.scan(prog)) p("  " + tokenLine(t));
p();

p("--- fixtures/probes.txt (each line scanned on its own) ---");
for (const raw of readFileSync("fixtures/probes.txt", "utf8").split(/\r?\n/)) {
  if (raw.trim() === "" || raw.startsWith("#")) continue;
  p();
  p("> " + raw);
  try {
    p("  " + sc.scan(raw).map(brief).join(" "));
  } catch (e) {
    if (!(e instanceof ScanError)) throw e;
    if (e.partial.length > 0) p("  " + e.partial.map(brief).join(" "));
    p("  error " + e.message);
  }
}
p();

p("--- fixtures/broken.tokens ---");
try {
  Scanner.fromSpec(readFileSync("fixtures/broken.tokens", "utf8"));
  p("  (unexpectedly accepted)");
} catch (e) {
  if (!(e instanceof SpecError)) throw e;
  p("  refused: " + e.message);
}
p();

p("summary: " + sc.rules.length + " rules -> 1 DFA of " + sc.dfaStates
  + " states | longest match, earliest rule breaks ties | keywords by table lookup"
  + " | line:col on every token | empty-string patterns refused at load");

process.stdout.write(sb);

function brief(t) { return t.kind === "EOF" ? "EOF" : t.kind + "(" + t.lexeme + ")"; }
function pad(s, w) { return s.length >= w ? s : s + " ".repeat(w - s.length); }
