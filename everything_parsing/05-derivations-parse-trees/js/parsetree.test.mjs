// node js/parsetree.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { ParseTree, productionMultiset, compact } from "./parsetree.mjs";

console.log("parsetree.test.mjs");

const expr = Grammar.parse("E -> E + T | T\nT -> T * F | F\nF -> ( E ) | id\n");
const lmChoices = [0, 1, 3, 5, 2, 3, 5, 5]; // leftmost derivation of id + id * id
const tree = ParseTree.build(expr, lmChoices, true);

// ── the tree yields the target string ──
equals(tree.terminalYield().join(" "), "id + id * id", "tree yields id + id * id");

// ── leftmost and rightmost derivations off the SAME tree ──
const lm = ParseTree.derivation(expr, tree, true);
const rm = ParseTree.derivation(expr, tree, false);

equals(lm.length, 9, "leftmost: 8 steps + initial form");
equals(rm.length, 9, "rightmost: 8 steps + initial form");

equals(lm[0].form.join(" "), "E", "leftmost starts at E");
equals(rm[0].form.join(" "), "E", "rightmost starts at E");
equals(lm[lm.length - 1].form.join(" "), "id + id * id", "leftmost ends at the yield");
equals(rm[rm.length - 1].form.join(" "), "id + id * id", "rightmost ends at the yield");

equals(lm[2].form.join(" "), "T + T", "leftmost step 2: expand the left E");
equals(rm[2].form.join(" "), "E + T * F", "rightmost step 2: expand the right T");
that(lm[2].form.join(" ") !== rm[2].form.join(" "), "the intermediate forms genuinely differ");

// ── same production multiset ──
const multiset = productionMultiset(tree);
equals(multiset, "[E->E+T, E->T, T->T*F, T->F, T->F, F->id, F->id, F->id]", "8 productions, sorted by index");
equals(prodMultiset(lm), multiset, "leftmost uses exactly this multiset");
equals(prodMultiset(rm), multiset, "rightmost uses exactly this multiset");

// ── a small nested tree: a^n b^n ──
const anbn = Grammar.parse("S -> a S b | epsilon");
const t2 = ParseTree.build(anbn, [0, 0, 1], true);
equals(t2.terminalYield().join(" "), "a a b b", "anbn tree yields a a b b");
equals(t2.render(), [
  "S",
  "+- a",
  "+- S",
  "|  +- a",
  "|  +- S",
  "|  |  +- epsilon",
  "|  +- b",
  "+- b",
].join("\n"), "the nested S tree renders as expected");

// ── build rejects mismatched / insufficient choices ──
that(rejects(() => ParseTree.build(expr, [5], true)),
  "choosing F->id when the frontier nonterminal is E is rejected");
that(rejects(() => ParseTree.build(expr, [1], true)),
  "choices that leave the tree incomplete are rejected");

summary();

function prodMultiset(steps) {
  const ps = steps.filter(s => s.applied).map(s => s.applied);
  ps.sort((a, b) => a.index - b.index);
  return "[" + ps.map(compact).join(", ") + "]";
}
function rejects(fn) { try { fn(); return false; } catch { return true; } }
