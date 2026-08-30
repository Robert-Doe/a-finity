// The derivation relation, made runnable.
//
//   =>    one step: replace one nonterminal by the rhs of one of its productions
//   =>*   zero or more steps
//   L(G)  = { w in Sigma* : S =>* w }
//
//   leftmostDerivation — apply caller-chosen productions to the leftmost nonterminal
//   enumerate          — every terminal string of length <= maxLen (breadth-first)
//   derives            — is a given string in L(G)? (bounded breadth-first search)

import { productionToString } from "./grammar.mjs";

const SEP = "";               // cannot occur in a whitespace-delimited symbol
const MAX_FORMS = 200_000;          // safety valve for pathological grammars

export function leftmostNonterminal(g, form) {
  for (let i = 0; i < form.length; i++) if (g.isNonterminal(form[i])) return i;
  return -1;
}

function substitute(form, i, rhs) {
  return [...form.slice(0, i), ...rhs, ...form.slice(i + 1)];
}

// Leftmost derivation driven by explicit production indices.
// Returns [{ form, applied }]; step 0 is { form: [S], applied: null }.
export function leftmostDerivation(g, choices) {
  const steps = [];
  let form = [g.start];
  steps.push({ form, applied: null });

  for (const c of choices) {
    const i = leftmostNonterminal(g, form);
    if (i < 0) throw new Error("derivation already complete: no nonterminal left to expand");
    const p = g.productions[c];
    if (p.lhs !== form[i])
      throw new Error(
        `production ${c} is ${productionToString(p)}, but the leftmost nonterminal is '${form[i]}'`);
    form = substitute(form, i, p.rhs);
    steps.push({ form, applied: p });
  }
  return steps;
}

function symbolCount(rendered) {
  if (rendered === "epsilon") return 0;
  let n = 1;
  for (const ch of rendered) if (ch === " ") n++;
  return n;
}

// Every terminal string of length <= maxLen, sorted by (terminal count, lexicographic).
export function enumerate(g, maxLen) {
  const results = new Set();
  const queue = [[g.start]];
  const seen = new Set([key([g.start])]);

  let budget = MAX_FORMS;
  while (queue.length > 0 && budget-- > 0) {
    const form = queue.shift();
    if (terminalCount(g, form) > maxLen) continue;

    const i = leftmostNonterminal(g, form);
    if (i < 0) { results.add(render(form)); continue; }

    for (const p of g.productionsFor(form[i])) {
      const next = substitute(form, i, p.rhs);
      if (terminalCount(g, next) > maxLen) continue;
      const k = key(next);
      if (!seen.has(k)) { seen.add(k); queue.push(next); }
    }
  }
  return [...results].sort((x, y) => {
    const sx = symbolCount(x), sy = symbolCount(y);
    return sx !== sy ? sx - sy : (x < y ? -1 : x > y ? 1 : 0);
  });
}

// Bounded breadth-first search for `target` in L(G).
export function derives(g, target) {
  const bound = target.length;
  const queue = [[g.start]];
  const depth = new Map([[key([g.start]), 0]]);

  let budget = MAX_FORMS;
  while (queue.length > 0 && budget-- > 0) {
    const form = queue.shift();
    const d = depth.get(key(form));

    const i = leftmostNonterminal(g, form);
    if (i < 0) {
      if (sameList(form, target)) {
        const s = d === 1 ? "step" : "steps";
        return { derivable: true, steps: d, note: `S =>* ${display(target)} in ${d} ${s}` };
      }
      continue;
    }
    for (const p of g.productionsFor(form[i])) {
      const next = substitute(form, i, p.rhs);
      if (terminalCount(g, next) > bound) continue;
      const k = key(next);
      if (!depth.has(k)) { depth.set(k, d + 1); queue.push(next); }
    }
  }
  return {
    derivable: false, steps: -1,
    note: `searched every sentential form with <= ${bound} terminal symbols`,
  };
}

// ─────────────────────────────────────────── helpers

function terminalCount(g, form) {
  let n = 0;
  for (const s of form) if (!g.isNonterminal(s)) n++;
  return n;
}
function key(form)      { return form.join(SEP); }
function sameList(a, b) { return a.length === b.length && a.every((x, i) => x === b[i]); }
function render(form)   { return form.length === 0 ? "epsilon" : form.join(" "); }

export function display(form) { return form.length === 0 ? '""' : form.join(" "); }
