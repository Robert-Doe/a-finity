// Module 24 — Left Factoring (mirror of java/LeftFactor.java).
//
//   A -> a b1 | a b2 | rest   ==>   A -> a A' | rest ;  A' -> b1 | b2
// repeat until no nonterminal has two alternatives sharing a first symbol.

import { G } from "./leftrec.mjs";

export function factor(src) {
  const g = G.from(src);
  let changed = true;
  while (changed) {
    changed = false;
    for (const nt of [...g.order]) {
      if (factorOne(g, nt)) { changed = true; break; }
    }
  }
  return g;
}

function factorOne(g, nt) {
  const alts = g.prods.get(nt);
  const prefix = longestSharedPrefix(alts);
  if (prefix.length === 0) return false;

  const withP = [], rest = [];
  for (const a of alts) {
    if (startsWith(a, prefix)) withP.push(a);
    else rest.push(a);
  }

  const ap = fresh(g, nt);
  const newNt = [...rest, [...prefix, ap]];
  const newAp = withP.map(a => a.slice(prefix.length));

  g.prods.set(nt, dedupe(newNt));
  g.prods.set(ap, dedupe(newAp));
  g.order.splice(g.order.indexOf(nt) + 1, 0, ap);
  return true;
}

export function longestSharedPrefix(alts) {
  let best = [];
  for (let i = 0; i < alts.length; i++)
    for (let j = i + 1; j < alts.length; j++) {
      const cp = commonPrefix(alts[i], alts[j]);
      if (cp.length > best.length) best = cp;
    }
  return best;
}

function commonPrefix(a, b) {
  let n = 0;
  while (n < a.length && n < b.length && a[n] === b[n]) n++;
  return a.slice(0, n);
}

function startsWith(a, p) {
  if (a.length < p.length) return false;
  for (let i = 0; i < p.length; i++) if (a[i] !== p[i]) return false;
  return true;
}

function fresh(g, base) {
  let name = base + "'";
  while (g.prods.has(name)) name += "'";
  return name;
}

function dedupe(inp) {
  const seen = new Set();
  const out = [];
  for (const r of inp) { const k = r.join(""); if (!seen.has(k)) { seen.add(k); out.push(r); } }
  return out;
}

export function needsFactoring(g) {
  for (const nt of g.order) {
    const firsts = new Set();
    for (const r of g.prods.get(nt)) {
      const key = r.length === 0 ? "" : r[0];
      if (firsts.has(key)) return true;
      firsts.add(key);
    }
  }
  return false;
}
