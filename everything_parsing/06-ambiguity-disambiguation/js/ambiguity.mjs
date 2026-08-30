// Module 06 — find every parse tree a grammar gives a string.
//
// A grammar is AMBIGUOUS if some string has two or more parse trees. Since a
// parse tree is in bijection with its leftmost derivation (Module 5), it is
// enough to enumerate every leftmost derivation that yields the target.
//
// Breadth-first over sentential forms, with two prunes:
//   - terminals never disappear → drop forms with more terminals than |w|
//   - in a leftmost derivation everything left of the leftmost nonterminal is
//     final → that terminal prefix must already match the target

import { leftmostNonterminal, enumerate } from "./derivation.mjs";
import { ParseTree } from "./parsetree.mjs";

const MAX_STATES = 1_000_000;

export function allLeftmostDerivations(g, target) {
  const out = [];
  const queue = [{ form: [g.start], choices: [] }];
  let budget = MAX_STATES;

  while (queue.length > 0 && budget-- > 0) {
    const st = queue.shift();
    const i = leftmostNonterminal(g, st.form);
    if (i < 0) {
      if (sameList(st.form, target)) out.push(st.choices);
      continue;
    }
    if (i > target.length) continue;
    if (!sameList(st.form.slice(0, i), target.slice(0, i))) continue;

    for (const p of g.productionsFor(st.form[i])) {
      const next = [...st.form.slice(0, i), ...p.rhs, ...st.form.slice(i + 1)];
      if (terminalCount(g, next) > target.length) continue;
      queue.push({ form: next, choices: [...st.choices, p.index] });
    }
  }
  return out;
}

export function allTrees(g, target) {
  const distinct = new Map();
  for (const choices of allLeftmostDerivations(g, target)) {
    const t = ParseTree.build(g, choices, true);
    const key = t.render();
    if (!distinct.has(key)) distinct.set(key, t);
  }
  return [...distinct.values()];
}

export function isAmbiguousFor(g, target) {
  return allTrees(g, target).length > 1;
}

export function smallestAmbiguousString(g, maxLen) {
  for (const w of enumerate(g, maxLen)) {
    const toks = w === "epsilon" ? [] : w.split(" ");
    if (allTrees(g, toks).length > 1) return toks;
  }
  return null;
}

// ── helpers ──

function terminalCount(g, form) {
  let n = 0;
  for (const s of form) if (!g.isNonterminal(s)) n++;
  return n;
}
function sameList(a, b) { return a.length === b.length && a.every((x, i) => x === b[i]); }
