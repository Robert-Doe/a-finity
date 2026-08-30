// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { ShiftReduce, compact } from "./shiftreduce.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };
const padEnd = (s, w) => { s = String(s); while (s.length < w) s += " "; return s; };

p("=== Module 30 - Handles, Shift-Reduce & Viable Prefixes ===");
p();
p("SHIFT  : push the next input terminal.");
p("REDUCE A -> b : the top |b| stack symbols are b (a HANDLE); pop them, push A.");
p("ACCEPT : stack is [start], input consumed.");
p("the reductions, reversed, are a RIGHTMOST derivation of the input.");
p();

const grammars = {
  expr: Grammar.parse(readFileSync("fixtures/expr.grammar", "utf8")),
  ambiguous: Grammar.parse(readFileSync("fixtures/ambiguous.grammar", "utf8")),
};

for (const line of readFileSync("fixtures/inputs.txt", "utf8").split(/\r?\n/)) {
  if (line.trim() === "" || line.startsWith("#")) continue;
  const c = line.indexOf(":");
  const gname = line.slice(0, c).trim();
  const toks = line.slice(c + 1).trim().split(/\s+/);
  const g = grammars[gname];
  const sr = new ShiftReduce(g);

  p("########## " + gname + " : " + toks.join(" "));
  for (const prod of g.productions) p("  " + prod.lhs + " -> " + (prod.rhs.length === 0 ? "epsilon" : prod.rhs.join(" ")));
  p();

  const r = sr.parse(toks);
  if (!r.ok) { p("  no parse found"); p(); continue; }

  p("  " + padEnd("STACK", 26) + " " + padEnd("ACTION", 40) + " " + "INPUT");
  for (const s of r.steps)
    p("  " + padEnd(s.stack, 26) + " " + padEnd(s.action, 40) + " " + s.rest);
  p();

  p("  reductions applied (bottom-up): " + r.reductions.map(compact).join("  |  "));

  p("  reversed = rightmost derivation:");
  let out = "    " + g.start;
  const rev = [...r.reductions].reverse();
  let form = [g.start];
  for (const prod of rev) {
    let i = -1;
    for (let k = form.length - 1; k >= 0; k--) if (form[k] === prod.lhs) { i = k; break; }
    form = [...form.slice(0, i), ...prod.rhs, ...form.slice(i + 1)];
    out += "\n    => " + form.join(" ") + "   (" + compact(prod) + ")";
  }
  p(out);
  p("  final form == input? " + (form.length === toks.length && form.every((x, k) => x === toks[k])));

  if (r.conflict) p("  CONFLICT: " + r.conflictNote);
  else p("  no conflict: every right-sentential form has a unique handle.");
  p();
}

p("VIABLE PREFIX: any prefix of a right-sentential form that does not run past");
p("the right end of that form's handle. Every STACK column above is a viable");
p("prefix. The set of all viable prefixes of a grammar is REGULAR -- Module 31");
p("builds the DFA that recognizes it, and that DFA is the LR parser's engine.");

process.stdout.write(sb);
