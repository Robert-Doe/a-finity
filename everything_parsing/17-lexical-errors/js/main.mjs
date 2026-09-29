// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Scanner, ScanError } from "./scanner.mjs";
import { Strategy, scanRecovering, errorStr } from "./recovery.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

const sc = Scanner.fromSpec(readFileSync("fixtures/ajoda.tokens", "utf8"));

p("=== Module 17 - Lexical Errors & Recovery ===");
p();
p("spec: fixtures/ajoda.tokens  (" + sc.rules.length + " rules, from Module 16)");
p();

for (const raw of readFileSync("fixtures/errors.txt", "utf8").split(/\r?\n/)) {
  if (raw.trim() === "" || raw.startsWith("#")) continue;
  p("--- probe: " + raw + " ---");
  report(raw, Strategy.SKIP_ONE, "SKIP_ONE (one ERROR per bad character)");
  report(raw, Strategy.SKIP_TO_RESTART, "SKIP_TO_RESTART (one ERROR per bad run)");
  try {
    sc.scan(raw);
    p("  Module 16 alone: scans cleanly");
  } catch (e) {
    if (!(e instanceof ScanError)) throw e;
    p("  Module 16 alone: stops at " + e.message);
  }
  p();
}

p("--- fixtures/typos.ajoda, SKIP_TO_RESTART ---");
const src = readFileSync("fixtures/typos.ajoda", "utf8").replace(/\r\n/g, "\n");
const r = scanRecovering(sc, src, Strategy.SKIP_TO_RESTART);
p("  " + r.tokens.length + " tokens, " + r.errors.length + " errors:");
for (const e of r.errors) p("    " + errorStr(e));
p("  (the comment on line 4 holds @#$? but is discarded whole, so it is not an error)");
p();

p("summary: report + resynchronize | SKIP_ONE: one error per character"
  + " | SKIP_TO_RESTART: one error per run | one scan finds every error, each with line:col");

process.stdout.write(sb);

function report(input, strategy, label) {
  const res = scanRecovering(sc, input, strategy);
  p("  " + label + ":");
  p("    " + res.tokens.map(t => t.kind === "EOF" ? "EOF" : t.kind + "(" + t.lexeme + ")").join(" "));
  p("    " + res.errors.length + " error" + (res.errors.length === 1 ? "" : "s") + ": "
    + res.errors.map(errorStr).join(", "));
}
