// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { SpecLexer, LexicalError, tokenStr } from "./speclexer.mjs";
import { Strategy, tokenize as recover, errStr } from "./recovery.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

const lex = SpecLexer.load(readFileSync("fixtures/mini.spec", "utf8"));

p("=== Module 17 - Lexical Errors & Recovery ===");
p();
p("spec: fixtures/mini.spec  (" + lex.rules.length + " rules)  --  errors on characters not in the alphabet");
p();

for (const raw of readFileSync("fixtures/errors.txt", "utf8").split(/\r?\n/)) {
  const line = raw.trim();
  if (line === "" || line.startsWith("#")) continue;
  if (line.length < 2 || line[0] !== '"' || line[line.length - 1] !== '"') continue;
  const input = line.slice(1, -1);

  p('--- input "' + input + '" ---');
  report(input, Strategy.PANIC_ONE, "PANIC_ONE (skip one bad char, ERROR per char)");
  report(input, Strategy.PANIC_TO_SYNC, "PANIC_TO_SYNC (skip to a plausible token start)");

  try {
    lex.tokenize(input);
    p("  Module 16 (no recovery):  tokenized cleanly (no lexical error)");
  } catch (e) {
    if (e instanceof LexicalError) p("  Module 16 (no recovery):  stops -- " + e.message);
    else throw e;
  }
  p();
}

p("summary: recovery = report + resync | PANIC_ONE: one ERROR per bad char"
  + " | PANIC_TO_SYNC: one ERROR per garbage run | one scan finds ALL errors");

process.stdout.write(sb);

function report(input, s, label) {
  const r = recover(lex, input, s);
  p("  strategy " + label + ":");
  for (const t of r.tokens) p("    " + tokenStr(t));
  p("    -> " + r.errors.length + " lexical error" + (r.errors.length === 1 ? "" : "s") + ":  "
    + r.errors.map(errStr).join(",  "));
}
