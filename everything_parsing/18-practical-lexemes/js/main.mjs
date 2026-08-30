// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { KEYWORDS, lex, tokStr } from "./practicallexer.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 18 - Practical Lexemes: Numbers, Strings, Comments, Keywords ===");
p();
p("keyword table: " + [...KEYWORDS].sort().join(" "));
p("  -> scan an identifier, then look it up.  A new keyword is a one-line table edit.");
p();

for (const raw of readFileSync("fixtures/programs.txt", "utf8").split(/\r?\n/)) {
  const line = raw.replace(/\s+$/, "");
  if (line.trim() === "" || line.trim().startsWith("#")) continue;
  const program = line.replace(/~/g, "\n");

  p("--- " + line + " ---");
  const r = lex(program);
  p("  tokens:  " + r.tokens.map(tokStr).join("  "));
  for (const e of r.errors) p("  ERROR:   " + e);
  p();
}

p("nesting is beyond regular:");
const s1 = "/* a /* b */ c */";
const n1 = lex(s1 + " x");
p('  "' + s1 + ' x"  ->  ' + n1.tokens.map(tokStr).join(" "));
p("  a flat regex /\\*.*\\*/ would stop at the FIRST '*/' and leave  ' c */ x'  unparsed.");
p("  the hand-coded scanner uses a depth counter -- one integer of memory the DFA doesn't have.");
p();

const s2 = "/* /* only one close */ y";
const n2 = lex(s2);
p('  "' + s2 + '"  ->  ' + (n2.errors.length === 0 ? "(no error?!)" : n2.errors[0]));
p();

p("summary: reserved-word trick (1 DFA + lookup) | // and /* */ comments | NESTED comments need a counter"
  + " (not regular) | strings with \\n \\t \\\" \\\\ \\uXXXX | numbers: INT FLOAT exp HEX");

process.stdout.write(sb);
