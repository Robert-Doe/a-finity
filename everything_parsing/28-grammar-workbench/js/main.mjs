// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { desugar } from "./ebnf.mjs";
import { report, parse, readTokens, traceLines } from "./workbench.mjs";
import { LL1Table } from "./ll1table.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };
const load = (f) => desugar(readFileSync(f, "utf8"));

p("=== Module 28 - Grammar Workbench: from a language's EBNF to a working LL(1) parser ===");
p();

// 1. Ajoda as specified
p("--- 1. the grammar as the spec writes it ---");
sb += report("fixtures/ajoda.ebnf", load("fixtures/ajoda.ebnf")).text;
p();

// 2. the repaired grammar, then real tokens from Module 16
p("--- 2. one repair: fold assignment into ExprStmt ---");
const fixed = load("fixtures/ajoda-fixed.ebnf");
const r = report("fixtures/ajoda-fixed.ebnf", fixed);
sb += r.text;
p();

const table = new LL1Table(r.grammar);
for (const f of ["fixtures/factorial.tokens", "fixtures/missing-semi.tokens"]) {
  const toks = readTokens(readFileSync(f, "utf8"));
  const res = parse(r.grammar, table, fixed.origin, toks, f.includes("factorial") ? 12 : 0);
  p("parse " + f + " (" + toks.length + " tokens from Module 16):");
  if (res.trace.length > 0) {
    p("  first " + res.trace.length + " steps:");
    for (const line of traceLines(res.trace)) p(line);
  }
  if (res.ok)
    p("  ACCEPTED: " + res.expansions + " expansions, " + res.matches + " matches, stack depth up to " + res.maxDepth);
  else
    p("  REJECTED at " + res.error);
  p();
}

// 3. left recursion, found and removed
p("--- 3. a left-recursive grammar ---");
sb += report("fixtures/arith-leftrec.ebnf", load("fixtures/arith-leftrec.ebnf")).text;
p();

p("summary: EBNF read as written | brackets desugared to named helpers | conflicts traced to the"
  + " rules that cause them | left recursion removed | the finished table parses Module 16's tokens");

process.stdout.write(sb);
