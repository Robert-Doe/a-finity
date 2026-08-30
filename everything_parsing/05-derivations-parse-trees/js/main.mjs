// node js/main.mjs fixtures/expr.grammar
// Same output bytes as the Java build.

import { readFileSync } from "node:fs";
import { basename } from "node:path";
import { Grammar, productionToString } from "./grammar.mjs";
import { ParseTree, productionMultiset } from "./parsetree.mjs";

const path = process.argv[2] ?? "fixtures/expr.grammar";
const g = Grammar.parse(readFileSync(path, "utf8"));
const file = basename(path);

const choices = choicesFor(file);
const target = targetFor(file);

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 05 - Derivations, Parse Trees, Leftmost/Rightmost ===");
p();
p("grammar: " + path.replace(/\\/g, "/"));
for (const prod of g.productions) p(`  (${prod.index}) ${productionToString(prod)}`);
p();
p("target: " + target.join(" "));
p();

const tree = ParseTree.build(g, choices, true);

p("parse tree:");
p(tree.render());
p();
p("yield (terminal frontier): " + tree.terminalYield().join(" "));
p("  matches target? " + arrEq(tree.terminalYield(), target));
p();

printDerivation(true, "leftmost derivation  (always expand the leftmost nonterminal):");
printDerivation(false, "rightmost derivation  (always expand the rightmost nonterminal):");

const multiset = productionMultiset(tree);
p("productions used (multiset, sorted by index):");
p("  leftmost : " + multiset);
p("  rightmost: " + multiset);
p("  same multiset? true  (they are derived from the SAME tree)");
p();

const steps = ParseTree.derivation(g, tree, true).length - 1;
p(`summary: parseTrees=1  canonicalDerivations=2  steps=${steps}  productionsEach=${steps}  sameMultiset=true`);

process.stdout.write(sb);

function printDerivation(leftmost, heading) {
  p(heading);
  const steps = ParseTree.derivation(g, tree, leftmost);
  steps.forEach((st, k) => {
    const form = st.form.length === 0 ? "epsilon" : st.form.join(" ");
    if (k === 0) p("    " + form);
    else p("=>  " + pad(form, 24) + `(${productionToString(st.applied)})`);
  });
  p(`    ${steps.length - 1} steps`);
  p();
}

function choicesFor(f) {
  if (f === "expr.grammar") return [0, 1, 3, 5, 2, 3, 5, 5];
  if (f === "anbn.grammar") return [0, 0, 1];
  return [];
}
function targetFor(f) {
  if (f === "expr.grammar") return ["id", "+", "id", "*", "id"];
  if (f === "anbn.grammar") return ["a", "a", "b", "b"];
  return [];
}
function arrEq(a, b) { return a.length === b.length && a.every((x, i) => x === b[i]); }
function pad(s, w) { return s.length >= w ? s : s + " ".repeat(w - s.length); }
