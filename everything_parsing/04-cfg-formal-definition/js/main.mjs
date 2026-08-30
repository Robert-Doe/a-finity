// node js/main.mjs fixtures/anbn.grammar
//
// Same four demonstrations as the Java build, byte-for-byte identical output.

import { readFileSync } from "node:fs";
import { basename } from "node:path";
import { Grammar, productionToString } from "./grammar.mjs";
import { leftmostDerivation, enumerate, derives, display } from "./derivation.mjs";

const path = process.argv[2] ?? "fixtures/anbn.grammar";
const g = Grammar.parse(readFileSync(path, "utf8"));

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 04 - Context-Free Grammars ===");
p();

// 1. the grammar
p("grammar: " + path.replace(/\\/g, "/"));
p("  nonterminals: " + [...g.nonterminals].join(" "));
p("  terminals   : " + [...g.terminals()].join(" "));
p("  start       : " + g.start);
p("  productions :");
for (const prod of g.productions) p(`    (${prod.index}) ${productionToString(prod)}`);
p();

// 2. a leftmost derivation
const file = basename(path);
const choices = derivationChoicesFor(file);
const target = leftmostTargetFor(file);
p(`leftmost derivation of ${display(target)}  [choices: ${choices.join(" ")}]`);
const steps = leftmostDerivation(g, choices);
steps.forEach((st, k) => {
  const form = st.form.length === 0 ? "epsilon" : st.form.join(" ");
  if (k === 0) p("    " + form);
  else p("=>  " + pad(form, 16) + `(${st.applied.index}) ${productionToString(st.applied)}`);
});
p(`    ${steps.length} sentential forms, ${steps.length - 1} steps`);
p();

// 3. the language, up to a bound
const bound = 6;
const lang = enumerate(g, bound);
p(`language up to length ${bound}:`);
let shownLen = -1;
for (const w of lang) {
  const len = w === "epsilon" ? 0 : w.split(" ").length;
  if (len !== shownLen) { sb += `    len ${len}: `; shownLen = len; }
  else sb += "  |  ";
  sb += w + "\n";
}
p(`    ${lang.length} strings of length <= ${bound} -- raise the bound and the list always grows (the grammar is infinite)`);
p();

// 4. membership
p("membership (bounded derivation search):");
const tests = testStringsFor(file);
let derivable = 0;
for (const t of tests) {
  const m = derives(g, t);
  if (m.derivable) derivable++;
  p("    " + pad(display(t), 14) + (m.derivable ? "DERIVABLE      " : "NOT DERIVABLE  ") + `(${m.note})`);
}
p();

p(`summary: productions=${g.productions.length}  language=infinite  testsDerivable=${derivable}/${tests.length}`);

process.stdout.write(sb);

// ─────────────────────────────────────────── per-fixture scripts

function derivationChoicesFor(f) {
  if (f === "anbn.grammar") return [0, 0, 1];
  if (f === "balanced.grammar") return [0, 1, 0, 1, 1];
  return [];
}
function leftmostTargetFor(f) {
  if (f === "anbn.grammar") return ["a", "a", "b", "b"];
  if (f === "balanced.grammar") return ["(", ")", "(", ")"];
  return [];
}
function testStringsFor(f) {
  if (f === "anbn.grammar") return [["a", "a", "b", "b"], ["a", "a", "b"], ["b", "a"], []];
  if (f === "balanced.grammar") return [["(", ")"], ["(", "(", ")", ")"], ["(", ")", ")"], []];
  return [];
}
function pad(s, w) { return s.length >= w ? s : s + " ".repeat(w - s.length); }
