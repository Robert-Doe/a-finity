// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { lex, tokStr } from "./lexer.mjs";
import { parse, render, terminalYield, SyntaxError_ } from "./recursivedescent.mjs";
import { replay } from "./derivation.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 19 - Recursive-Descent Parsing ===");
p();
p("grammar (no left recursion; Module 23 derives this from  E -> E + T):");
p("  E  -> T E'");
p("  E' -> + T E'  |  epsilon");
p("  T  -> F T'");
p("  T' -> * F T'  |  epsilon");
p("  F  -> ( E )   |  num  |  id");
p("  one method per nonterminal; the productions they fire ARE a leftmost derivation");
p();

for (const line of readFileSync("fixtures/exprs.txt", "utf8").split(/\r?\n/)) {
  if (line.trim() === "" || line.startsWith("#")) continue;
  p(`--- ${line} ---`);

  const toks = lex(line);
  p("  tokens: " + toks.map(t => " " + tokStr(t)).join(""));

  try {
    const r = parse(line);

    p("  productions fired, in order (top of the list = first call):");
    for (const rule of r.rules) p("      " + rule);

    const steps = replay(r.rules);
    p("  same list, replayed as a leftmost derivation:");
    p("      E");
    for (let i = 1; i < steps.length; i++) {
      const s = steps[i];
      p("      =>  " + s.form.padEnd(24) + " (" + s.rule + ")");
    }

    const sentence = steps[steps.length - 1].form;
    const kinds = toks.filter(t => t.kind !== "EOF").map(t => t.kind).join(" ");
    p("  derived sentence: " + sentence);
    p("  token kinds     : " + kinds);
    p("  match? " + (sentence === kinds));

    p("  concrete syntax tree:");
    for (const tl of render(r.tree).split("\n")) p("    " + tl);
    p("  yield: " + terminalYield(r.tree).join(" "));

  } catch (e) {
    if (e instanceof SyntaxError_) {
      p("  SYNTAX ERROR at position " + e.pos + ": " + e.message);
    } else throw e;
  }
  p();
}

p("why not parse the Module 6 grammar  E -> E + T  directly?");
p("  parseE() would call parseE() as its first action, cursor unmoved:");
p("  infinite recursion, stack overflow, zero tokens consumed. Recursive");
p("  descent REQUIRES a non-left-recursive grammar (Module 23). The right-");
p("  recursive E' here makes  a + b + c  lean RIGHT in the tree; left");
p("  associativity is put back when lowering to an AST (Module 39), or by");
p("  writing E' as a loop:");
p("      Node e = parseT();");
p('      while (at("+")) { expect("+"); e = plus(e, parseT()); }   // left-leaning');

process.stdout.write(sb);
